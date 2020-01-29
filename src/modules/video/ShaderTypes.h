/**
 * @file
 */

#pragma once

#include "Types.h"
#include <unordered_map>
#include "core/String.h"

namespace video {

typedef std::unordered_map<std::string, Uniform> ShaderUniforms;
typedef std::unordered_map<std::string, int> ShaderAttributes;

}
