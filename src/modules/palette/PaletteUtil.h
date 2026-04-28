/**
 * @file
 */

#pragma once

#include "color/ColorUtil.h"
#include "io/StdoutWriteStream.h"
#include "io/Stream.h"
#include "palette/ColorPalette.h"
#include "palette/Palette.h"

namespace palette {

palette::ColorPalette toColorPalette(const palette::Palette &palette);
palette::Palette toPalette(const palette::ColorPalette &colorPalette);
/**
 * @brief Like toPalette(), but quantizes to at most @c targetColors colors when the palette exceeds
 * PaletteMaxColors. If @c targetColors is <= 0, the full PaletteMaxColors limit is used.
 */
palette::Palette toPaletteQuantized(const palette::ColorPalette &colorPalette, int targetColors);

template<class PaletteType>
static void writeJson(io::WriteStream &out, const PaletteType &palette) {
	out.writeString("{", false);
	out.writeString("\"name\":\"" + palette.name() + "\",", false);
	out.writeString("\"colors\":[", false);
	for (int i = 0; i < (int)palette.size(); ++i) {
		const color::RGBA color = palette.color(i);
		out.writeString("{", false);
		out.writeStringFormat(false, "\"r\":%u,\"g\":%u,\"b\":%u,\"a\":%u", color.r, color.g, color.b, color.a);
		if (!palette.colorName(i).empty()) {
			out.writeStringFormat(false, ",\"name\":\"%s\"", palette.colorName(i).c_str());
		}

		float h, s, b;
		color::getHSB(color, h, s, b);
		out.writeStringFormat(false, ",\"hue\":%f,\"saturation\":%f,\"brightness\":%f", h, s, b);

		const palette::Material &mat = palette.material(i);
		int n = palette::MaterialProperty::MaterialMetal;
		const int maxN = palette::MaterialProperty::MaterialMax;
		out.writeString(",\"material\":{", false);
		int matPrinted = 0;
		for (; n < maxN; ++n) {
			const palette::MaterialProperty propEnum = (palette::MaterialProperty)n;
			if (!mat.has(propEnum)) {
				continue;
			}
			if (matPrinted > 0) {
				out.writeString(",", false);
			}
			out.writeStringFormat(false, "\"%s\":%f", palette::MaterialPropertyName(propEnum), mat.value(propEnum));
			matPrinted++;
		}
		out.writeString("}", false); // material
		out.writeString("}", false); // color

		if (i != (int)palette.size() - 1) {
			out.writeString(",", false);
		}
	}
	out.writeString("]", false);
	out.writeString("}\n", false);
}

template<class PaletteType>
void printJson(const PaletteType &palette) {
	io::StdoutWriteStream out;
	palette::writeJson(out, palette);
}

template<class PaletteType>
core::String toString(const PaletteType &palette, bool colorAsHex = false) {
	if (palette.colorCount() == 0) {
		return "no colors";
	}
	core::String palStr;
	core::String line;
	for (int i = 0; i < palette.colorCount(); ++i) {
		if (i % 16 == 0 && !line.empty()) {
			palStr.append(core::String::format("%03i %s\n", i - 16, line.c_str()));
			line = "";
		}
		const core::String c = color::print(palette.color(i), colorAsHex);
		line += c;
	}
	if (!line.empty()) {
		palStr.append(core::String::format("%03i %s\n", (palette.colorCount() - 1) / 16 * 16, line.c_str()));
	}
	return palStr;
}

template<class PaletteType>
void writeHex(io::WriteStream &out, const PaletteType &palette) {
	for (int i = 0; i < palette.colorCount(); ++i) {
		const color::RGBA color = palette.color(i);
		out.writeStringFormat(false, "0x%02x%02x%02x%02x", color.r, color.g, color.b, color.a);
		if (i != palette.colorCount() - 1) {
			out.writeString(", ", false);
		}
	}
	out.writeString("\n", false);
}

template<class PaletteType>
void printHexPalette(const PaletteType &palette) {
	io::StdoutWriteStream out;
	writeHex(out, palette);
}

} // namespace palette
