/**
 * @file
 */

#pragma once

#include "Types.h"
#include "core/String.h"

namespace shadertool {

extern bool parse(ShaderStruct& shaderStruct, const std::string& shaderFile, const std::string& buffer, bool vertex);

}
