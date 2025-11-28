/**
 * @file
 */

#include "QBCLPalette.h"

namespace palette {

bool QBCLPalette::load(const core::String &filename, io::SeekableReadStream &stream, palette::ColorPalette &palette) {
	palette.setName(filename);

	core::String name;
	stream.readPascalStringUInt8(name);
	core::String version;
	stream.readPascalStringUInt8(version);

	uint8_t unknown1;
	stream.readUInt8(unknown1);
	uint8_t unknown2;
	stream.readUInt8(unknown2);
	uint8_t unknown3;
	stream.readUInt8(unknown3);
	uint8_t unknown4;
	stream.readUInt8(unknown4);
	uint8_t colorformat;
	stream.readUInt8(colorformat);
	uint8_t unknown6;
	stream.readUInt8(unknown6);
	uint8_t unknown7;
	stream.readUInt8(unknown7);

	struct entry {
		core::RGBA palColor = 0;
		bool valid = false;
		core::RGBA color1 = 0;
		core::RGBA color2 = 0;
	};

	int colorCount = 0;
	static_assert(PaletteMaxColors == 256, "expected 256 colors in QBCL format");
	for (int i = 0; i < PaletteMaxColors; ++i) {
		entry e;
		stream.readUInt8(e.palColor.a);
		stream.readUInt8(e.palColor.r);
		stream.readUInt8(e.palColor.g);
		stream.readUInt8(e.palColor.b);

		e.valid = stream.readBool();
		stream.readUInt32(e.color1.rgba);
		stream.readUInt32(e.color2.rgba);
		if (!e.valid) {
			continue;
		}

		// ignore alpha here
		palette.setColor(colorCount, core::RGBA(e.palColor.r, e.palColor.g, e.palColor.b));
		colorCount++;
	}
	palette.setSize(colorCount);
	return colorCount > 0;
}

bool QBCLPalette::save(const palette::ColorPalette &palette, const core::String &filename, io::SeekableWriteStream &stream) {
	return false;
}

} // namespace voxel
