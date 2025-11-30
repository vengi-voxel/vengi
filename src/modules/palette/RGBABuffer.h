/**
 * @file
 */

#pragma once

#include "color/RGBA.h"
#include "core/collection/DynamicSet.h"
#include "palette/Material.h"

namespace palette {

/**
 * see @c Format::createPalette()
 *
 * it's best to use @c flattenRGB() before putting the color into the map to filling
 * the map with almost identical colors (this speeds up the process of quantizing
 * the colors later on)
 */
using RGBABuffer = core::DynamicSet<color::RGBA, 1031, color::RGBAHasher, core::privdynamicmap::EqualCompare, 4096>;
// TODO: PERF: MEM: this is doing a lot of small memory allocations - use a better allocator or quantize the colors directly into the palette
using RGBAMaterialMap = core::DynamicMap<color::RGBA, const palette::Material *, 1031, color::RGBAHasher, core::privdynamicmap::EqualCompare, 4096>;

}
