#include <dirt/sampler.h>
#include <dirt/primetable.h>

namespace
{
  auto g_defaultSampler = std::make_shared<IndependentSampler>(json::object());
}

shared_ptr<Sampler> Sampler::defaultSampler()
{
  return g_defaultSampler;
}

void Sampler::startPixel()
{
  current1DDimension = 0;
  current2DDimension = 0;
  currentPixelSample = 0;
}

bool Sampler::startNextPixelSample()
{
  current1DDimension = 0;
  current2DDimension = 0;
  currentPixelSample++;
  currentGlobalSample++;
  return currentPixelSample < samplesPerPixel;
}

Vec2f Sampler::next2D()
{
  return Vec2f(next1D(), next1D());
}

IndependentSampler::IndependentSampler(const json &j) {}

float IndependentSampler::next1D()
{
  return randf();
}

StratifiedSampler::StratifiedSampler(const json &j) {
  // samplesPerPixel must be a perfect square (e.g. 1, 4, 9, 16, etc)
  samplesPerPixel = roundToPerfectSquare(j.value("samplesPerPixel", 4));
  dimension = j.value("dimension", 4);

  // TODO
}

void StratifiedSampler::startPixel()
{
  // TODO
  Sampler::startPixel();
}

void StratifiedSampler::stratifiedSample1D(std::vector<float> &samples)
{
  // TODO
}

void StratifiedSampler::stratifiedSample2D(std::vector<Vec2f> &samples)
{
  // TODO
}

float StratifiedSampler::next1D()
{
  // TODO 
}

Vec2f StratifiedSampler::next2D()
{
  // TODO
}

HaltonSampler::HaltonSampler(const json &j)
{
  dimension = j.value("dimension", 4);
  if (dimension > 0)
    dimension = std::min(dimension, (size_t)(PrimeTableSize));
  else
    dimension = PrimeTableSize;

  // TODO
}


float HaltonSampler::scrambledRadicalInverse(const std::vector<uint64_t> &perm, uint64_t a, uint64_t base)
{
  // TODO
}

float HaltonSampler::next1D()
{
  // TODO
}
