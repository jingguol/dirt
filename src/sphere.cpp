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

#include <dirt/sphere.h>
#include <dirt/scene.h>


Sphere::Sphere(float radius,
               shared_ptr<const Material> material,
               const Transform & xform)
    : Surface(xform), m_radius(radius), m_material(material)
{

}

Sphere::Sphere(const Scene & scene, const json & j)
    : Surface(scene, j)
{
	m_radius = j.value("radius", m_radius);
    m_material = scene.findOrCreateMaterial(j);
}

Box3f Sphere::localBBox() const
{
    return Box3f(Vec3f(-m_radius), Vec3f(m_radius));
}



bool Sphere::intersect(const Ray3f &ray, HitInfo &hit) const
{
    INCREMENT_INTERSECTION_TESTS;
    // TODO: Assignment 1: Implement ray-sphere intersection

    putYourCodeHere("Assignment 1: Insert your ray-sphere intersection code here");
    return false;

    // TODO: If the ray misses the sphere, you should return false
    // TODO: If you successfully hit something, you should compute the hit point, 
    //       hit distance, and normal and fill in these values
    float hitT = 0.0f;
    Vec3f hitPoint;
    Vec3f geometricNormal;

    // For this assignment you can leave these values as is
    Vec3f shadingNormal = geometricNormal;
    Vec2f uvCoordinates = Vec2f(0.0f, 0.0f);

    // You should only assign hit and return true if you successfully hit something
    hit = HitInfo(hitT,
            hitPoint,
            geometricNormal,
            shadingNormal,
            uvCoordinates,
            m_material.get(),
            this);

    return true;

}
