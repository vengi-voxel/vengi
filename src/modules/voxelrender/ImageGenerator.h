/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "image/Image.h"
#include "io/Stream.h"

namespace voxelrender {

image::ImagePtr volumeThumbnail(const core::String &fileName, io::SeekableReadStream &stream, int outputSize);

} // namespace voxelrender
