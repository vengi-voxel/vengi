/**
 * @file
 */

#include "PNGPalette.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "image/Image.h"
#include "palette/Palette.h"

namespace voxel {

bool PNGPalette::load(const core::String &filename, io::SeekableReadStream &stream, voxel::Palette &palette) {
	image::ImagePtr img = image::createEmptyImage(filename);
	img->load(stream, stream.size());
	return palette.load(img);
}

bool PNGPalette::save(const voxel::Palette &palette, const core::String &filename, io::SeekableWriteStream &stream) {
	image::Image img(filename);
	// must be voxel::PaletteMaxColors - otherwise the exporter uv coordinates must get adopted
	img.loadRGBA((const uint8_t *)palette.colors(), PaletteMaxColors, 1);
	if (!img.writePng(stream)) {
		Log::warn("Failed to write the palette file '%s'", filename.c_str());
		return false;
	}
	return true;
}

} // namespace voxel
