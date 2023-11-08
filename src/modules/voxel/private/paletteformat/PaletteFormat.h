/**
 * @file
 */

#pragma once

#include "io/Stream.h"
#include "voxel/Palette.h"

namespace voxel {

class PaletteFormat {
public:
	virtual bool load(const core::String &filename, io::SeekableReadStream &stream, voxel::Palette &palette) = 0;
	virtual bool save(const voxel::Palette &palette, const core::String &filename, io::SeekableWriteStream &stream) = 0;
};

bool loadPalette(const core::String &filename, io::SeekableReadStream &stream, voxel::Palette &palette);
bool savePalette(const voxel::Palette &palette, const core::String &filename, io::SeekableWriteStream &stream, const io::FormatDescription *desc = nullptr);

} // namespace voxel
