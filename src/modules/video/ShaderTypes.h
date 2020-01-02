/**
 * @file
 */

#pragma once

#include "Types.h"
#include <unordered_map>
#include <string>

namespace video {

typedef std::unordered_map<std::string, Uniform> ShaderUniforms;
typedef std::unordered_map<std::string, int> ShaderAttributes;

}
