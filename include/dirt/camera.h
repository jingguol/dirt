/*
    This file is part of Dirt, the Dartmouth introductory ray tracer.

    Copyright (c) 2017-2019 by Wojciech Jarosz

    Dirt is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License Version 3
    as published by the Free Software Foundation.

    Dirt is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <dirt/transform.h>
#include <dirt/vec.h>
#include <dirt/parser.h>
#include <dirt/sampling.h>
#include <dirt/medium.h>
#include <PolynomialOptics/TruncPolySystem.hh>
#include <PolynomialOptics/TwoPlane5.hh>
#include <PolynomialOptics/Propagation5.hh>
#include <PolynomialOptics/Spherical5.hh>
#include <PolynomialOptics/Cylindrical5.hh>
#include <PolynomialOptics/OpticalMaterial.hh>
#include <fstream>
#include <iostream>

/**
    This class represents a virtual pinhole camera.
   
    The camera is responsible for generating primary rays. It is positioned
    using a Transform and points along the -z axis of the local coordinate
    system. It has an image plane positioned a z = -dist with size
    (width, height).
   
    We currently only support pinhole perspective cameras. This class could
    be made into a virtual base class to support other types of cameras
    (e.g. an orthographic camera, or omni-directional camera).
 */
class Camera
{
public:
    /// Construct a camera from json parameters.
    Camera(const json & j = json())
    {
		m_xform = j.value("transform", m_xform);
	    m_resolution = j.value("resolution", m_resolution);
	    m_focalDistance = j.value("fdist", m_focalDistance);
	    m_apertureRadius = j.value("aperture", m_apertureRadius);
        if (j.contains("medium"))
        {
            m_medium = parseMedium(j.at("medium"));
        }
        if (j.contains("lens")) {
            json jLens = j.at("lens");
            m_useLens = true;
            int degree = jLens.value("degree", 3);
            const float lambda = 550;
            float radius = jLens.value("radius", 25.0);
            float distance = jLens.value("distance", radius);
            string material = jLens.value("material", "S-LAL7");
            OpticalMaterial glass1(material.c_str());
            string type = jLens.at("type").get<string>();
            if (type == "spherical_concave") {
                m_lens = propagate_5(distance, degree) >> 
                    refract_spherical_5(-radius, 1.0, glass1.get_index(lambda), degree) >>
                    propagate_5(radius, degree) >>
                    refract_spherical_5(radius, glass1.get_index(lambda), 1.0, degree);
                m_lensLength = distance + radius;
            } else if (type == "spherical_convex") {
                m_lens = propagate_5(distance, degree) >> 
                    refract_spherical_5(radius, 1.0, glass1.get_index(lambda), degree) >>
                    propagate_5(radius, degree) >>
                    refract_spherical_5(-radius, glass1.get_index(lambda), 1.0, degree);
                m_lensLength = distance + radius;
            } else if (type == "cylindrical_x") {
                m_lens = propagate_5(distance, degree) >> 
                    refract_cylindrical_x_5(-radius, 1.0, glass1.get_index(lambda), degree) >>
                    propagate_5(radius * 2, degree) >>
                    refract_cylindrical_x_5(radius, glass1.get_index(lambda), 1.0, degree);
                m_lensLength = distance + 2 * radius;
            } else if (type == "cylindrical_y") {
                m_lens = propagate_5(distance, degree) >> 
                    refract_cylindrical_y_5(-radius, 1.0, glass1.get_index(lambda), degree) >>
                    propagate_5(radius * 2, degree) >>
                    refract_cylindrical_y_5(radius, glass1.get_index(lambda), 1.0, degree);
                m_lensLength = distance + 2 * radius;
            } else {
                throw DirtException("Unknown 'lens' type '%s' here:\n%s.", type, j.dump(4));
            }
        }

		float vfov = 90.f; // Default vfov value. Override this with the value from json
        // TODO: Assignment 1: read the vertical field-of-view from j ("vfov"),
        // and compute the width and height of the image plane. Remember that
        // the "vfov" parameter is specified in degrees, but C++ math functions
        // expect it in radians. You can use deg2rad() from common.h to convert
        // from one to the other
        vfov = deg2rad(j.value("vfov", vfov));	
	    m_size.y = 2*tan(vfov/2) * m_focalDistance;
	    m_size.x = float(m_resolution.x)/m_resolution.y * m_size.y;
    }

	/// Return the camera's image resolution
	Vec2i resolution() const {return m_resolution;}

