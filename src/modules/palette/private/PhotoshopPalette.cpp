/**
 * @file
 */

#include "PhotoshopPalette.h"
#include "AdobeColorSpace.h"
#include "color/CMYK.h"
#include "color/Color.h"
#include "core/Log.h"

namespace palette {

static bool readColor(io::SeekableReadStream &stream, palette::ColorPalette &palette, uint16_t version) {
	uint16_t colorSpace;
	if (stream.readUInt16BE(colorSpace) == -1) {
		Log::error("PhotoshopPalette: Failed to read color space");
		return false;
	}
	int paletteColorIdx = palette.colorCount();
	uint16_t colorComponents[4];
	for (int j = 0; j < 4; ++j) {
		if (stream.readUInt16BE(colorComponents[j]) == -1) {
			Log::error("PhotoshopPalette: Failed to read color component %d", j);
			return false;
		}
	}
	if (version >= 2) {
		uint32_t characters;
		if (stream.readUInt32BE(characters) == -1) {
			Log::error("PhotoshopPalette: Failed to read color name length");
			return false;
		}
		core::String colorName;
		if (!stream.readUTF16BE(characters, colorName)) {
			Log::error("PhotoshopPalette: Failed to read color name with %i characters", (int)characters);
			return false;
		}
		palette.setColorName(paletteColorIdx, colorName);
	}

	core::RGBA rgba;
	// Convert the color space to RGBA
	switch (colorSpace) {
	case adobe::ColorSpace::RGB: {
		//  The first three values in the color data are red, green, and blue. They are full unsigned 16-bit values as
		//  in Apple's RGBColor data structure. Pure red = 65535,0,0.
		rgba.r = colorComponents[0] / 65535.0f * 255.0f;
		rgba.g = colorComponents[1] / 65535.0f * 255.0f;
		rgba.b = colorComponents[2] / 65535.0f * 255.0f;
		rgba.a = 255;
		break;
	}
	case adobe::ColorSpace::HSB: {
		//  The first three values in the color data are hue, saturation, and brightness. They are full unsigned 16-bit
		//  values as in Apple's HSVColor data structure. Pure red = 0,65535,65535.
		rgba = core::Color::fromHSB(colorComponents[0] / 65535.0f * 360.0f, colorComponents[1] / 65535.0f * 100.0f,
									colorComponents[2] / 65535.0f * 100.0f);
		break;
	}
	case adobe::ColorSpace::CMYK: {
		//  The four values in the color data are cyan , magenta , yellow , and black. They are full unsigned 16-bit
		//  values. 0 = 100% ink. For example, pure cyan = 0,65535,65535,65535.
		// TODO: multiply by 100.0f is correct?
		core::CMYK cmyk(
			1.0f - colorComponents[0] / 65535.0f, 1.0f - colorComponents[1] / 65535.0f,
			1.0f - colorComponents[2] / 65535.0f, 1.0f - colorComponents[3] / 65535.0f);
		rgba = cmyk.toRGB();
		break;
	}
	case adobe::ColorSpace::Grayscale: {
		//  The first value in the color data is the gray value, from 0...10000.
		uint8_t grayValue = (uint8_t)((1.0f - colorComponents[0] / 10000.0f) * 255.0f);
		rgba.r = grayValue;
		rgba.g = grayValue;
		rgba.b = grayValue;
		rgba.a = 255;
		break;
	}
	case adobe::ColorSpace::Lab: {
		//  The first three values in the color data are lightness, a chrominance, and b chrominance.
		//  Lightness is a 16-bit value from 0...10000. Chrominance components are each 16-bit values from
		//  -12800...12700. Gray values are represented by chrominance components of 0. Pure white = 10000,0,0.
		Log::error("Unsupported color space: %i", (int)colorSpace);
		return false;
	}
	default:
		Log::error("Unsupported color space: %i", (int)colorSpace);
		return false;
	}

	palette.setColor(paletteColorIdx, rgba);

	return true;
}

bool PhotoshopPalette::load(const core::String &filename, io::SeekableReadStream &stream, palette::ColorPalette &palette) {
	// Load the ACO Photoshop palette file
	uint16_t version;
	if (stream.readUInt16BE(version) == -1) {
		Log::error("PhotoshopPalette: Failed to read version");
		return false;
	}

	// The palette might have two versions, 1 and 2.
	// Version 1 contains the first set of colors and version 2 contains additional metadata
	if (version != 1 && version != 2) {
		Log::error("PhotoshopPalette: Unsupported version: %d", version);
		return false;
	}

	uint16_t colorCount;
	if (stream.readUInt16BE(colorCount) == -1) {
		Log::error("PhotoshopPalette: Failed to read color count");
		return false;
	}

	const int64_t remaining = stream.remaining();
	// version 1 photoshop < 7.0
	// version 2 might be attached to version 1 to keep
	// it compatible with older versions
	if (version == 1) {
		if (remaining != colorCount * 10) {
			// Skip version 1 and take the version 2 data
			stream.skip(colorCount * 10);
			if (stream.readUInt16BE(version) == -1) {
				Log::error("PhotoshopPalette: Failed to read version");
				return false;
			}
			if (stream.readUInt16BE(colorCount) == -1) {
				Log::error("PhotoshopPalette: Failed to read color count");
				return false;
			}
		}
	}
	Log::debug("Found %i colors in the palette", (int)colorCount);
	for (uint16_t i = 0; i < colorCount; ++i) {
		if (!readColor(stream, palette, version)) {
			return false;
		}
	}
	return true;
}

bool PhotoshopPalette::save(const palette::ColorPalette &palette, const core::String &filename,
							io::SeekableWriteStream &stream) {
	// save version 1
	stream.writeUInt16BE(1);
	stream.writeUInt16BE(palette.colorCount());
	for (int i = 0; i < palette.colorCount(); ++i) {
		const core::RGBA &rgba = palette.color(i);
		stream.writeUInt16BE(0); // RGB
		stream.writeUInt16BE((uint16_t)(rgba.r / 255.0f * 65535.0f));
		stream.writeUInt16BE((uint16_t)(rgba.g / 255.0f * 65535.0f));
		stream.writeUInt16BE((uint16_t)(rgba.b / 255.0f * 65535.0f));
		stream.writeUInt16BE(0u);
	}
	// append version 2
	stream.writeUInt16BE(2);
	stream.writeUInt16BE(palette.colorCount());
	for (int i = 0; i < palette.colorCount(); ++i) {
		const core::RGBA &rgba = palette.color(i);
		stream.writeUInt16BE(0); // RGB
		stream.writeUInt16BE((uint16_t)(rgba.r / 255.0f * 65535.0f));
		stream.writeUInt16BE((uint16_t)(rgba.g / 255.0f * 65535.0f));
		stream.writeUInt16BE((uint16_t)(rgba.b / 255.0f * 65535.0f));
		stream.writeUInt16BE(0u);
		// no color name
		if (!stream.writeUInt32BE(0)) {
			return false;
		}
	}
	return true;
}

} // namespace palette
