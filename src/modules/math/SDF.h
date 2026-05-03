/**
 * @file
 * @brief Signed distance functions based on the mathematical formulations from https://iquilezles.org/articles/distfunctions/
 */

#pragma once

#include <glm/fwd.hpp>

namespace math {
namespace sdf {

float sphere(const glm::vec3 &p, float r);
float box(const glm::vec3 &p, const glm::vec3 &b);
float roundBox(const glm::vec3 &p, const glm::vec3 &b, float r);
float boxFrame(const glm::vec3 &p, const glm::vec3 &b, float e);
float torus(const glm::vec3 &p, float majorRadius, float minorRadius);
float capsule(const glm::vec3 &p, const glm::vec3 &a, const glm::vec3 &b, float r);
float cone(const glm::vec3 &p, float angle, float h);
float roundCone(const glm::vec3 &p, float r1, float r2, float h);
float plane(const glm::vec3 &p, const glm::vec3 &n, float h);
float octahedron(const glm::vec3 &p, float s);
float pyramid(const glm::vec3 &p, float h);
float ellipsoid(const glm::vec3 &p, const glm::vec3 &r);

} // namespace sdf
} // namespace math
