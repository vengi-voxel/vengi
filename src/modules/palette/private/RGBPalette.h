/**
 * @file
 */

#pragma once

#include "PaletteFormat.h"

namespace palette {

/**
 * @ingroup Formats
 *
 * 768 byte files with RGB data
 */
class RGBPalette : public PaletteFormat {
public:
	bool load(const core::String &filename, io::SeekableReadStream &stream, palette::Palette &palette) override;
	bool save(const palette::Palette &palette, const core::String &filename, io::SeekableWriteStream &stream) override;
};

} // namespace palette
