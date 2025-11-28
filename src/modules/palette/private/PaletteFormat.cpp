/**
 * @file
 */

#include "PaletteFormat.h"
#include "ACBPalette.h"
#include "ASEPalette.h"
#include "AVMTPalette.h"
#include "CSVPalette.h"
#include "GimpPalette.h"
#include "JASCPalette.h"
#include "PNGPalette.h"
#include "PaintNetPalette.h"
#include "PhotoshopPalette.h"
#include "PixeloramaPalette.h"
#include "QBCLPalette.h"
#include "RGBPalette.h"
#include "VPLPalette.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "io/FormatDescription.h"
#include "metric/MetricFacade.h"
#include "palette/Palette.h"
#include "palette/PaletteFormatDescription.h"
#include "palette/PaletteView.h"

namespace palette {

palette::Palette toPalette(const palette::ColorPalette &colorPalette) {
	palette::Palette palette;
	if (colorPalette.size() < PaletteMaxColors) {
		palette.setSize(colorPalette.size());
		palette.setName(colorPalette.name());
		for (size_t i = 0; i < colorPalette.size(); ++i) {
			palette.setColor(i, colorPalette.color(i));
			palette.setColorName(i, colorPalette.colorName(i));
			palette.setMaterial(i, colorPalette.material(i));
		}
	} else {
		const size_t colorCount = (int)colorPalette.size();
		core::Buffer<core::RGBA> colorBuffer;
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

palette::ColorPalette toColorPalette(const palette::Palette &palette) {
	palette::ColorPalette colorPalette;
	colorPalette.setSize(palette.size());
	colorPalette.setName(palette.name());
	for (size_t i = 0; i < palette.size(); ++i) {
		colorPalette.set(i, palette.color(i), palette.colorName(i), palette.material(i));
	}
	colorPalette.markDirty();
	return colorPalette;
}

bool PaletteFormat::save(const palette::Palette &palette, const core::String &filename,
						 io::SeekableWriteStream &stream) {
	return save(toColorPalette(palette), filename, stream);
}

bool PaletteFormat::load(const core::String &filename, io::SeekableReadStream &stream, palette::Palette &palette) {
	palette::ColorPalette colorPalette;
	if (!load(filename, stream, colorPalette)) {
		return false;
	}
	palette = toPalette(colorPalette);
	return true;
}

static core::SharedPtr<PaletteFormat> getFormat(const io::FormatDescription &desc, uint32_t magic) {
	core::SharedPtr<PaletteFormat> format;
	for (const core::String &ext : desc.exts) {
		// you only have to check one of the supported extensions here
		if (ext == GimpPalette::format().mainExtension()) {
			return core::make_shared<GimpPalette>();
		} else if (ext == QBCLPalette::format().mainExtension()) {
			return core::make_shared<QBCLPalette>();
		} else if (ext == ASEPalette::format().mainExtension()) {
			return core::make_shared<ASEPalette>();
		} else if (ext == CSVPalette::format().mainExtension()) {
			return core::make_shared<CSVPalette>();
		} else if (ext == PhotoshopPalette::format().mainExtension()) {
			return core::make_shared<PhotoshopPalette>();
		} else if (ext == PaintNetPalette::format().mainExtension()) {
			return core::make_shared<PaintNetPalette>();
		} else if (ext == PixeloramaPalette::format().mainExtension()) {
			return core::make_shared<PixeloramaPalette>();
		} else if (ext == io::format::png().mainExtension()) {
			return core::make_shared<PNGPalette>();
		} else if (ext == VPLPalette::format().mainExtension()) {
			return core::make_shared<VPLPalette>();
		} else if (ext == AVMTPalette::format().mainExtension()) {
			return core::make_shared<AVMTPalette>();
		} else if (ext == ACBPalette::format().mainExtension()) {
			return core::make_shared<ACBPalette>();
		} else if (ext == JASCPalette::format().mainExtension()) {
			if (desc.name == JASCPalette::format().name || magic == FourCC('J', 'A', 'S', 'C')) {
				return core::make_shared<JASCPalette>();
				// http://www.selapa.net/swatches/colors/fileformats.php
				// } else if (magic == FourCC('R', 'I', 'F', 'F')) {
				// 	return core::make_shared<RIFFPalette>();
			}
			return core::make_shared<RGBPalette>();
		} else {
			Log::warn("Unknown extension %s", ext.c_str());
		}
	}
	return {};
}

template<class PALETTE>
static bool loadPaletteInternal(const core::String &filename, io::SeekableReadStream &stream, PALETTE &palette) {
	const uint32_t magic = loadMagic(stream);
	const io::FormatDescription *desc = io::getDescription(filename, magic, palette::palettes());
	if (desc == nullptr) {
		Log::warn("Palette format %s isn't supported", filename.c_str());
		return false;
	}
	if (const core::SharedPtr<PaletteFormat> &f = getFormat(*desc, magic)) {
		stream.seek(0);
		palette.setSize(0);
		palette.setName(core::string::extractFilename(filename));
		if (f->load(filename, stream, palette)) {
			palette.markDirty();
			const core::String &ext = core::string::extractExtension(filename);
			if (!ext.empty()) {
				metric::count("load", 1, {{"type", ext.toLower()}, {"palette", "true"}});
			}
			return true;
		}
		// even if case the load returned false, the palette could have been partially loaded
		palette.markDirty();
	}
	return false;
}

bool loadPalette(const core::String &filename, io::SeekableReadStream &stream, palette::ColorPalette &palette) {
	return loadPaletteInternal<palette::ColorPalette>(filename, stream, palette);
}

bool loadPalette(const core::String &filename, io::SeekableReadStream &stream, palette::Palette &palette) {
	return loadPaletteInternal<palette::Palette>(filename, stream, palette);
}

template<class PALETTE>
static bool savePaletteInternal(const PALETTE &palette, const core::String &filename, io::SeekableWriteStream &stream,
								const io::FormatDescription *desc) {
	Log::info("Saving palette to '%s'", filename.c_str());
	const core::String &ext = core::string::extractExtension(filename);
	if (desc && !desc->matchesExtension(ext)) {
		desc = nullptr;
	}
	if (desc != nullptr) {
		if (core::SharedPtr<PaletteFormat> f = getFormat(*desc, 0u)) {
			if (f->save(palette, filename, stream)) {
				Log::debug("Saved file for format '%s' (ext: '%s')", desc->name.c_str(), ext.c_str());
				if (!ext.empty()) {
					metric::count("save", 1, {{"type", ext.toLower()}, {"palette", "true"}});
				}
				return true;
			}
			Log::error("Failed to save '%s' file", desc->name.c_str());
			return false;
		}
	}
	if (ext.empty()) {
		Log::error("No extension found for '%s' - can't determine the palette format", filename.c_str());
		return false;
	}
	for (desc = palette::palettes(); desc->valid(); ++desc) {
		if (!desc->matchesExtension(ext) /*&& (type.empty() || type == desc->name)*/) {
			continue;
		}
		if (core::SharedPtr<PaletteFormat> f = getFormat(*desc, 0u)) {
			if (f->save(palette, filename, stream)) {
				Log::debug("Saved file for format '%s' (ext: '%s')", desc->name.c_str(), ext.c_str());
				if (!ext.empty()) {
					metric::count("save", 1, {{"type", ext.toLower()}, {"palette", "true"}});
				}
				return true;
			}
			Log::error("Failed to save '%s' file", desc->name.c_str());
			return false;
		}
	}
	Log::error("Failed to find a matching palette format for '%s'", filename.c_str());
	return false;
}

bool savePalette(const palette::ColorPalette &palette, const core::String &filename, io::SeekableWriteStream &stream,
				 const io::FormatDescription *desc) {
	return savePaletteInternal<palette::ColorPalette>(palette, filename, stream, desc);
}

bool savePalette(const palette::Palette &palette, const core::String &filename, io::SeekableWriteStream &stream,
				 const io::FormatDescription *desc) {
	return savePaletteInternal<palette::Palette>(palette, filename, stream, desc);
}

} // namespace palette
