/**
 * @file
 */

#include "QuantizedPalette.h"
#include "core/Log.h"
#include "core/RGBA.h"
#include "core/collection/Buffer.h"

namespace palette {

bool QuantizedPalette::load(const core::String &filename, io::SeekableReadStream &stream, palette::Palette &palette) {
	palette::RGBABuffer colors;
	colors.reserve(1024 * 256);
	if (!load(filename, stream, colors)) {
		return false;
	}
	const size_t colorCount = (int)colors.size();
	core::Buffer<core::RGBA> colorBuffer;
	colorBuffer.reserve(colorCount);
	for (const auto &e : colors) {
		colorBuffer.push_back(e->first);
	}
	palette.quantize(colorBuffer.data(), colorBuffer.size());
	if ((int)colorBuffer.size() != (int)palette.colorCount()) {
		Log::info("Loaded %i colors and quanitized to %i", (int)colorCount, palette.colorCount());
	}
	return palette.colorCount() > 0;
}

} // namespace palette
