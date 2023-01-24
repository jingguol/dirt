#pragma once

#include "common.h"

inline Vec2f randomInUnitDisk()
{
	Vec2f p;
	do
	{
        float a = randf();
        float b = randf();
		p = 2.0f * Vec2f(a, b) - Vec2f(1);
	} while (length2(p) >= 1.f);

	return p;
}


inline Vec3f randomInUnitSphere()
{
	Vec3f p;
	do
	{
        float a = randf();
        float b = randf();
        float c = randf();
		p = 2.0f * Vec3f(a, b, c) - Vec3f(1);
	} while (length2(p) >= 1.0f);

	return p;
}
