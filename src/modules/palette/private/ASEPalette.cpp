/**
 * @file
 */

#include "ASEPalette.h"
#include "core/CMYK.h"
#include "core/Color.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "palette/Palette.h"

namespace palette {

namespace priv {

enum BlockTypes { COLOR_START = 0x0001, GROUP_START = 0xc001, GROUP_END = 0xc002 };

}

bool ASEPalette::load(const core::String &filename, io::SeekableReadStream &stream, palette::Palette &palette) {
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
		if (blockType != priv::COLOR_START) {
			// only extract the colors
			stream.skip(blockLength);
			continue;
		}
		uint16_t nameLength;
		if (stream.readUInt16BE(nameLength) == -1) {
			Log::error("ASEPalette: Failed to read name length");
			return false;
		}
		if (nameLength > 0) {
			core::String name;
			stream.readUTF16BE(nameLength, name);
			Log::debug("Name: %s", name.c_str());
		}

		union {
			uint32_t colorMode;
			char colorModeStr[4];
		};
		if (stream.readUInt32(colorMode) == -1) {
			Log::error("ASEPalette: Failed to read color mode");
			return false;
		}
		for (int i = 0; i < 4; ++i) {
			colorModeStr[i] = core::string::toUpper(colorModeStr[i]);
		}
		glm::vec4 color(0.0f);
		if (colorMode == FourCC('C', 'Y', 'M', 'K')) {
			stream.readFloatBE(color[0]);
			stream.readFloatBE(color[1]);
			stream.readFloatBE(color[2]);
			stream.readFloatBE(color[3]);
			const core::CMYK cmyk(color[0], color[1], color[2], color[3]);
			palette.setColor(colorCount, cmyk.toRGB());
			++colorCount;
			continue;
		}

		if (colorMode == FourCC('R', 'G', 'B', ' ') || colorMode == FourCC('L', 'A', 'B', ' ')) {
			stream.readFloatBE(color[0]);
			stream.readFloatBE(color[1]);
			stream.readFloatBE(color[2]);
			color[3] = 1.0f;
		} else if (colorMode == FourCC('G', 'R', 'A', 'Y')) {
			stream.readFloatBE(color[0]);
			color[1] = color[2] = color[0];
			color[3] = 1.0f;
		} else {
			continue;
		}
		palette.setColor(colorCount, core::Color::getRGBA(color));
		++colorCount;
	}

	palette.setSize(colorCount);
	return colorCount > 0;
}

bool ASEPalette::save(const palette::Palette &palette, const core::String &filename, io::SeekableWriteStream &stream) {
	stream.writeUInt32(FourCC('A', 'S', 'E', 'F'));
	stream.writeUInt16BE(1);						// versionMajor
	stream.writeUInt16BE(0);						// versionMinor
	stream.writeUInt32BE((uint16_t)palette.size()); // blocks
	// TODO: write group with palette name
	for (size_t i = 0; i < palette.size(); ++i) {
		const core::RGBA &color = palette.color(i);
		const glm::vec4 scaled = core::Color::fromRGBA(color);
		stream.writeUInt16BE(priv::COLOR_START);		// blocktype
		stream.writeUInt32BE(18);						// blocksize
		stream.writeUInt16BE(0);						// namelength
		stream.writeUInt32(FourCC('R', 'G', 'B', ' ')); // colormode
		stream.writeFloatBE(scaled.r);
		stream.writeFloatBE(scaled.g);
		stream.writeFloatBE(scaled.b);
	}
	return true;
}

} // namespace voxel
