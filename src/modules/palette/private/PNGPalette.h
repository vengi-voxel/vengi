/**
 * @file
 */

#pragma once

#include "PaletteFormat.h"

namespace palette {

/**
 * Load a 256x1 PNG image as a palette or quantize an image to a palette.
 *
 * @ingroup Formats
 */
class PNGPalette : public PaletteFormat {
public:
	bool load(const core::String &filename, io::SeekableReadStream &stream, palette::ColorPalette &palette) override;
	bool save(const palette::ColorPalette &palette, const core::String &filename,
			  io::SeekableWriteStream &stream) override;
	bool save(const palette::Palette &palette, const core::String &filename, io::SeekableWriteStream &stream) override;
};

} // namespace palette
