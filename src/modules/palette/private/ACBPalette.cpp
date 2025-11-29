/**
 * @file
 */

#include "ACBPalette.h"
#include "core/Color.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/String.h"
#include "palette/private/AdobeColorSpace.h"
#include <glm/common.hpp>

namespace palette {

#define wrapBool(x)                                                                                                    \
	if (!(x)) {                                                                                                        \
		Log::error("Failed to write to stream");                                                                       \
		return false;                                                                                                  \
	}

bool ACBPalette::save(const palette::ColorPalette &palette, const core::String &filename, io::SeekableWriteStream &stream) {
	wrapBool(stream.writeUInt32(FourCC('8', 'B', 'C', 'B')))
	wrapBool(stream.writeUInt16BE(1)) // version
	wrapBool(stream.writeUInt16BE(0)) // bookid
	wrapBool(stream.writeUInt32BE(palette.name().size()))
	wrapBool(stream.writeUTF16BE(palette.name()))
	wrapBool(stream.writeUInt32BE(0)) // prefix
	wrapBool(stream.writeUInt32BE(0)) // suffix
	wrapBool(stream.writeUInt32BE(0)) // description
	const int colorCount = palette.colorCount();
	wrapBool(stream.writeUInt16BE(colorCount))
	wrapBool(stream.writeUInt16BE(0)) // page size
	wrapBool(stream.writeUInt16BE(0)) // page selector offset
	wrapBool(stream.writeUInt16BE((uint16_t)adobe::ColorSpace::RGB))
	for (int i = 0; i < colorCount; ++i) {
		const core::String &name = palette.colorName(i);
		wrapBool(stream.writeUInt32BE(name.size()))
		wrapBool(stream.writeUTF16BE(name))
		uint8_t code[6] = {0};
		if (stream.write(code, 6) != 6) {
			Log::error("Failed to write to stream");
			return false;
		}
		const core::RGBA &color = palette.color(i);
		wrapBool(stream.writeUInt8(color.r))
		wrapBool(stream.writeUInt8(color.g))
		wrapBool(stream.writeUInt8(color.b))
	}
	return true;
}
#undef wrapBool

bool ACBPalette::load(const core::String &filename, io::SeekableReadStream &stream, palette::ColorPalette &colors) {
	uint32_t magic;
	if (stream.readUInt32(magic) == -1) {
		Log::error("ACBPalette: Failed to read magic");
		return false;
	}

	if (magic != FourCC('8', 'B', 'C', 'B')) {
		Log::error("ACBPalette: Invalid magic");
		return false;
	}

	uint16_t version;
	if (stream.readUInt16BE(version) == -1) {
		Log::error("ACBPalette: Failed to read version");
		return false;
	}
	if (version != 1u) {
		Log::error("Unsupported ACB version %u", version);
		return false;
	}

	uint16_t bookId;
	if (stream.readUInt16BE(bookId) == -1) {
		Log::error("ACBPalette: Failed to read bookId");
		return false;
	}
	Log::debug("ACBPalette: Book ID: %d", bookId);

	uint32_t len;

	core::String title;
	if (stream.readUInt32BE(len) == -1) {
		Log::error("ACBPalette: Failed to read title length");
		return false;
	}
	if (!stream.readUTF16BE(len, title)) {
		Log::error("ACBPalette: Failed to read title");
		return false;
	}
	Log::debug("ACBPalette: Title: %s", title.c_str());
	colors.setName(title);

	core::String prefix;
	if (stream.readUInt32BE(len) == -1) {
		Log::error("ACBPalette: Failed to read prefix length");
		return false;
	}
	if (!stream.readUTF16BE(len, prefix)) {
		Log::error("ACBPalette: Failed to read prefix");
		return false;
	}
	Log::debug("ACBPalette: Prefix: %s", prefix.c_str());

	core::String suffix;
	if (stream.readUInt32BE(len) == -1) {
		Log::error("ACBPalette: Failed to read suffix length");
		return false;
	}
	if (!stream.readUTF16BE(len, suffix)) {
		Log::error("ACBPalette: Failed to read suffix");
		return false;
	}
	Log::debug("ACBPalette: Suffix: %s", suffix.c_str());

	core::String description;
	if (stream.readUInt32BE(len) == -1) {
		Log::error("ACBPalette: Failed to read description length");
		return false;
	}
	if (!stream.readUTF16BE(len, description)) {
		Log::error("ACBPalette: Failed to read description");
		return false;
	}
	Log::debug("ACBPalette: Description: %s", description.c_str());

	uint16_t colorCount;
	if (stream.readUInt16BE(colorCount) == -1) {
		Log::error("ACBPalette: Failed to read color count");
		return false;
	}
	Log::debug("ACBPalette: Color count: %d", colorCount);

	uint16_t pageSize;
	if (stream.readUInt16BE(pageSize) == -1) {
		Log::error("ACBPalette: Failed to read page size");
		return false;
	}
	Log::debug("ACBPalette: Page size: %d", pageSize);

	uint16_t pageSelectorOffset;
	if (stream.readUInt16BE(pageSelectorOffset) == -1) {
		Log::error("ACBPalette: Failed to read page selector offset");
		return false;
	}
	Log::debug("ACBPalette: Page selector offset: %d", pageSelectorOffset);

	uint16_t colorSpace;
	if (stream.readUInt16BE(colorSpace) == -1) {
		Log::error("ACBPalette: Failed to read color space");
		return false;
	}
	Log::debug("ACBPalette: Color space: %d", colorSpace);

	const adobe::ColorSpace space = (adobe::ColorSpace)colorSpace;
	colors.reserve(colorCount);
	for (uint16_t i = 0; i < colorCount; ++i) {
		core::String colorName;
		if (stream.readUInt32BE(len) == -1) {
			Log::error("ACBPalette: Failed to read colorName length");
			return false;
		}
		if (!stream.readUTF16BE(len, colorName)) {
			Log::error("ACBPalette: Failed to read color name with len %d", len);
			return false;
		}
		core::String code;
		if (!stream.readString(6, code, false)) {
			Log::error("ACBPalette: Failed to read color code");
			return false;
		}
		if (space == adobe::ColorSpace::RGB) {
			uint8_t rgb[3];
			if (stream.read(rgb, 3) == -1) {
				Log::error("ACBPalette: Failed to read RGB color");
				return false;
			}
			colors.add(core::RGBA{rgb[0], rgb[1], rgb[2], 255}, colorName);
		} else if (space == adobe::ColorSpace::CMYK) {
			uint8_t cmyk[4];
			if (stream.read(cmyk, sizeof(cmyk)) == -1) {
				Log::error("ACBPalette: Failed to read cielab color");
				return false;
			}
			// Convert ACB CMYK (0–255) to standard CMYK (0–100%)
			float C = (255 - cmyk[0]) / 2.55f;
			float M = (255 - cmyk[1]) / 2.55f;
			float Y = (255 - cmyk[2]) / 2.55f;
			float K = (255 - cmyk[3]) / 2.55f;

			// Convert percentage to normalized values (0–1)
			C /= 100.0f;
			M /= 100.0f;
			Y /= 100.0f;
			K /= 100.0f;

			// Convert CMYK to RGB (0–255)
			const uint8_t r = (uint8_t)(glm::round(255 * (1 - C) * (1 - K)));
			const uint8_t g = (uint8_t)(glm::round(255 * (1 - M) * (1 - K)));
			const uint8_t b = (uint8_t)(glm::round(255 * (1 - Y) * (1 - K)));
			colors.add(core::RGBA{r, g, b, 255}, colorName);
		} else if (space == adobe::ColorSpace::Lab) {
			uint8_t lab[3];
			if (stream.read(lab, sizeof(lab)) == -1) {
				Log::error("ACBPalette: Failed to read cielab color");
				return false;
			}
			// Convert ACB Lab (0–255) to standard Lab (0–100)
			float L = lab[0] / 2.55f;
			float a = lab[1] / 2.55f - 128.0f;
			float b = lab[2] / 2.55f - 128.0f;
			colors.add(core::Color::fromCIELab(glm::vec4(L, a, b, 1.0f)), colorName);
		} else {
			// TODO: PALETTE: support grayscale...
			Log::error("Unsupported color space %d", colorSpace);
			return false;
		}
	}
	return colorCount > 0;
}

} // namespace palette
