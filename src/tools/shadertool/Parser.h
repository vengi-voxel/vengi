/**
 * @file
 */

#pragma once

#include "Types.h"
#include "core/String.h"

namespace shadertool {

extern bool parse(ShaderStruct& shaderStruct, const core::String& shaderFile, const core::String& buffer, bool vertex);

}
