/**
 * @file
 */

#include "PaletteFormat.h"
#include "CSVPalette.h"
#include "GimpPalette.h"
#include "JASCPalette.h"
#include "PNGPalette.h"
#include "QBCLPalette.h"
#include "RGBPalette.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "io/FileStream.h"
#include "io/FormatDescription.h"

namespace voxel {

static core::SharedPtr<PaletteFormat> getFormat(const io::FormatDescription &desc, uint32_t magic) {
	core::SharedPtr<PaletteFormat> format;
	for (const core::String &ext : desc.exts) {
		// you only have to check one of the supported extensions here
		if (ext == "gpl") {
			return core::make_shared<GimpPalette>();
		} else if (ext == "qsm") {
			return core::make_shared<QBCLPalette>();
		} else if (ext == "csv") {
			return core::make_shared<CSVPalette>();
		} else if (ext == "png") {
			return core::make_shared<PNGPalette>();
		} else if (ext == "pal") {
			if (magic == FourCC('J', 'A', 'S', 'C')) {
				return core::make_shared<JASCPalette>();
			}
			return core::make_shared<RGBPalette>();
		} else {
			Log::warn("Unknown extension %s", ext.c_str());
		}
	}
	return {};
}

bool loadPalette(const core::String &filename, io::SeekableReadStream &stream, voxel::Palette &palette) {
	const uint32_t magic = loadMagic(stream);
	const io::FormatDescription *desc = io::getDescription(filename, magic, io::format::palettes());
	if (desc == nullptr) {
		Log::warn("Palette format %s isn't supported", filename.c_str());
		return image::ImagePtr();
	}
	if (const core::SharedPtr<PaletteFormat> &f = getFormat(*desc, magic)) {
		stream.seek(0);
		palette.setSize(0);
		palette.setName(core::string::extractFilename(filename));
		if (f->load(filename, stream, palette)) {
			palette.markDirty();
			return true;
		}
		// even if case the load returned false, the palette could have been partially loaded
		palette.markDirty();
	}
	return false;
}

bool savePalette(const voxel::Palette &palette, const core::String &filename, io::SeekableWriteStream &stream,
				 const io::FormatDescription *desc) {
	Log::info("Save palette to %s", filename.c_str());
	const core::String &extension = core::string::extractExtension(filename);
	const core::String &ext = core::string::extractExtension(filename);
	if (desc) {
		if (!desc->matchesExtension(ext)) {
			desc = nullptr;
		}
	}
	if (desc != nullptr) {
		core::SharedPtr<PaletteFormat> f = getFormat(*desc, 0u);
		if (f) {
			if (f->save(palette, filename, stream)) {
				Log::debug("Saved file for format '%s' (ext: '%s')", desc->name.c_str(), ext.c_str());
				return true;
			}
			Log::error("Failed to save %s file", desc->name.c_str());
			return false;
		}
	}
	for (desc = io::format::palettes(); desc->valid(); ++desc) {
		if (desc->matchesExtension(ext) /*&& (type.empty() || type == desc->name)*/) {
			core::SharedPtr<PaletteFormat> f = getFormat(*desc, 0u);
			if (f) {
				if (f->save(palette, filename, stream)) {
					Log::debug("Saved file for format '%s' (ext: '%s')", desc->name.c_str(), ext.c_str());
					return true;
				}
				Log::error("Failed to save %s file", desc->name.c_str());
				return false;
			}
		}
	}
	return false;
}

} // namespace voxel
