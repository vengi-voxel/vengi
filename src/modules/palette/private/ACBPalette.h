/**
 * @file
 */

#pragma once

#include "io/FormatDescription.h"
#include "QuantizedPalette.h"

namespace palette {

/**
 * Adobe Color Bock (binary)
 *
 * https://github.com/albemala/color_palette_formats/tree/main/assets
 * https://www.adobe.com/devnet-apps/photoshop/fileformatashtml/#50577411_pgfId-1066780
 * https://ates.dev/pages/acb-spec/
 *
 * Test file: https://github.com/1j01/palettes/blob/bb970c808fabfaaf228780eeb449febd88e751eb/AdobeColorBook/TRUMATCH.acb
 */
class ACBPalette : public QuantizedPalette {
public:
	bool load(const core::String &filename, io::SeekableReadStream &stream, RGBAMap &colors) override;
	bool save(const palette::Palette &palette, const core::String &filename, io::SeekableWriteStream &stream) override {
		return false;
	}

	static const io::FormatDescription &format() {
		static const io::FormatDescription desc = {"Adobe Color Bock", {"acb"}, {"8BCB"}, 0u};
		return desc;
	}
};

} // namespace palette
