/**
 * @file
 */

#pragma once

#include "PaletteFormat.h"
#include "core/collection/Array.h"
#include "core/collection/Buffer.h"
#include "io/FormatDescription.h"

namespace palette {

/**
 * @ingroup Formats
 *
 * Command and Conquer VPL (Voxel Palette Lookup) palette format
 *
 * 8bit colors followed by the normal palette indices in 1-32 sections
 */
class VPLPalette : public PaletteFormat {
private:
	core::Buffer<core::Array<uint8_t, 256>> _luts;
public:
	bool load(const core::String &filename, io::SeekableReadStream &stream, palette::ColorPalette &palette) override;
	bool save(const palette::ColorPalette &palette, const core::String &filename, io::SeekableWriteStream &stream) override {
		return false;
	}

	uint8_t index(uint8_t section, uint8_t color) const;

	static const io::FormatDescription &format() {
		static const io::FormatDescription desc{"Tiberian Sun Palette", "", {"vpl"}, {}, 0u};
		return desc;
	}
};

} // namespace palette
