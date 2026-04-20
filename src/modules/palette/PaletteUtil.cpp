/**
 * @file
 */

#include "PaletteUtil.h"
#include "core/Log.h"
#include "core/collection/Buffer.h"
#include "palette/Palette.h"

namespace palette {

palette::Palette toPalette(const palette::ColorPalette &colorPalette) {
	palette::Palette palette;
	palette.setName(colorPalette.name());
	palette.setFilename(colorPalette.filename());
	if (colorPalette.size() <= PaletteMaxColors) {
		palette.setSize(colorPalette.size());
		for (size_t i = 0; i < colorPalette.size(); ++i) {
			palette.setColor(i, colorPalette.color(i));
			palette.setColorName(i, colorPalette.colorName(i));
			palette.setMaterial(i, colorPalette.material(i));
		}
	} else {
		const size_t colorCount = (int)colorPalette.size();
		core::Buffer<color::RGBA> colorBuffer;
		colorBuffer.reserve(colorCount);
		for (const auto &e : colorPalette) {
			colorBuffer.push_back(e.color);
		}
		palette.quantize(colorBuffer.data(), colorBuffer.size());
		if ((int)colorBuffer.size() != (int)palette.colorCount()) {
			Log::info("Loaded %i colors and quanitized to %i", (int)colorCount, palette.colorCount());
		}

		for (const auto &entry : colorPalette) {
			const int palIdx = palette.getClosestMatch(entry.color);
			if (palIdx == PaletteColorNotFound) {
				continue;
			}
			palette.setColorName(palIdx, entry.name);
			palette.setMaterial(palIdx, entry.material);
		}
	}
	palette.markDirty();
	return palette;
}

palette::Palette toPaletteQuantized(const palette::ColorPalette &colorPalette, int targetColors) {
	if (colorPalette.size() <= PaletteMaxColors && (targetColors <= 0 || (int)colorPalette.size() <= targetColors)) {
		return toPalette(colorPalette);
	}
	palette::Palette palette;
	palette.setName(colorPalette.name());
	palette.setFilename(colorPalette.filename());
	const size_t colorCount = colorPalette.size();
	core::Buffer<color::RGBA> colorBuffer;
	colorBuffer.reserve(colorCount);
	for (const auto &e : colorPalette) {
		colorBuffer.push_back(e.color);
	}
	palette.quantize(colorBuffer.data(), colorBuffer.size(), targetColors > 0 ? targetColors : 0);
	if ((int)colorBuffer.size() != (int)palette.colorCount()) {
		Log::info("Loaded %i colors and quantized to %i", (int)colorCount, palette.colorCount());
	}
	for (const auto &entry : colorPalette) {
		const int palIdx = palette.getClosestMatch(entry.color);
		if (palIdx == PaletteColorNotFound) {
			continue;
		}
		palette.setColorName(palIdx, entry.name);
		palette.setMaterial(palIdx, entry.material);
	}
	palette.markDirty();
	return palette;
}

palette::ColorPalette toColorPalette(const palette::Palette &palette) {
	palette::ColorPalette colorPalette;
	colorPalette.setSize(palette.size());
	colorPalette.setName(palette.name());
	colorPalette.setFilename(palette.filename());
	for (size_t i = 0; i < palette.size(); ++i) {
		colorPalette.set(i, palette.color(i), palette.colorName(i), palette.material(i));
	}
	colorPalette.markDirty();
	return colorPalette;
}

} // namespace palette
