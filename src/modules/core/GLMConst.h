/**
 * @file
 *
 * What this basically says is that
 * @li the positive x-axis is to your right
 * @li the positive y-axis is up
 * @li the positive z-axis is backwards
 * Think of your screen being the center of the 3 axes and the positive z-axis going through your
 * screen towards you.
 */

#pragma once

#include <glm/vec3.hpp>

namespace glm {

// clang-format off
constexpr vec3 forward  = vec3( 0.0f,  0.0f, -1.0f);
constexpr vec3 backward = vec3( 0.0f,  0.0f,  1.0f);
constexpr vec3 right    = vec3( 1.0f,  0.0f,  0.0f);
constexpr vec3 left     = vec3(-1.0f,  0.0f,  0.0f);
constexpr vec3 up       = vec3( 0.0f,  1.0f,  0.0f);
constexpr vec3 down     = vec3( 0.0f, -1.0f,  0.0f);
// clang-format on

} // namespace glm
