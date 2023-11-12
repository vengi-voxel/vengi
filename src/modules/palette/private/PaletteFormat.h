/**
 * @file
 */

#pragma once

#include "io/Stream.h"
#include "palette/Palette.h"

namespace palette {

// TODO: http://www.selapa.net/swatches/colors/fileformats.php
/**
 * @ingroup Formats
 */
class PaletteFormat {
public:
	virtual ~PaletteFormat() = default;
	virtual bool load(const core::String &filename, io::SeekableReadStream &stream, palette::Palette &palette) = 0;
	virtual bool save(const palette::Palette &palette, const core::String &filename,
					  io::SeekableWriteStream &stream) = 0;
};

bool loadPalette(const core::String &filename, io::SeekableReadStream &stream, palette::Palette &palette);
bool savePalette(const palette::Palette &palette, const core::String &filename, io::SeekableWriteStream &stream,
				 const io::FormatDescription *desc = nullptr);

} // namespace palette
