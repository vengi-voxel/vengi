/**
 * @file
 */

#pragma once

#include "PaletteFormat.h"
#include "io/FormatDescription.h"

namespace palette {

/**
 * Paint.NET palette format
 *
 * @ingroup Formats
 */
class PaintNetPalette : public PaletteFormat {
public:
	bool load(const core::String &filename, io::SeekableReadStream &stream, palette::ColorPalette &palette) override;
	bool save(const palette::ColorPalette &palette, const core::String &filename, io::SeekableWriteStream &stream) override;

	static const io::FormatDescription &format() {
		static const io::FormatDescription desc = {"Paint.NET Palette", "", {"txt"}, {}, FORMAT_FLAG_SAVE};
		return desc;
	}
};

} // namespace palette