    /**
        Generate a ray going through image-plane location (u,v).

        (\c u,\c v) range from 0 to m_resolution.x() and m_resolution.y() along
       	the x- and y-axis of the rendered image, respectively

        \param u 	The horizontal position within the image
        \param v  	The vertical position within the image
        \return 	The \ref Ray3f data structure filled with the
       				appropriate position and direction
     */
    Ray3f generateRay(float u, float v) const
    {
        u /= m_resolution.x;
		v /= m_resolution.y;

		Vec2f disk = m_apertureRadius*randomInUnitDisk();
		Vec3f origin(disk.x, disk.y, 0.f);
        Vec3f dir(Vec3f((u - 0.5f) * m_size.x, (0.5f - v) * m_size.y, -m_focalDistance) - origin);
        dir = normalize(dir);
        Ray3f ray(origin, dir);

        if (m_useLens) {
            float lensInput[4] = {origin[0], origin[1], dir[0], dir[1]};
            float lensOutput[4];
            Transform4f lens = m_lens;
            lens.evaluate(lensInput, lensOutput, false);
            Vec3f newDir = Vec3f(lensOutput[2], lensOutput[3], -sqrtf(1 - lensOutput[2] * lensOutput[2] - lensOutput[3] * lensOutput[3]));
            Vec3f newOrigin = Vec3f(lensOutput[0], lensOutput[1], m_lensLength);
            Ray3f newRay = Ray3f(newOrigin, newDir);

            // if (randf() < 1e-5) {
            //     std::cout << origin << '\n' << newOrigin << '\n';
            //     std::cout << dir << '\n' << newDir << "\n\n";
            // }
            return m_xform.ray(newRay).withMedium(m_medium);
        } else {
            return m_xform.ray(ray).withMedium(m_medium);
        }  
    }

private:
	//
	// The camera setup looks something like this, where the
	// up vector points out of the screen:
	//
	//         top view                         side view
	//            ^                    up
	//            |                     ^
	//            |                     |             _,-'
	//          width                   |         _,-'   |
	//       +----|----+     +          |     _,-'       | h
	//        \   |   /    d |        e | _,-'           | e
	//         \  |  /     i |        y +'---------------+-i----->
	//          \ | /      s |        e  '-,_   dist     | g
	//           \|/       t |               '-,_        | h
	//            +          +                   '-,_    | t
	//           eye                                 '-,_|
	//


	Transform m_xform = Transform();      ///< Local coordinate system
	Vec2f m_size = Vec2f(1,1);            ///< Physical size of the image plane
	float m_focalDistance = 1.f;          ///< Distance to image plane along local z axis
	Vec2i m_resolution = Vec2i(512,512);  ///< Image resolution
	float m_apertureRadius = 0.f;         ///< The size of the aperture for depth of field
    std::shared_ptr<const Medium> m_medium;

    bool m_useLens = false;
    float m_lensLength = 0.0f;
    Transform4f m_lens;
    Transform4f get_system_from_file(char *filename, float lambda, int degree, float distance) {
        std::ifstream infile(filename);
        std::string line;

        Transform4f system;
        while (std::getline(infile, line)) {
            std::istringstream ls(line);
            std::string op;
            ls >> op;

            if (op == "two_plane") {
                system = two_plane_5(distance, degree);
                //cout << "two_plane" << " " << d << endl;
            }
            else if (op == "cylindrical_x") {
                float radius;
                std::string glassName1;
                std::string glassName2;
                ls >> radius;
                ls >> glassName1;
                ls >> glassName2;
                float n1 = 1.0f;
                float n2 = 1.0f;
                if (glassName1[0] >= '0' && glassName1[0] <= '9') {
                    n1 = atof(glassName1.c_str());

                } else {
                    OpticalMaterial glass1(glassName1.c_str());
                    n1 = glass1.get_index(lambda);
                }

                if (glassName2[0] >= '0' && glassName2[0] <= '9') {
                    n2 = atof(glassName2.c_str());
                } else {
                    OpticalMaterial glass2(glassName2.c_str());
                    n2 = glass2.get_index(lambda);
                }

                system = system >> refract_cylindrical_x_5(radius, n1, n2);
            }
            else if (op == "cylindrical_y") {
                float radius;
                std::string glassName1;
                std::string glassName2;
                ls >> radius;
                ls >> glassName1;
                ls >> glassName2;
                float n1 = 1.0f;
                float n2 = 1.0f;
                if (glassName1[0] >= '0' && glassName1[0] <= '9') {
                    n1 = atof(glassName1.c_str());

                } else {
                    OpticalMaterial glass1(glassName1.c_str());
                    n1 = glass1.get_index(lambda);
                }

                if (glassName2[0] >= '0' && glassName2[0] <= '9') {
                    n2 = atof(glassName2.c_str());
                } else {
                    OpticalMaterial glass2(glassName2.c_str());
                    n2 = glass2.get_index(lambda);
                }

                system = system >> refract_cylindrical_y_5(radius, n1, n2);
            }
            else if (op == "reflect_spherical") {
                float radius;
                ls >> radius;
                system = system >> reflect_spherical_5(radius, degree);
            }
            else if (op == "refract_spherical") {
                    float radius;
                    std::string glassName1;
                    std::string glassName2;
                    ls >> radius;
                    ls >> glassName1;
                    ls >> glassName2;

                    float n1 = 1.0f;
                    float n2 = 1.0f;

                    if (glassName1[0] >= '0' && glassName1[0] <= '9') {
                        n1 = atof(glassName1.c_str());

                    } else {
                        OpticalMaterial glass1(glassName1.c_str());
                        n1 = glass1.get_index(lambda);
                    }

                    if (glassName2[0] >= '0' && glassName2[0] <= '9') {
                        n2 = atof(glassName2.c_str());
                    } else {
                        OpticalMaterial glass2(glassName2.c_str());
                        n2 = glass2.get_index(lambda);
                    }

                    system = system >> refract_spherical_5(radius, n1, n2, degree);
                    //system = system >> refract_cylindrical_x_5(radius, n1, n2, degree);
                    //cout << "refract_spherical" << " " << radius << " " << n1 << " " << n2 << endl;

            } else if (op == "propagate") {
                float d;
                ls >> d;
                system = system >> propagate_5(d, degree);
                //cout << "propagate" << " " << d << endl;

            } else {
                cout << "invalid op: " << op << endl;
            }
        }
        return system;
    }
};