#include <dirt/common.h>
#include <dirt/background.h>
#include <dirt/scene.h>
#include <dirt/surface.h>
#include <filesystem/resolver.h>

namespace
{

auto g_defaultBackground = make_shared<ConstantBackground>(Color3f(1.f, 1.f, 1.f));

} // namespace


shared_ptr<Background> Background::defaultBackground()
{
	return g_defaultBackground;
}

ConstantBackground::ConstantBackground(const json & j)
{  
    // TODO
}

ImageBackground::ImageBackground(const json & j)
{
    // TODO
}

Color3f ImageBackground::value(const Ray3f & ray) const
{
    // TODO
}
