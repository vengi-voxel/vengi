/**
 * @file
 */

#pragma once

#include "math/Vec3f.h"

namespace math {

Vec3f polarToVector(float longitude, float latitude);
void vectorToPolar(const Vec3f &vector, float &longitude, float &latitude);

}
