/**
* @file
* @brief Signed distance functions based on the mathematical formulations from https://iquilezles.org/articles/distfunctions/
*/

#pragma once

#include "math/Vec3f.h"

namespace math {
namespace sdf {

// Goxel-compatible SDFs (used by the gox format loader)
float goxSphere(const math::Vec3f &p, const math::Vec3f &s);
float goxCube(const math::Vec3f &p, const math::Vec3f &s);
float goxCylinder(const math::Vec3f &p, const math::Vec3f &s);

float sphere(const math::Vec3f &p, float r);
float box(const math::Vec3f &p, const math::Vec3f &b);
float roundBox(const math::Vec3f &p, const math::Vec3f &b, float r);
float boxFrame(const math::Vec3f &p, const math::Vec3f &b, float e);
float torus(const math::Vec3f &p, float majorRadius, float minorRadius);
float capsule(const math::Vec3f &p, const math::Vec3f &a, const math::Vec3f &b, float r);
float cone(const math::Vec3f &p, float angle, float h);
float roundCone(const math::Vec3f &p, float r1, float r2, float h);
float plane(const math::Vec3f &p, const math::Vec3f &n, float h);
float octahedron(const math::Vec3f &p, float s);
float pyramid(const math::Vec3f &p, float h);
float ellipsoid(const math::Vec3f &p, const math::Vec3f &r);

} // namespace sdf
} // namespace math
