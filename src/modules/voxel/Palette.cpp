/**
 * @file
 */

#include "Palette.h"
#include "app/App.h"
#include "core/Color.h"
#include "core/Log.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "image/Image.h"
#include "io/File.h"
#include "io/Filesystem.h"
#include "math/Math.h"
#include <SDL_endian.h>
#include <float.h>

namespace voxel {

bool Palette::addColorToPalette(uint32_t rgba, bool skipSimilar) {
	for (int i = 0; i < colorCount; ++i) {
		if (colors[i] == rgba) {
			return false;
		}
	}
	static constexpr float MaxThreshold = 0.00014f;
	if (skipSimilar) {
		for (int i = 0; i < colorCount; ++i) {
			const float dist = core::Color::getDistance(colors[i], rgba);
			if (dist < MaxThreshold) {
				return false;
			}
		}
	}

	if (colorCount < PaletteMaxColors) {
		colors[colorCount++] = rgba;
		return true;
	}

	// now we are looking for the color in the existing palette entries that is most similar
	// to other entries in the palette. If this entry is than above a certain threshold, we
	// will replace that color with the new rgba value
	int bestIndex = -1;
	float bestColorDistance = FLT_MAX;
	for (int i = 0; i < colorCount; ++i) {
		float colorDistance;
		const int closestColorIdx = getClosestMatch(colors[i], &colorDistance, i);
		if (colorDistance < bestColorDistance) {
			bestColorDistance = colorDistance;
			bestIndex = closestColorIdx;
		}
	}
	if (bestIndex != -1) {
		const float dist = core::Color::getDistance(colors[bestIndex], rgba);
		if (dist > MaxThreshold) {
			colors[bestIndex] = rgba;
			return true;
		}
	}
	return false;
}

int Palette::getClosestMatch(const glm::vec4& color, float *distance, int skip) const {
	if (size() == 0) {
		return -1;
	}

	float minDistance = FLT_MAX;
	int minIndex = -1;

	float hue;
	float saturation;
	float brightness;
	core::Color::getHSB(color, hue, saturation, brightness);

	for (int i = 0; i < colorCount; ++i) {
		if (i == skip) {
			continue;
		}
		const float val = core::Color::getDistance(colors[i], hue, saturation, brightness);
		if (val < minDistance) {
			minDistance = val;
			minIndex = (int)i;
		}
	}
	if (distance) {
		*distance = minDistance;
	}
	return minIndex;
}

int Palette::getClosestMatch(const uint32_t rgba, float *distance, int skip) const {
	for (int i = 0; i < colorCount; ++i) {
		if (i == skip) {
			continue;
		}
		if (colors[i] == rgba) {
			return (int)i;
		}
	}
	return getClosestMatch(core::Color::fromRGBA(rgba), distance, skip);
}

bool Palette::save(const char *name) {
	if (name == nullptr || name[0] == '\0') {
		if (_paletteFilename.empty()) {
			return false;
		}
		name = _paletteFilename.c_str();
	}
	image::Image img(name);
	Log::info("Save palette to %s", name);
	img.loadRGBA((const uint8_t *)colors, sizeof(colors), lengthof(colors), 1);
	if (!img.writePng()) {
		Log::warn("Failed to write the palette file '%s'", name);
		return false;
	}
	return true;
}

bool Palette::load(const uint8_t *rgbaBuf, size_t bufsize) {
	if (bufsize % 4 != 0) {
		Log::warn("Buf size doesn't match expectation: %i", (int)bufsize);
	}
	int ncolors = (int)bufsize / 4;
	if (ncolors <= 0) {
		Log::error("Buffer is not big enough: %i bytes", (int)bufsize);
		return false;
	}
	if (ncolors > PaletteMaxColors) {
		Log::warn("Too many colors given for palette.");
	}
	ncolors = core_min(ncolors, PaletteMaxColors);
	image::ImagePtr img = image::createEmptyImage("**palette**");
	if (!img->loadRGBA(rgbaBuf, ncolors * 4, ncolors, 1)) {
		return false;
	}
	_paletteFilename = "";
	return load(img);
}

bool Palette::load(const image::ImagePtr &img) {
	if (img->depth() != 4) {
		Log::warn("Palette image has invalid depth (exepected: 4bpp, got %i)", img->depth());
		return false;
	}
	core_memset(glowColors, 0, sizeof(glowColors));
	int ncolors = img->width();
	if (ncolors > PaletteMaxColors) {
		ncolors = PaletteMaxColors;
		Log::warn("Palette image has invalid dimensions - we need max 256x1(depth: 4)");
	}
	colorCount = ncolors;
	for (int i = 0; i < colorCount; ++i) {
		colors[i] = *(uint32_t *)img->at(i, 0);
	}
	for (int i = colorCount; i < PaletteMaxColors; ++i) {
		colors[i] = core::Color::getRGBA(0, 0, 0);
	}
	_dirty = true;
	Log::debug("Set up %i material colors", colorCount);
	return true;
}

bool Palette::load(const char *paletteName) {
	const io::FilesystemPtr &filesystem = io::filesystem();
	io::FilePtr paletteFile = filesystem->open(paletteName);
	if (!paletteFile->validHandle()) {
		paletteFile = filesystem->open(core::string::format("palette-%s.png", paletteName));
	}
	if (!paletteFile->validHandle()) {
		Log::error("Failed to load palette file %s", paletteName);
		return false;
	}
	const image::ImagePtr &img = image::loadImage(paletteFile, false);
	if (!img->isLoaded()) {
		Log::error("Failed to load image %s", paletteFile->name().c_str());
		return false;
	}
	const io::FilePtr &luaFile = filesystem->open(core::string::format("palette-%s.lua", paletteName));
	if (luaFile->validHandle()) {
		lua = luaFile->load();
	} else {
		lua = "";
	}
	_paletteFilename = paletteFile->name();
	return load(img);
}

bool Palette::minecraft() {
	uint32_t palette[PaletteMaxColors] = {
		0xff000000, 0xff7d7d7d, 0xff4cb376, 0xff436086, 0xff7a7a7a, 0xff4e7f9c, 0xff256647, 0xff535353, 0xffdcaf70,
		0xffdcaf70, 0xff135bcf, 0xff125ad4, 0xffa0d3db, 0xff7a7c7e, 0xff7c8b8f, 0xff7e8287, 0xff737373, 0xff315166,
		0xff31b245, 0xff54c3c2, 0xfff4f0da, 0xff867066, 0xff894326, 0xff838383, 0xff9fd3dc, 0xff324364, 0xff3634b4,
		0xff23c7f6, 0xff7c7c7c, 0xff77bf8e, 0xffdcdcdc, 0xff296595, 0xff194f7b, 0xff538ba5, 0xff5e96bd, 0xffdddddd,
		0xffe5e5e5, 0xff00ffff, 0xff0d00da, 0xff415778, 0xff0d0fe1, 0xff4eecf9, 0xffdbdbdb, 0xffa1a1a1, 0xffa6a6a6,
		0xff0630bc, 0xff0026af, 0xff39586b, 0xff658765, 0xff1d1214, 0xff00ffff, 0xff005fde, 0xff31271a, 0xff4e87a6,
		0xff2a74a4, 0xff0000ff, 0xff8f8c81, 0xffd5db61, 0xff2e5088, 0xff17593c, 0xff335682, 0xff676767, 0xff00b9ff,
		0xff5b9ab8, 0xff387394, 0xff345f79, 0xff5190b6, 0xff6a6a6a, 0xff5b9ab8, 0xff40596a, 0xff7a7a7a, 0xffc2c2c2,
		0xff65a0c9, 0xff6b6b84, 0xff2d2ddd, 0xff000066, 0xff0061ff, 0xff848484, 0xfff1f1df, 0xffffad7d, 0xfffbfbef,
		0xff1d830f, 0xffb0a49e, 0xff65c094, 0xff3b5985, 0xff42748d, 0xff1b8ce3, 0xff34366f, 0xff334054, 0xff45768f,
		0xffbf0a57, 0xff2198f1, 0xffffffec, 0xffb2b2b2, 0xffb2b2b2, 0xffffffff, 0xff2d5d7e, 0xff7c7c7c, 0xff7a7a7a,
		0xff7cafcf, 0xff78aaca, 0xff6a6c6d, 0xfff4efd3, 0xff28bdc4, 0xff69dd92, 0xff53ae73, 0xff0c5120, 0xff5287a5,
		0xff2a4094, 0xff7a7a7a, 0xff75718a, 0xff767676, 0xff1a162c, 0xff1a162c, 0xff1a162c, 0xff2d28a6, 0xffb1c454,
		0xff51677c, 0xff494949, 0xff343434, 0xffd18934, 0xffa5dfdd, 0xff0f090c, 0xff316397, 0xff42a0e3, 0xff4d84a1,
		0xff49859e, 0xff1f71dd, 0xffa8e2e7, 0xff74806d, 0xff3c3a2a, 0xff7c7c7c, 0xff5a5a5a, 0xff75d951, 0xff345e81,
		0xff84c0ce, 0xff455f88, 0xff868b8e, 0xffd7dd74, 0xff595959, 0xff334176, 0xff008c0a, 0xff17a404, 0xff5992b3,
		0xffb0b0b0, 0xff434347, 0xff1d6b9e, 0xff70fdfe, 0xffe5e5e5, 0xff4c4a4b, 0xffbdc6bf, 0xffddedfb, 0xff091bab,
		0xff4f547d, 0xff717171, 0xffdfe6ea, 0xffe3e8eb, 0xff41819b, 0xff747474, 0xffa1b2d1, 0xfff6f6f6, 0xff878787,
		0xff395ab0, 0xff325cac, 0xff152c47, 0xff65c878, 0xff3534df, 0xffc7c7c7, 0xffa5af72, 0xffbec7ac, 0xff9fd3dc,
		0xffcacaca, 0xff425c96, 0xff121212, 0xfff4bfa2, 0xff1474cf, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xff1d56ac,
		0xff1d57ae, 0xff1d57ae, 0xff1d57ae, 0xff243c50, 0xff8dcddd, 0xff4d7aaf, 0xff0e2034, 0xff366bcf, 0xff355d7e,
		0xff7bb8c7, 0xff5f86bb, 0xff1e2e3f, 0xff3a6bc5, 0xff30536e, 0xffe0f3f7, 0xff5077a9, 0xff2955aa, 0xff21374e,
		0xffcdc5dc, 0xff603b60, 0xff856785, 0xffa679a6, 0xffaa7eaa, 0xffa879a8, 0xffa879a8, 0xffa879a8, 0xffaae6e1,
		0xffaae6e1, 0xff457d98, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0,
		0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0,
		0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0,
		0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0,
		0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0,
		0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xff242132};

	uint32_t *swapBuf = palette;
	for (int i = 0; i < lengthof(palette); ++i) {
		swapBuf[i] = SDL_SwapLE32(swapBuf[i]);
	}

	return load((const uint8_t *)palette, sizeof(palette));
}

bool Palette::magicaVoxel() {
	uint32_t palette[PaletteMaxColors] = {
		0x00000000, 0xffffffff, 0xffccffff, 0xff99ffff, 0xff66ffff, 0xff33ffff, 0xff00ffff, 0xffffccff, 0xffccccff,
		0xff99ccff, 0xff66ccff, 0xff33ccff, 0xff00ccff, 0xffff99ff, 0xffcc99ff, 0xff9999ff, 0xff6699ff, 0xff3399ff,
		0xff0099ff, 0xffff66ff, 0xffcc66ff, 0xff9966ff, 0xff6666ff, 0xff3366ff, 0xff0066ff, 0xffff33ff, 0xffcc33ff,
		0xff9933ff, 0xff6633ff, 0xff3333ff, 0xff0033ff, 0xffff00ff, 0xffcc00ff, 0xff9900ff, 0xff6600ff, 0xff3300ff,
		0xff0000ff, 0xffffffcc, 0xffccffcc, 0xff99ffcc, 0xff66ffcc, 0xff33ffcc, 0xff00ffcc, 0xffffcccc, 0xffcccccc,
		0xff99cccc, 0xff66cccc, 0xff33cccc, 0xff00cccc, 0xffff99cc, 0xffcc99cc, 0xff9999cc, 0xff6699cc, 0xff3399cc,
		0xff0099cc, 0xffff66cc, 0xffcc66cc, 0xff9966cc, 0xff6666cc, 0xff3366cc, 0xff0066cc, 0xffff33cc, 0xffcc33cc,
		0xff9933cc, 0xff6633cc, 0xff3333cc, 0xff0033cc, 0xffff00cc, 0xffcc00cc, 0xff9900cc, 0xff6600cc, 0xff3300cc,
		0xff0000cc, 0xffffff99, 0xffccff99, 0xff99ff99, 0xff66ff99, 0xff33ff99, 0xff00ff99, 0xffffcc99, 0xffcccc99,
		0xff99cc99, 0xff66cc99, 0xff33cc99, 0xff00cc99, 0xffff9999, 0xffcc9999, 0xff999999, 0xff669999, 0xff339999,
		0xff009999, 0xffff6699, 0xffcc6699, 0xff996699, 0xff666699, 0xff336699, 0xff006699, 0xffff3399, 0xffcc3399,
		0xff993399, 0xff663399, 0xff333399, 0xff003399, 0xffff0099, 0xffcc0099, 0xff990099, 0xff660099, 0xff330099,
		0xff000099, 0xffffff66, 0xffccff66, 0xff99ff66, 0xff66ff66, 0xff33ff66, 0xff00ff66, 0xffffcc66, 0xffcccc66,
		0xff99cc66, 0xff66cc66, 0xff33cc66, 0xff00cc66, 0xffff9966, 0xffcc9966, 0xff999966, 0xff669966, 0xff339966,
		0xff009966, 0xffff6666, 0xffcc6666, 0xff996666, 0xff666666, 0xff336666, 0xff006666, 0xffff3366, 0xffcc3366,
		0xff993366, 0xff663366, 0xff333366, 0xff003366, 0xffff0066, 0xffcc0066, 0xff990066, 0xff660066, 0xff330066,
		0xff000066, 0xffffff33, 0xffccff33, 0xff99ff33, 0xff66ff33, 0xff33ff33, 0xff00ff33, 0xffffcc33, 0xffcccc33,
		0xff99cc33, 0xff66cc33, 0xff33cc33, 0xff00cc33, 0xffff9933, 0xffcc9933, 0xff999933, 0xff669933, 0xff339933,
		0xff009933, 0xffff6633, 0xffcc6633, 0xff996633, 0xff666633, 0xff336633, 0xff006633, 0xffff3333, 0xffcc3333,
		0xff993333, 0xff663333, 0xff333333, 0xff003333, 0xffff0033, 0xffcc0033, 0xff990033, 0xff660033, 0xff330033,
		0xff000033, 0xffffff00, 0xffccff00, 0xff99ff00, 0xff66ff00, 0xff33ff00, 0xff00ff00, 0xffffcc00, 0xffcccc00,
		0xff99cc00, 0xff66cc00, 0xff33cc00, 0xff00cc00, 0xffff9900, 0xffcc9900, 0xff999900, 0xff669900, 0xff339900,
		0xff009900, 0xffff6600, 0xffcc6600, 0xff996600, 0xff666600, 0xff336600, 0xff006600, 0xffff3300, 0xffcc3300,
		0xff993300, 0xff663300, 0xff333300, 0xff003300, 0xffff0000, 0xffcc0000, 0xff990000, 0xff660000, 0xff330000,
		0xff0000ee, 0xff0000dd, 0xff0000bb, 0xff0000aa, 0xff000088, 0xff000077, 0xff000055, 0xff000044, 0xff000022,
		0xff000011, 0xff00ee00, 0xff00dd00, 0xff00bb00, 0xff00aa00, 0xff008800, 0xff007700, 0xff005500, 0xff004400,
		0xff002200, 0xff001100, 0xffee0000, 0xffdd0000, 0xffbb0000, 0xffaa0000, 0xff880000, 0xff770000, 0xff550000,
		0xff440000, 0xff220000, 0xff110000, 0xffeeeeee, 0xffdddddd, 0xffbbbbbb, 0xffaaaaaa, 0xff888888, 0xff777777,
		0xff555555, 0xff444444, 0xff222222, 0xff111111};

	uint32_t *swapBuf = palette;
	for (int i = 0; i < lengthof(palette); ++i) {
		swapBuf[i] = SDL_SwapLE32(swapBuf[i]);
	}
	return load((const uint8_t *)palette, sizeof(palette));
}

core::String Palette::extractPaletteName(const core::String &file) {
	if (!core::string::startsWith(file, "palette-")) {
		return "";
	}
	const core::String &nameWithExtension = file.substr(8);
	const size_t extPos = nameWithExtension.rfind('.');
	if (extPos != core::String::npos) {
		return nameWithExtension.substr(0, extPos);
	}
	return nameWithExtension;
}

bool Palette::createPalette(const image::ImagePtr &image, voxel::Palette &palette) {
	if (!image || !image->isLoaded()) {
		return false;
	}
	const int colors = (int)PaletteMaxColors;
	const int imageWidth = image->width();
	const int imageHeight = image->height();
	Log::debug("Create palette for image: %s", image->name().c_str());
	uint16_t paletteIndex = 0;
	uint32_t empty = core::Color::getRGBA(core::Color::White);
	palette.colors[paletteIndex++] = empty;
	palette._dirty = true;
	for (int x = 0; x < imageWidth; ++x) {
		for (int y = 0; y < imageHeight; ++y) {
			const uint8_t *data = image->at(x, y);
			palette.addColorToPalette(*(uint32_t*)data);
		}
	}
	for (int i = paletteIndex; i < colors; ++i) {
		palette.colors[i] = empty;
	}
	return true;
}

bool Palette::hasGlow(uint8_t idx) const {
	return glowColors[idx] != 0;
}

void Palette::removeGlow(uint8_t idx) {
	glowColors[idx] = 0;
	_dirty = true;
}

void Palette::setGlow(uint8_t idx, float factor) {
	// TODO: handle factor
	glowColors[idx] = colors[idx];
	_dirty = true;
}

void Palette::toVec4f(core::DynamicArray<glm::vec4> &vec4f) const {
	vec4f.reserve(PaletteMaxColors);
	for (int i = 0; i < colorCount; ++i) {
		vec4f.push_back(core::Color::fromRGBA(colors[i]));
	}
	for (int i = colorCount; i < PaletteMaxColors; ++i) {
		vec4f.emplace_back(0.0f);
	}
}

void Palette::glowToVec4f(core::DynamicArray<glm::vec4> &vec4f) const {
	vec4f.reserve(PaletteMaxColors);
	for (int i = 0; i < colorCount; ++i) {
		vec4f.push_back(core::Color::fromRGBA(glowColors[i]));
	}
	for (int i = colorCount; i < PaletteMaxColors; ++i) {
		vec4f.emplace_back(0.0f);
	}
}

bool Palette::convertImageToPalettePng(const image::ImagePtr &image, const char *paletteFile) {
	if (!image || !image->isLoaded() || paletteFile == nullptr) {
		return false;
	}
	Palette palette;
	if (!createPalette(image, palette)) {
		return false;
	}
	const image::ImagePtr &paletteImg = image::createEmptyImage("**palette**");
	return paletteImg->writePng(paletteFile, (const uint8_t *)palette.colors, palette.colorCount, 1, 4);
}

} // namespace voxel
