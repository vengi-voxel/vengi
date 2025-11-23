/**
 * @file
 */

#include "JASCPalette.h"
#include "core/Log.h"

namespace palette {

bool JASCPalette::load(const core::String &filename, io::SeekableReadStream &stream, palette::Palette &palette) {
	char line[512];
	if (!stream.readLine(sizeof(line), line)) {
		Log::error("Failed to read first line of JASC palette file %s", filename.c_str());
		return false;
	}
	if (strcmp(line, "JASC-PAL") != 0) {
		Log::error("Invalid JASC palette file %s", filename.c_str());
		return false;
	}
	if (!stream.readLine(sizeof(line), line)) {
		Log::error("Failed to read version line of JASC palette file %s", filename.c_str());
		return false;
	}
	if (strcmp(line, "0100") != 0) {
		Log::error("Invalid JASC version in %s: %s", filename.c_str(), line);
		return false;
	}
	if (!stream.readLine(sizeof(line), line)) {
		Log::error("Failed to read color count line of JASC palette file %s", filename.c_str());
		return false;
	}
	int colorCount = 0;
	while (stream.readLine(sizeof(line), line)) {
		int r, g, b;
		if (SDL_sscanf(line, "%i %i %i", &r, &g, &b) != 3) {
			Log::error("Failed to parse JASC color line '%s'", line);
			continue;
		}
		if (colorCount >= PaletteMaxColors) {
			Log::warn("Not all colors were loaded");
			break;
		}
		palette.setColor(colorCount, core::RGBA(r, g, b));
		++colorCount;
	}
	palette.setSize(colorCount);
	return colorCount > 0;
}

bool JASCPalette::save(const palette::Palette &palette, const core::String &filename, io::SeekableWriteStream &stream) {
	stream.writeLine("JASC-PAL");
	stream.writeLine("0100");
	stream.writeLine(core::String::format("%i", (int)palette.size()));
	for (size_t i = 0; i < palette.size(); ++i) {
		const core::RGBA &color = palette.color(i);
		stream.writeLine(core::String::format("%i %i %i", color.r, color.g, color.b));
	}
	return true;
}

} // namespace voxel
