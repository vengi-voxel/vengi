/**
 * @file
 */

#include "ASEPalette.h"
#include "color/CMYK.h"
#include "color/Color.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "palette/Palette.h"

namespace palette {

namespace priv {

enum BlockTypes : uint16_t { COLOR_START = 0x0001, GROUP_START = 0xc001, GROUP_END = 0xc002 };

}

bool ASEPalette::parseColorBlock(io::SeekableReadStream &stream, color::RGBA &rgba, core::String &name) const {
	uint16_t nameLength;
	if (stream.readUInt16BE(nameLength) == -1) {
		Log::error("ASEPalette: Failed to read name length");
		return false;
	}
	if (nameLength > 0) {
		stream.readUTF16BE(nameLength, name);
		Log::debug("Name: %s", name.c_str());
	}

	union ToUpper {
		uint32_t colorMode;
		char colorModeStr[4];
	} mode;
	if (stream.readUInt32(mode.colorMode) == -1) {
		Log::error("ASEPalette: Failed to read color mode");
		return false;
	}
	for (int j = 0; j < 4; ++j) {
		mode.colorModeStr[j] = core::string::toUpper(mode.colorModeStr[j]);
	}

	uint8_t buf[4];
	FourCCRev(buf, mode.colorMode);
	core::String colorModeStr((const char *)buf, 4);
	Log::debug("ASEPalette: color mode %s", colorModeStr.c_str());

	glm::vec4 color(0.0f);
	if (mode.colorMode == FourCC('C', 'M', 'Y', 'K')) {
		stream.readFloatBE(color[0]);
		stream.readFloatBE(color[1]);
		stream.readFloatBE(color[2]);
		stream.readFloatBE(color[3]);
		const color::CMYK cmyk(color[0], color[1], color[2], color[3]);
		rgba = cmyk.toRGB();
	} else if (mode.colorMode == FourCC('R', 'G', 'B', ' ')) {
		stream.readFloatBE(color[0]);
		stream.readFloatBE(color[1]);
		stream.readFloatBE(color[2]);
		color[3] = 1.0f;
		rgba = color::Color::getRGBA(color);
	} else if (mode.colorMode == FourCC('L', 'A', 'B', ' ')) {
		stream.readFloatBE(color[0]);
		stream.readFloatBE(color[1]);
		stream.readFloatBE(color[2]);
		color[3] = 1.0f;
		// L goes from 0 to 100 percent
		color[0] *= 100.0f;
		rgba = color::Color::fromCIELab(color);
	} else if (mode.colorMode == FourCC('G', 'R', 'A', 'Y')) {
		stream.readFloatBE(color[0]);
		color[1] = color[2] = color[0];
		color[3] = 1.0f;
		rgba = color::Color::getRGBA(color);
	} else {
		Log::error("ASEPalette: Unknown color mode %s", colorModeStr.c_str());
		return false;
	}

	int16_t colorType; // 0 = global, 1 = spot, 2 = normal
	stream.readInt16BE(colorType);

	return true;
}

bool ASEPalette::load(const core::String &filename, io::SeekableReadStream &stream, palette::ColorPalette &palette) {
	int colorCount = 0;

	uint32_t magic;
	if (stream.readUInt32(magic) == -1) {
		Log::error("ASEPalette: Failed to read magic");
		return false;
	}

	if (magic != FourCC('A', 'S', 'E', 'F')) {
		Log::error("ASEPalette: Invalid magic");
		return false;
	}

	uint16_t versionMajor;
	if (stream.readUInt16BE(versionMajor) == -1) {
		Log::error("ASEPalette: Failed to read version major");
		return false;
	}

	uint16_t versionMinor;
	if (stream.readUInt16BE(versionMinor) == -1) {
		Log::error("ASEPalette: Failed to read version minor");
		return false;
	}
	Log::debug("Found version %d.%d", versionMajor, versionMinor);

	uint32_t blocks;
	if (stream.readUInt32BE(blocks) == -1) {
		Log::error("ASEPalette: Failed to read blocks");
		return false;
	}
	Log::debug("Found %d blocks", blocks);

	for (uint32_t i = 0; i < blocks; ++i) {
		uint16_t blockType;
		if (stream.readUInt16BE(blockType) == -1) {
			Log::error("ASEPalette: Failed to read block type of block %d/%d", i, blocks);
			return false;
		}
		uint32_t blockLength;
		if (stream.readUInt32BE(blockLength) == -1) {
			Log::error("ASEPalette: Failed to read block length");
			return false;
		}
		if (blockType == priv::COLOR_START) {
			color::RGBA rgba;
			core::String name;
			if (!parseColorBlock(stream, rgba, name)) {
				Log::error("ASEPalette: Failed to parse color block %d/%d", i, blocks);
				return false;
			}
			palette.setColor(colorCount, rgba);
			palette.setColorName(colorCount, name);
			++colorCount;
			continue;
		}
		// only extract the colors
		stream.skip(blockLength);
	}

	palette.setSize(colorCount);
	return colorCount > 0;
}

bool ASEPalette::save(const palette::ColorPalette &palette, const core::String &filename, io::SeekableWriteStream &stream) {
	stream.writeUInt32(FourCC('A', 'S', 'E', 'F'));
	stream.writeUInt16BE(1);						// versionMajor
	stream.writeUInt16BE(0);						// versionMinor
	stream.writeUInt32BE((uint16_t)palette.size()); // blocks
	// TODO: write group with palette name
	for (size_t i = 0; i < palette.size(); ++i) {
		const color::RGBA &color = palette.color(i);
		const glm::vec4 scaled = color::Color::fromRGBA(color);
		stream.writeUInt16BE(priv::COLOR_START);		// blocktype
		stream.writeUInt32BE(18);						// blocksize
		stream.writeUInt16BE(0);						// namelength
		stream.writeUInt32(FourCC('R', 'G', 'B', ' ')); // colormode
		stream.writeFloatBE(scaled.r);
		stream.writeFloatBE(scaled.g);
		stream.writeFloatBE(scaled.b);
		stream.writeInt16BE(0); // colorType
	}
	return true;
}

} // namespace voxel
