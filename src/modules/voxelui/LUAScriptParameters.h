/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/DynamicArray.h"

namespace palette {
class Palette;
}

namespace voxelgenerator {
struct LUAParameterDescription;
}

namespace voxelui {

void renderScriptParameters(const core::DynamicArray<voxelgenerator::LUAParameterDescription> &params,
							core::DynamicArray<core::String> &values,
							const palette::Palette *palette = nullptr);

} // namespace voxelui
