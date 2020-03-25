/**
 * @file
 */

#pragma once

#include "Types.h"
#include "core/String.h"

namespace shadertool {

extern bool parse(const core::String& filename, ShaderStruct& shaderStruct, const core::String& shaderFile, const core::String& buffer, bool vertex);

}
