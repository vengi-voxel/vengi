/**
 * @file
 */

#pragma once

#include "PaletteFormat.h"

namespace palette {

/**
 * Adobe Photoshop palette format
 *
 * https://www.adobe.com/devnet-apps/photoshop/fileformatashtml/#50577411_pgfId-1055819
 *
 * @ingroup Formats
 */
class PhotoshopPalette : public PaletteFormat {
public:
	bool load(const core::String &filename, io::SeekableReadStream &stream, palette::Palette &palette) override;
	bool save(const palette::Palette &palette, const core::String &filename, io::SeekableWriteStream &stream) override;
};

} // namespace palette
