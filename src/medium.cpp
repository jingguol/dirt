#include <dirt/medium.h>
#include <dirt/parser.h>
#include <dirt/onb.h>
#include <dirt/ray.h>
#include <dirt/scene.h>
#include <dirt/sampler.h>

HenyeyGreenstein::HenyeyGreenstein(const json &j)
{
  g = j.value("g", g);
}

// HenyeyGreenstein Method Definitions

float HenyeyGreenstein::p(const Vec3f &wo, const Vec3f &wi) const
{
	// TODO Part 2, Task 1
}

float HenyeyGreenstein::sample(const Vec3f &wo, Vec3f &wi, const Vec2f &u) const
{
	// TODO Part 2, Task 2
}


HomogeneousMedium::HomogeneousMedium(const json &j)
{
  phase = parsePhase(j.at("phase"));
  sigma_a = j.value("sigma_a", sigma_a);
  sigma_s = j.value("sigma_s", sigma_s);
  sigma_t = sigma_s + sigma_a;
}

float HomogeneousMedium::Tr(const Ray3f &ray_, Sampler &sampler) const
{
  Ray3f ray = ray_.normalizeRay();
  // TODO Part 3, Task 1
  return 0;
}

float HomogeneousMedium::Sample(const Ray3f &ray_, Sampler &sampler, MediumInteraction &mi) const
{
  Ray3f ray = ray_.normalizeRay();
  // TODO Part 3, Task 2
  return 0;
}

float HomogeneousMedium::density(const Vec3f &p) const
{
  return sigma_t;
}

PerlinMedium::PerlinMedium(const json &j)
{
  phase = parsePhase(j.at("phase"));
  sigma_a = j.value("sigma_a", sigma_a);
  sigma_s = j.value("sigma_s", sigma_s);
  sigma_t = sigma_s + sigma_a;

  spatialScale = j.value("spatial_scale", spatialScale);
  densityScale = abs(j.value("density_scale", densityScale));
  densityOffset = j.value("density_offset", densityOffset);

  assert(densityScale + densityOffset > 0.0f);

  invMaxDensity = 1.0f / (sigma_t * (densityScale + densityOffset));
}

float PerlinMedium::Tr(const Ray3f &ray_, Sampler &sampler) const
{
  Ray3f ray = ray_.normalizeRay();
  // TODO Part 5
  return 0;
}

float PerlinMedium::Sample(const Ray3f &ray_, Sampler &sampler, MediumInteraction &mi) const
{
  Ray3f ray = ray_.normalizeRay();
  // TODO Part 4
  return 0;
}

float PerlinMedium::density(const Vec3f &p) const
{
  Vec3f pScaled(p.x * spatialScale.x, p.y * spatialScale.y, p.z * spatialScale.z);
  return sigma_t * std::max(0.0f, densityScale * perlin.noise(pScaled) + densityOffset);
}

std::shared_ptr<const Medium> MediumInterface::getMedium(const Ray3f ray, const HitInfo &hit) const
{
    if (dot(hit.sn, ray.d) < 0)
      return inside;
    else
      return outside;
}

Color3f TrL(const Scene &scene, Sampler &sampler, const Ray3f &ray_)
{
  Ray3f ray = ray_.normalizeRay();
  float Tr = 1.0;
  while (true)
  {
    HitInfo hit;
    bool hitSurface = scene.intersect(ray, hit);

    if (hitSurface) ray.maxt = length(hit.p - ray.o) + 2.0 * Epsilon;

    // hit an emitter
    if (hitSurface && hit.mat != nullptr)
      return hit.mat->isEmissive() ? Tr * hit.mat->emitted(ray, hit) : Color3f(0.0f);

    // stil in medium
    if (ray.medium) Tr *= ray.medium->Tr(ray, sampler);

    // if transmittance below threshold exit
    if (Tr < Epsilon) break;

    // escaped scene (assume no infinite lights)
    if (!hitSurface)
      return Tr * scene.background(ray);

    // set medium based on whether we are entering or exiting the surface
    if (hit.mi->IsMediumTransition())
      ray.medium = hit.mi->getMedium(ray, hit);

    // update ray origin
    ray.o = hit.p;
  }

  return Color3f(0.0f);
}
