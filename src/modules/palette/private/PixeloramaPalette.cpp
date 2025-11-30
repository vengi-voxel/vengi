/**
 * @file
 */

#include "PixeloramaPalette.h"
#include "SDL_stdinc.h"
#include "color/Color.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "glm/fwd.hpp"
#include "json/JSON.h"

namespace palette {

bool PixeloramaPalette::load(const core::String &filename, io::SeekableReadStream &stream, palette::ColorPalette &palette) {
	core::String jsonStr;
	if (!stream.readString(stream.size(), jsonStr)) {
		Log::error("Failed to read json file");
		return false;
	}

	nlohmann::json json = nlohmann::json::parse(jsonStr, nullptr, false, true);
	if (!json.contains("colors")) {
		Log::error("No colors found in json file");
		return false;
	}
	int maxIdx = 0;
	for (const auto &color : json["colors"]) {
		const std::string &colorString = color["color"];
		const int idx = color["index"];
		glm::vec4 c{0.0f, 0.0f, 0.0f, 1.0f};
		if (SDL_sscanf(colorString.c_str(), "(%f, %f, %f, %f)", &c.r, &c.g, &c.b, &c.a) != 4) {
			Log::warn("Unexpected color format: '%s'", colorString.c_str());
		}
		palette.setColor(idx, color::Color::getRGBA(c));
		maxIdx = core_max(idx, maxIdx);
	}
	if (json.contains("comment")) {
		const std::string &comment = json["comment"];
		palette.setName(comment.c_str());
	}
	palette.setSize(maxIdx + 1);
	return true;
}

bool PixeloramaPalette::save(const palette::ColorPalette &palette, const core::String &filename,
							 io::SeekableWriteStream &stream) {
	stream.writeString("{\n \"colors\": [\n", false);
	for (int i = 0; i < palette.colorCount(); ++i) {
		const color::RGBA &rgba = palette.color(i);
		const glm::vec4 &color = color::Color::fromRGBA(rgba);
		stream.writeStringFormat(false, "  {\n   \"color\": \"(%f, %f, %f, %f)\",\n   \"index\": %d\n  }", color.r,
								 color.g, color.b, color.a, i);
		if (i + 1 < palette.colorCount()) {
			stream.writeString(",\n", false);
		} else {
			stream.writeString("\n", false);
		}
	}
	stream.writeString(" ],\n", false);
	stream.writeStringFormat(false, " \"comment\": \"%s\",\n", palette.name().c_str());
	stream.writeString(" \"height\": 1,\n", false);
	return stream.writeStringFormat(false, " \"width\": %d\n}\n", palette.colorCount());
}

} // namespace palette
