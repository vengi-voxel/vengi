/**
 * @file
 */

#include "PNGPalette.h"
#include "core/Log.h"
#include "core/collection/DynamicArray.h"
#include "image/Image.h"
#include "image/ImageType.h"
#include "palette/Palette.h"
#include "palette/PaletteView.h"

namespace palette {

bool PNGPalette::load(const core::String &filename, io::SeekableReadStream &stream, palette::ColorPalette &palette) {
	if (stream.size() == 0) {
		Log::warn("The palette file '%s' is empty", filename.c_str());
		return false;
	}
	image::ImagePtr img = image::createEmptyImage(filename);
	if (!img->load(image::ImageType::PNG, stream, (int)stream.size())) {
		Log::warn("Failed to load the palette image '%s'", filename.c_str());
		return false;
	}
	return palette.load(img);
}

bool PNGPalette::save(const palette::ColorPalette &palette, const core::String &filename, io::SeekableWriteStream &stream) {
	image::Image img(filename);
	core::DynamicArray<core::RGBA> colors;
	colors.resize(palette.size());
	for (size_t i = 0; i < palette.size(); i++) {
		colors[i] = palette.color(i);
	}
	img.loadRGBA((const uint8_t *)colors.data(), palette.size(), 1);
	if (!img.writePNG(stream)) {
		Log::warn("Failed to write the palette file '%s'", filename.c_str());
		return false;
	}
	return true;
}

bool PNGPalette::save(const palette::Palette &palette, const core::String &filename, io::SeekableWriteStream &stream) {
	image::Image img(filename);
	core::RGBA colors[PaletteMaxColors];
	for (int i = 0; i < PaletteMaxColors; i++) {
		colors[i] = palette.color(i);
	}
	// must be palette::PaletteMaxColors - otherwise the exporter uv coordinates must get adopted
	img.loadRGBA((const uint8_t *)colors, PaletteMaxColors, 1);
	if (!img.writePNG(stream)) {
		Log::warn("Failed to write the palette file '%s'", filename.c_str());
		return false;
	}
	return true;
}

} // namespace voxel
