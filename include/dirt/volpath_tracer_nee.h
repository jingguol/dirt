#pragma once

#include <dirt/integrator.h>
#include <dirt/medium.h>
#include <dirt/scene.h>
#include <dirt/pdf.h>

class VolpathTracerNEE: public Integrator
{
public:
    VolpathTracerNEE(const json& j = json::object())
    {
        m_maxBounces = j.value("max_bounces", m_maxBounces);
        m_power = j.value("mis_power", m_power);
    }

    float misWeight(float pdfA, float pdfB) const
    {
        pdfA = std::pow(pdfA, m_power);
        pdfB = std::pow(pdfB, m_power);
        return pdfA / (pdfA + pdfB);
    }
    
    /** Set a ray's maxt to a given hit point. (Plus epsilon! The hit point still intersects the ray)
     * 
     * This assumes that the ray actually points at the hit point.
     * Otherwise, the calculated maxt is incorrect.
    */
    inline void setRayMaxt(const HitInfo &hit, Ray3f &ray) const
    {
        ray.maxt = length(hit.p - ray.o) / length(ray.d) + Epsilon;
    }

    Color3f surfaceNEE(const Scene &scene, Sampler &sampler, const HitInfo &hit, const Ray3f &ray, const Color3f &throughput) const
    {
        Color3f result(0.0f);

        Vec3f lightSample = scene.emitters().sample(hit.p, sampler.next2D());
        Ray3f scat = Ray3f(hit.p, normalize(lightSample));
        float lightPdf = scene.emitters().pdf(hit.p, scat.d);
        if (lightPdf > 0.0f)
        {
            float bsdfPdf = hit.mat->pdf(ray.d, scat.d, hit);
            result += misWeight(lightPdf, bsdfPdf) * throughput * hit.mat->eval(ray.d, scat.d, hit) * TrL(scene, sampler, scat) / lightPdf;
        }

        ScatterRecord srec;
        if (hit.mat->sample(ray.d, hit, sampler.next2D(), srec))
        {
            Ray3f scat = Ray3f(hit.p, srec.scattered).withMedium(ray.medium);
            float bsdfPdf = hit.mat->pdf(ray.d, scat.d, hit);
            if (bsdfPdf > 0.0f)
            {
                float lightPdf = scene.emitters().pdf(hit.p, scat.d);
                result += misWeight(bsdfPdf, lightPdf) * throughput * hit.mat->eval(ray.d, scat.d, hit) * TrL(scene, sampler, scat) / bsdfPdf;
            }
        }
        return result;
    }

    Color3f mediumNEE(const Scene &scene, Sampler &sampler, const MediumInteraction &mi, const Ray3f &ray, const Color3f &throughput) const
    {
        Color3f result(0.0f);

        Vec3f lightSample = scene.emitters().sample(mi.p, sampler.next2D());
        Ray3f scat = Ray3f(mi.p, normalize(lightSample)).withMedium(ray.medium);
        float lightPdf = scene.emitters().pdf(mi.p, scat.d);
        if (lightPdf > 0.0f)
        {
            float phasePdf = mi.medium->phase->p(mi.wo, scat.d);
            result += misWeight(lightPdf, phasePdf) * throughput * mi.medium->phase->p(mi.wo, scat.d) * TrL(scene, sampler, scat) / lightPdf;
        }

        Vec3f wi;
        float phasePdf = mi.medium->phase->sample(mi.wo, wi, sampler.next2D());
        if (phasePdf > 0.0f)
        {
            Ray3f scat = Ray3f(mi.p, wi).withMedium(ray.medium);
            float lightPdf = scene.emitters().pdf(mi.p, scat.d);
            result += misWeight(phasePdf, lightPdf) * throughput * mi.medium->phase->p(mi.wo, scat.d) * TrL(scene, sampler, scat) / phasePdf;
        }

        return result;
    }

    virtual Color3f Li(const Scene & scene, Sampler &sampler, const Ray3f& ray_) const override
    {
        Ray3f ray(ray_);
        Color3f throughput(1.f);
        Color3f result(0.f);
        Vec3f wi;

        // first compute primary ray connection to light source
        HitInfo hit;
        Ray3f primaryRay(ray_);
        if (scene.intersect(primaryRay, hit)) setRayMaxt(hit, primaryRay);
        result += TrL(scene, sampler, primaryRay);

        int bounces = 0;
        while (bounces < m_maxBounces)
        {
            HitInfo hit;
            bool foundIntersection = scene.intersect(ray, hit);
            if (foundIntersection)
                setRayMaxt(hit, ray); // Cut ray short at intersection point

            // Sample a medium interaction.
            MediumInteraction mi;
            if (ray.medium)
                throughput *= ray.medium->Sample(ray, sampler, mi);

            // Did we get a medium interaction?
            if (mi.isValid())
            {
                // Sampled a medium interaction
                result += mediumNEE(scene, sampler, mi, ray, throughput);
                float phasePdf = mi.medium->phase->sample(mi.wo, wi, sampler.next2D());
                throughput *= mi.medium->phase->p(mi.wo, wi) / phasePdf;
                ray = Ray3f(mi.p, wi).withMedium(ray.medium);
                bounces ++;
            }
            else
            {
                // No medium interaction: We didn't hit any particles along the way

                // Do we have a surface hit?
                if (!foundIntersection)
                {
                    // No, the ray went all the way to the background
                    result += throughput * scene.background(ray);
                    break;
                }

                // We have a surface hit.

                // Do we have a surface material?
                if (hit.mat == nullptr)
                {
                    // No surface material: continue along the same direction, potentially change medium

                    ray = Ray3f(hit.p, normalize(ray.d)).withMedium(ray.medium);
                
                    // set next medium if this is a transition
                    if (hit.mi->IsMediumTransition())
                        ray.medium = hit.mi->getMedium(ray, hit);

                    // don't increment bounce count.
                    continue;
                }

                // We have a surface material. Sample it.

                ScatterRecord srec;
                if (!hit.mat->sample(ray.d, hit, sampler.next2D(), srec)) break;
                Ray3f scat = Ray3f(hit.p, srec.scattered).withMedium(ray.medium);

                if (srec.isSpecular)
                {
                    throughput *= srec.attenuation;
                    result += throughput * TrL(scene, sampler, scat);
                }
                else
                {
                    result += surfaceNEE(scene, sampler, hit, ray, throughput);
                    float bsdfPdf = hit.mat->pdf(ray.d, scat.d, hit);
                    if (bsdfPdf == 0.0f) break;
                    throughput *= hit.mat->eval(ray.d, scat.d, hit) / bsdfPdf;
                }

                ray = scat;
                bounces ++;
            }

            // Russian roulette
            float lum = luminance(throughput);
            const float rrThreshold = 1.0f;
            if (lum < rrThreshold)
            {
                float q = std::max((float).05, 1.0f - lum);
                if (sampler.next1D() < q) break;
                throughput /= (1 - q);
            }
        }
        return result;
    }

private:
    int m_maxBounces = 64;
    float m_power = 2;
};
