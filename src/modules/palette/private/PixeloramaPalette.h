/**
 * @file
 */

#pragma once

#include "PaletteFormat.h"
#include "io/FormatDescription.h"

namespace palette {

/**
 * Pixelorama json palette format
 *
 * https://github.com/Orama-Interactive/Pixelorama/blob/master/pixelorama_data/Palettes/Pixelorama.json
 *
 * @ingroup Formats
 */
class PixeloramaPalette : public PaletteFormat {
public:
	bool load(const core::String &filename, io::SeekableReadStream &stream, palette::ColorPalette &palette) override;
	bool save(const palette::ColorPalette &palette, const core::String &filename, io::SeekableWriteStream &stream) override;

	static const io::FormatDescription &format() {
		static const io::FormatDescription desc = {"Pixelorama", "", {"json"}, {}, FORMAT_FLAG_SAVE};
		return desc;
	}
};

} // namespace palette
