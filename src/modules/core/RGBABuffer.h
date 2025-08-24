/**
 * @file
 */

#pragma once

#include "core/RGBA.h"
#include "core/collection/DynamicSet.h"

namespace core {

/**
 * see @c Format::createPalette()
 *
 * it's best to use @c flattenRGB() before putting the color into the map to filling
 * the map with almost identical colors (this speeds up the process of quantizing
 * the colors later on)
 */
using RGBABuffer = core::DynamicSet<core::RGBA, 1031, core::RGBAHasher>;

}
