/**
 * @file
 */

#pragma once

#include "PaletteFormat.h"

namespace palette {

/**
 * Gimp format with Aseprite extension for alpha channels
 *
 * @ingroup Formats
 */
class GimpPalette : public PaletteFormat {
public:
	bool load(const core::String &filename, io::SeekableReadStream &stream, palette::Palette &palette) override;
	bool save(const palette::Palette &palette, const core::String &filename, io::SeekableWriteStream &stream) override;
};

} // namespace palette
