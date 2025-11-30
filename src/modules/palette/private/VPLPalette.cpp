/**
 * @file
 */

#include "VPLPalette.h"
#include "core/Log.h"
#include "core/Var.h"

namespace palette {

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Error: " CORE_STRINGIFY(read) " at " CORE_FILE ":%i", CORE_LINE);                                  \
		return false;                                                                                                  \
	}

bool VPLPalette::load(const core::String &filename, io::SeekableReadStream &stream, palette::ColorPalette &palette) {
	uint32_t remapStart;
	wrap(stream.readUInt32(remapStart))
	uint32_t remapEnd;
	wrap(stream.readUInt32(remapEnd))
	uint32_t sectionCount;
	wrap(stream.readUInt32(sectionCount))
	uint32_t unknown;
	wrap(stream.readUInt32(unknown))

	palette.setSize(PaletteMaxColors);
	for (int i = 0; i < PaletteMaxColors; ++i) {
		color::RGBA color;
		wrap(stream.readUInt8(color.r))
		wrap(stream.readUInt8(color.g))
		wrap(stream.readUInt8(color.b))
		color.a = 255;
		palette.setColor(i, color);
	}

	// normal lookup tables (1-32)
	_luts.resize(sectionCount);

	for (uint32_t i = 0; i < sectionCount; ++i) {
		for (int n = 0; n < 256; ++n) {
			wrap(stream.readUInt8(_luts[i][n]))
		}
	}

	return true;
}

uint8_t VPLPalette::index(uint8_t section, uint8_t color) const {
	if (section >= _luts.size()) {
		return 0;
	}
	return _luts[section][color];
}

#undef wrap

} // namespace palette
