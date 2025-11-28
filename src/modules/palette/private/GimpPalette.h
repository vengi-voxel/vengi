/**
 * @file
 */

#pragma once

#include "PaletteFormat.h"
#include "io/FormatDescription.h"

namespace palette {

/**
 * Gimp format with Aseprite extension for alpha channels
 *
 * https://github.com/LibreSprite/LibreSprite/tree/master/data/palettes
 *
 * @ingroup Formats
 */
class GimpPalette : public PaletteFormat {
public:
	bool load(const core::String &filename, io::SeekableReadStream &stream, palette::ColorPalette &palette) override;
	bool save(const palette::ColorPalette &palette, const core::String &filename, io::SeekableWriteStream &stream) override;

	static const io::FormatDescription &format() {
		static const io::FormatDescription desc = {"Gimp Palette", {"gpl"}, {}, FORMAT_FLAG_SAVE};
		return desc;
	}
};

} // namespace palette
