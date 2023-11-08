/**
 * @file
 */

#pragma once

#include "PaletteFormat.h"

namespace voxel {

class CSVPalette : public PaletteFormat {
public:
	bool load(const core::String &filename, io::SeekableReadStream &stream, voxel::Palette &palette) override;
	bool save(const voxel::Palette &palette, const core::String &filename, io::SeekableWriteStream &stream) override;
};

} // namespace voxel
