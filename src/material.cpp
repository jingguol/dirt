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

#include <dirt/material.h>
#include <dirt/texture.h>
#include <dirt/parser.h>
#include <dirt/scene.h>
#include <dirt/surface.h>

namespace
{

auto g_defaultMaterial = make_shared<Lambertian>(json{{"albedo", 0.8}});

} // namespace


shared_ptr<const Material> Material::defaultMaterial()
{
	return g_defaultMaterial;
}


inline bool refract(const Vec3f &v, const Vec3f &n, float iorIOverT, Vec3f &refracted, float &cosTheta2)
{
	Vec3f uv = normalize(v);
	float dt = dot(uv,n);
	float discrim = 1.0f - iorIOverT * iorIOverT * (1.0f - dt * dt);
	if (discrim > 0)
	{
		cosTheta2 = std::sqrt(discrim);
		refracted = iorIOverT * (uv - n * dt) - n * cosTheta2;
		return true;
	}
	else
	{
		return false;
	}
}

inline Vec3f reflect(const Vec3f &v, const Vec3f &n)
{
	return v - 2 * dot(v, n) * n;
}

Lambertian::Lambertian(const json & j)
{
	albedo = parseTexture(j.at("albedo"));
}

bool Lambertian::scatter(const Ray3f &ray, const HitInfo &hit, Color3f &attenuation, Ray3f &scattered) const
{
	// TODO: Implement Lambertian reflection
	//       You should assign the albedo to ``attenuation'', and
	//       you should assign the scattered ray to ``scattered''
	//       The origin of the scattered ray should be at the hit point,
	//       and the scattered direction is the shading normal plus a random
	//       point on a sphere (please look at the text book for this)

	//       You can get the hit point using hit.p, and the shading normal using hit.sn

	//       Hint: You can use the function randomInUnitSphere() to get a random
	//       point in a sphere. IMPORTANT: You want to add a random point *on*
	//       a sphere, not *in* the sphere (the text book gets this wrong)
	//       If you normalize the point, you can force it to be on the sphere always, so
	//       add normalize(randomInUnitSphere()) to your shading normal
	scattered = Ray3f(hit.p, hit.sn + normalize(randomInUnitSphere()));
	attenuation = albedo->value(hit);
	return true;
}


Metal::Metal(const json & j)
{
	albedo = parseTexture(j.at("albedo"));
	roughness = parseTexture(j.at("roughness"));
}

bool Metal::scatter(const Ray3f &ray, const HitInfo &hit, Color3f &attenuation, Ray3f &scattered) const
{
	// TODO: Implement metal reflection
	//       This function proceeds similar to the lambertian material, except that the
	//       scattered direction is different.
	//       Instead of adding a point on a sphere to the normal as before, you should add the point
	//       to the *reflected ray direction*.
	//       You can reflect a vector by the normal using reflect(vector, hit.sn); make sure the vector is normalized.
	//       Different to before you can't just use randomInUnitSphere directly; the sphere should be scaled by roughness.
	//       (see text book). In other words, if roughness is 0, the scattered direction should just be the reflected direction.
	//       
	//       This procedure could produce directions below the surface. Handle this by returning false if the scattered direction and the shading normal
	//       point in different directions (i.e. their dot product is negative)
	Vec3f reflected = reflect(normalize(ray.d), hit.sn);
	scattered = Ray3f(hit.p, reflected + luminance(roughness->value(hit)) * normalize(randomInUnitSphere()));
	attenuation = albedo->value(hit);
	return (dot(scattered.d, hit.sn) > 0);
}


Dielectric::Dielectric(const json & j)
{
	ior = j.value("ior", ior);
}

bool Dielectric::scatter(const Ray3f &ray, const HitInfo &hit, Color3f &attenuation, Ray3f &scattered) const
{
	Vec3f normal;
	float eta1, eta2;

	// ensure ior and normal are correctly oriented for computing reflection and refraction
	if (dot(ray.d, hit.sn) > 0)
	{
		normal = -hit.sn;	
		eta1 = ior;
		eta2 = 1.0f;
	}
	else
	{
		normal = hit.sn;
		eta1 = 1.0f;
		eta2 = ior;
	}

	attenuation = Color3f(1);

	// compute reflected + refracted ray
	float cosTheta2, cosTheta1 = dot(ray.d, -normal) / length(ray.d);
	Vec3f refracted, reflected = reflect(ray.d, hit.sn);
	if (!refract(ray.d, normal, eta1 / eta2, refracted, cosTheta2))
	{
		// no refraction, only reflection
		scattered = Ray3f(hit.p, reflected);
		return true;
	}

	// compute fresnel solutions
	float rho_parallel = ((eta2 * cosTheta1) - (eta1 * cosTheta2)) / ((eta2 * cosTheta1) + (eta1 * cosTheta2));
	float rho_perp = ((eta1 * cosTheta1) - (eta2 * cosTheta2)) / ((eta1 * cosTheta1) + (eta2 * cosTheta2));
	float Freflected = (rho_parallel * rho_parallel + rho_perp * rho_perp) / 2.0f;

	// sample scattered or reflected ray
	scattered = randf() < Freflected 
		? Ray3f(hit.p, reflected)
		: Ray3f(hit.p, refracted);

	return true;
}


DiffuseLight::DiffuseLight(const json & j)
{
	emit = j.value("emit", emit);
}

Color3f DiffuseLight::emitted(const Ray3f &ray, const HitInfo &hit) const
{
	// only emit from the normal-facing side
	if (dot(ray.d, hit.sn) > 0)
		return Color3f(0,0,0);
	else
		return emit;
}

BlendMaterial::BlendMaterial(const json & j)
{
	a = parseMaterial(j.at("a"));
	b = parseMaterial(j.at("b"));
	amount = parseTexture(j.at("amount"));
}

bool BlendMaterial::scatter(const Ray3f &ray, const HitInfo &hit, Color3f &attenuation, Ray3f &scattered) const
{
	float t = luminance(amount->value(hit));
	if (randf() < t)
		return b->scatter(ray, hit, attenuation, scattered);
	else
		return a->scatter(ray, hit, attenuation, scattered);
}
