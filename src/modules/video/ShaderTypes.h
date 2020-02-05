/**
 * @file
 */

#pragma once

#include "Types.h"
#include <unordered_map>
#include "core/String.h"

namespace video {

typedef std::unordered_map<core::String, Uniform, core::StringHash> ShaderUniforms;
typedef std::unordered_map<core::String, int, core::StringHash> ShaderAttributes;

}
