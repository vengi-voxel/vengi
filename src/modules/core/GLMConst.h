/**
 * @file
 */

#pragma once

#include <glm/vec3.hpp>

namespace glm {

constexpr vec3 forward  = vec3( 0.0f,  0.0f, -1.0f);
constexpr vec3 backward = vec3( 0.0f,  0.0f,  1.0f);
constexpr vec3 right    = vec3( 1.0f,  0.0f,  0.0f);
constexpr vec3 left     = vec3(-1.0f,  0.0f,  0.0f);
constexpr vec3 up       = vec3( 0.0f,  1.0f,  0.0f);
constexpr vec3 down     = vec3( 0.0f, -1.0f,  0.0f);

}
