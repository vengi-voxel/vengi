/**
 * @file
 */

#pragma once

#include "PaletteFormat.h"

namespace palette {

class PNGPalette : public PaletteFormat {
public:
	bool load(const core::String &filename, io::SeekableReadStream &stream, palette::Palette &palette) override;
	bool save(const palette::Palette &palette, const core::String &filename, io::SeekableWriteStream &stream) override;
};

} // namespace voxel
