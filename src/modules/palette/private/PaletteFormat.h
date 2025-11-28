/**
 * @file
 */

#pragma once

#include "io/Stream.h"
#include "palette/ColorPalette.h"
#include "palette/Palette.h"

namespace palette {

// TODO: http://www.selapa.net/swatches/colors/fileformats.php
/**
 * @ingroup Formats
 */
class PaletteFormat {
public:
	virtual ~PaletteFormat() = default;
	virtual bool load(const core::String &filename, io::SeekableReadStream &stream, palette::Palette &palette);
	virtual bool save(const palette::Palette &palette, const core::String &filename, io::SeekableWriteStream &stream);

	virtual bool load(const core::String &filename, io::SeekableReadStream &stream, palette::ColorPalette &palette) = 0;
	virtual bool save(const palette::ColorPalette &palette, const core::String &filename,
					  io::SeekableWriteStream &stream) = 0;
};

palette::ColorPalette toColorPalette(const palette::Palette &palette);
palette::Palette toPalette(const palette::ColorPalette &colorPalette);

bool loadPalette(const core::String &filename, io::SeekableReadStream &stream, palette::ColorPalette &palette);
bool loadPalette(const core::String &filename, io::SeekableReadStream &stream, palette::Palette &palette);

bool savePalette(const palette::ColorPalette &palette, const core::String &filename, io::SeekableWriteStream &stream,
				 const io::FormatDescription *desc = nullptr);
bool savePalette(const palette::Palette &palette, const core::String &filename, io::SeekableWriteStream &stream,
				 const io::FormatDescription *desc = nullptr);

} // namespace palette
