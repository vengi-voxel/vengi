/**
 * @file
 */

#pragma once

#include "Types.h"
#include <string>

namespace shadertool {

extern bool parse(ShaderStruct& shaderStruct, const std::string& shaderFile, const std::string& buffer, bool vertex);

}
