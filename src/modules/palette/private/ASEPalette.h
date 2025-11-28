/**
 * @file
 */

#pragma once

#include "PaletteFormat.h"
#include "io/FormatDescription.h"

namespace palette {

/**
 * Adobe Swatch Exchange (ASE) Format
 *
 * @ingroup Formats
 *
 * Byte-order: Big-endian
 *
 * File Structure:
 *  Header (12 bytes):
 *   Signature: "ASEF" (4 bytes)
 *   Version: Two 16-bit unsigned integers representing the major and minor version (4 bytes)
 *   Number of blocks: A 32-bit unsigned integer indicating the number of color and group blocks in the file (4 bytes)
 *
 *  Color/Group Blocks (variable):
 *   Block Type (2 bytes):
 *    COLOR_START (0x0001): Indicates the start of a color block.
 *    GROUP_START (0xc001): Indicates the start of a group block.
 *    GROUP_END (0xc002): Indicates the end of a group block.
 *
 *   Block Length (4 bytes):
 *    The length of the current block in bytes, including the block type and length fields.
 *
 *   Block Name (variable):
 *    Pascal-style string (length byte followed by characters) representing the name of the color or group.
 *
 *   Block Model (4 bytes):
 *    A string representing the color model used in the color block (e.g., "RGB", "CMYK", "LAB", "Gray").
 *
 *   Block Color (variable):
 *    The color values based on the color model. The size of this field depends on the color model.
 *     For example, if the color model is "RGB", this field would consist of three 32-bit floating-point values
 * representing the red, green, and blue components.
 *
 *   Block Type (2 bytes):
 *    The type of color block (global, spot, normal). This field is optional and may not be present in all color
 * blocks.
 *
 * Color Models and Sizes:
 *  RGB:
 *   Size: 3 * 4 bytes (3 floating-point values for red, green, blue)
 *  CMYK:
 *   Size: 4 * 4 bytes (4 floating-point values for cyan, magenta, yellow, black)
 *  LAB:
 *   Size: 3 * 4 bytes (3 floating-point values for lightness, a, b)
 *  GRAY:
 *   Size: 1 * 4 bytes (1 floating-point value for gray intensity)
 *
 * Usage:
 *  The ASE file consists of a sequence of color and group blocks.
 *  Color blocks represent individual colors with their name, model, color values, and type.
 *  Group blocks group colors together and can be nested.
 *
 * http://www.selapa.net/swatches/colors/fileformats.php
 */
class ASEPalette : public PaletteFormat {
protected:
	bool parseColorBlock(io::SeekableReadStream &stream, core::RGBA &rgba, core::String &name) const;
public:
	bool load(const core::String &filename, io::SeekableReadStream &stream, palette::ColorPalette &palette) override;
	bool save(const palette::ColorPalette &palette, const core::String &filename, io::SeekableWriteStream &stream) override;

	static const io::FormatDescription &format() {
		static const io::FormatDescription desc = {"Adobe Swatch Exchange", {"ase"}, {"ASEF"}, FORMAT_FLAG_SAVE};
		return desc;
	}
};

} // namespace palette
