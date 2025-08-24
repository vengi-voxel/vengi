/**
 * @file
 */

#pragma once

#include "PaletteFormat.h"
#include "palette/RGBABuffer.h"

namespace palette {

/**
 * @brief A palette implementation for formats that support more than 256 colors - we automatically quantize the colors
 * to reduce them to 256
 */
class QuantizedPalette : public PaletteFormat {
public:
	bool load(const core::String &filename, io::SeekableReadStream &stream, palette::Palette &palette) override final;
	virtual bool load(const core::String &filename, io::SeekableReadStream &stream, palette::RGBABuffer &colors) = 0;
};

} // namespace palette
