/**
 * @file
 */

#include "Palette.h"
#include "core/StandardLib.h"
#include "app/App.h"
#include "core/Color.h"
#include "core/Log.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/ArrayLength.h"
#include "core/collection/Buffer.h"
#include "core/RGBA.h"
#include "image/Image.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "io/MemoryReadStream.h"
#include "math/Math.h"
#include "engine-config.h"

#include <SDL_endian.h>
#include <float.h>

namespace voxel {

void Palette::markDirty() {
	_dirty = true;
	_hash._hashColors[0] = core::hash(colors, sizeof(colors));
	_hash._hashColors[1] = core::hash(glowColors, sizeof(glowColors));
}

void Palette::quantize(const core::RGBA *inputColors, const size_t inputColorCount) {
	colorCount = core::Color::quantize(colors, lengthof(colors), inputColors, inputColorCount);
}

bool Palette::addColorToPalette(core::RGBA rgba, bool skipSimilar) {
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
		float colorDistance = 0.0f;
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

int Palette::getClosestMatch(const core::RGBA rgba, float *distance, int skip) const {
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

bool Palette::save(const char *name) const {
	if (name == nullptr || name[0] == '\0') {
		if (_paletteFilename.empty()) {
			return false;
		}
		name = _paletteFilename.c_str();
	}
	Log::info("Save palette to %s", name);
	const core::String &extension = core::string::extractExtension(name);
	if (extension == "gpl") {
		if (!saveGimpPalette(name)) {
			Log::warn("Failed to write the gimp palette file '%s'", name);
			return false;
		}
		return true;
	}
	image::Image img(name);
	// must be voxel::PaletteMaxColors - otherwise the exporter uv coordinates must get adopted
	img.loadRGBA((const uint8_t *)colors, lengthof(colors), 1);
	if (!img.writePng()) {
		Log::warn("Failed to write the palette file '%s'", name);
		return false;
	}
	return true;
}

bool Palette::saveGlow(const char *name) const {
	if (name == nullptr || name[0] == '\0') {
		return false;
	}
	image::Image img(name);
	Log::info("Save glow palette colors to %s", name);
	// must be voxel::PaletteMaxColors - otherwise the exporter uv coordinates must get adopted
	img.loadRGBA((const uint8_t *)glowColors, lengthof(glowColors), 1);
	if (!img.writePng()) {
		Log::warn("Failed to write the glow palette colors file '%s'", name);
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
	if (!img->loadRGBA(rgbaBuf, ncolors, 1)) {
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
	if (img->width() * img->height() > PaletteMaxColors) {
		return createPalette(img, *this);
	}
	int ncolors = img->width();
	if (ncolors > PaletteMaxColors) {
		ncolors = PaletteMaxColors;
		Log::warn("Palette image has invalid dimensions - we need max 256x1(depth: 4)");
	}
	colorCount = ncolors;
	for (int i = 0; i < colorCount; ++i) {
		colors[i] = img->colorAt(i, 0);
	}
	for (int i = colorCount; i < PaletteMaxColors; ++i) {
		colors[i] = core::Color::getRGBA(0, 0, 0);
	}
	markDirty();
	Log::debug("Set up %i material colors", colorCount);
	return true;
}

bool Palette::load(const char *paletteName) {
	if (SDL_strcmp(paletteName, builtIn[0]) == 0) {
		return nippon();
	} else if (SDL_strcmp(paletteName, builtIn[1]) == 0) {
		return minecraft();
	} else if (SDL_strcmp(paletteName, builtIn[2]) == 0) {
		return magicaVoxel();
	} else if (SDL_strcmp(paletteName, builtIn[3]) == 0) {
		return quake1();
	}

	const io::FilesystemPtr &filesystem = io::filesystem();
	io::FilePtr paletteFile = filesystem->open(paletteName);
	if (!paletteFile->validHandle()) {
		paletteFile = filesystem->open(core::string::format("palette-%s.png", paletteName));
	}
	if (!paletteFile->validHandle()) {
		Log::error("Failed to load palette file %s", paletteName);
		return false;
	}
	if (paletteFile->extension() == "gpl") {
		return loadGimpPalette(paletteName);
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

bool Palette::loadGimpPalette(const char *filename) {
	const io::FilesystemPtr &filesystem = io::filesystem();
	io::FilePtr paletteFile = filesystem->open(filename);
	if (!paletteFile->validHandle()) {
		Log::error("Failed to load gimp palette file %s", filename);
		return false;
	}
	const core::String &gpl = paletteFile->load();
	io::MemoryReadStream stream(gpl.c_str(), gpl.size());
	char line[2048];
	colorCount = 0;
	_paletteFilename = paletteFile->name();
	while (stream.readLine(sizeof(line), line)) {
		if (line[0] == '#') {
			continue;
		}
		if (strcmp("GIMP Palette", line) == 0) {
			continue;
		}
		if (strncmp("Name", line, 4) == 0) {
			continue;
		}

		int r, g, b;
		if (SDL_sscanf(line, "%i %i %i", &r, &g, &b) != 3) {
			Log::error("Failed to parse line '%s'", line);
			continue;
		}
		if (colorCount >= PaletteMaxColors) {
			Log::warn("Not all colors were loaded");
			break;
		}
		colors[colorCount++] = core::RGBA(r, g, b);
	}
	markDirty();
	return colorCount > 0;
}

bool Palette::saveGimpPalette(const char *filename, const char *name) const {
	const io::FilesystemPtr &filesystem = io::filesystem();
	io::FilePtr paletteFile = filesystem->open(filename, io::FileMode::SysWrite);
	if (!paletteFile->validHandle()) {
		Log::error("Failed to open file %s for saving the gimp palette", filename);
		return false;
	}
	io::FileStream stream(paletteFile);
	stream.writeString("GIMP Palette\n", false);
	stream.writeStringFormat(false, "Name: %s\n", name);
	stream.writeString("# Generated by vengi " PROJECT_VERSION " github.com/mgerhardy/vengi\n", false);
	for (int i = 0; i < colorCount; ++i) {
		stream.writeStringFormat(false, "%3i %3i %3i\tcolor index %i\n", colors[i].r, colors[i].g, colors[i].b, i);
	}
	return true;
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

bool Palette::quake1() {
	uint32_t palette[PaletteMaxColors] = {
		0x000000ff,0x0f0f0fff,0x1f1f1fff,0x2f2f2fff,0x3f3f3fff,0x4b4b4bff,0x5b5b5bff,0x6b6b6bff,
		0x7b7b7bff,0x8b8b8bff,0x9b9b9bff,0xabababff,0xbbbbbbff,0xcbcbcbff,0xdbdbdbff,0xebebebff,
		0x0f0b07ff,0x170f0bff,0x1f170bff,0x271b0fff,0x2f2313ff,0x372b17ff,0x3f2f17ff,0x4b371bff,
		0x533b1bff,0x5b431fff,0x634b1fff,0x6b531fff,0x73571fff,0x7b5f23ff,0x836723ff,0x8f6f23ff,
		0x0b0b0fff,0x13131bff,0x1b1b27ff,0x272733ff,0x2f2f3fff,0x37374bff,0x3f3f57ff,0x474767ff,
		0x4f4f73ff,0x5b5b7fff,0x63638bff,0x6b6b97ff,0x7373a3ff,0x7b7bafff,0x8383bbff,0x8b8bcbff,
		0x000000ff,0x070700ff,0x0b0b00ff,0x131300ff,0x1b1b00ff,0x232300ff,0x2b2b07ff,0x2f2f07ff,
		0x373707ff,0x3f3f07ff,0x474707ff,0x4b4b0bff,0x53530bff,0x5b5b0bff,0x63630bff,0x6b6b0fff,
		0x070000ff,0x0f0000ff,0x170000ff,0x1f0000ff,0x270000ff,0x2f0000ff,0x370000ff,0x3f0000ff,
		0x470000ff,0x4f0000ff,0x570000ff,0x5f0000ff,0x670000ff,0x6f0000ff,0x770000ff,0x7f0000ff,
		0x131300ff,0x1b1b00ff,0x232300ff,0x2f2b00ff,0x372f00ff,0x433700ff,0x4b3b07ff,0x574307ff,
		0x5f4707ff,0x6b4b0bff,0x77530fff,0x835713ff,0x8b5b13ff,0x975f1bff,0xa3631fff,0xaf6723ff,
		0x231307ff,0x2f170bff,0x3b1f0fff,0x4b2313ff,0x572b17ff,0x632f1fff,0x733723ff,0x7f3b2bff,
		0x8f4333ff,0x9f4f33ff,0xaf632fff,0xbf772fff,0xcf8f2bff,0xdfab27ff,0xefcb1fff,0xfff31bff,
		0x0b0700ff,0x1b1300ff,0x2b230fff,0x372b13ff,0x47331bff,0x533723ff,0x633f2bff,0x6f4733ff,
		0x7f533fff,0x8b5f47ff,0x9b6b53ff,0xa77b5fff,0xb7876bff,0xc3937bff,0xd3a38bff,0xe3b397ff,
		0xab8ba3ff,0x9f7f97ff,0x937387ff,0x8b677bff,0x7f5b6fff,0x775363ff,0x6b4b57ff,0x5f3f4bff,
		0x573743ff,0x4b2f37ff,0x43272fff,0x371f23ff,0x2b171bff,0x231313ff,0x170b0bff,0x0f0707ff,
		0xbb739fff,0xaf6b8fff,0xa35f83ff,0x975777ff,0x8b4f6bff,0x7f4b5fff,0x734353ff,0x6b3b4bff,
		0x5f333fff,0x532b37ff,0x47232bff,0x3b1f23ff,0x2f171bff,0x231313ff,0x170b0bff,0x0f0707ff,
		0xdbc3bbff,0xcbb3a7ff,0xbfa39bff,0xaf978bff,0xa3877bff,0x977b6fff,0x876f5fff,0x7b6353ff,
		0x6b5747ff,0x5f4b3bff,0x533f33ff,0x433327ff,0x372b1fff,0x271f17ff,0x1b130fff,0x0f0b07ff,
		0x6f837bff,0x677b6fff,0x5f7367ff,0x576b5fff,0x4f6357ff,0x475b4fff,0x3f5347ff,0x374b3fff,
		0x2f4337ff,0x2b3b2fff,0x233327ff,0x1f2b1fff,0x172317ff,0x0f1b13ff,0x0b130bff,0x070b07ff,
		0xfff31bff,0xefdf17ff,0xdbcb13ff,0xcbb70fff,0xbba70fff,0xab970bff,0x9b8307ff,0x8b7307ff,
		0x7b6307ff,0x6b5300ff,0x5b4700ff,0x4b3700ff,0x3b2b00ff,0x2b1f00ff,0x1b0f00ff,0x0b0700ff,
		0x0000ffff,0x0b0befff,0x1313dfff,0x1b1bcfff,0x2323bfff,0x2b2bafff,0x2f2f9fff,0x2f2f8fff,
		0x2f2f7fff,0x2f2f6fff,0x2f2f5fff,0x2b2b4fff,0x23233fff,0x1b1b2fff,0x13131fff,0x0b0b0fff,
		0x2b0000ff,0x3b0000ff,0x4b0700ff,0x5f0700ff,0x6f0f00ff,0x7f1707ff,0x931f07ff,0xa3270bff,
		0xb7330fff,0xc34b1bff,0xcf632bff,0xdb7f3bff,0xe3974fff,0xe7ab5fff,0xefbf77ff,0xf7d38bff,
		0xa77b3bff,0xb79b37ff,0xc7c337ff,0xe7e357ff,0x7fbfffff,0xabe7ffff,0xd7ffffff,0x670000ff,
		0x8b0000ff,0xb30000ff,0xd70000ff,0xff0000ff,0xfff393ff,0xfff7c7ff,0xffffffff,0x9f5b53ff
	};
	uint32_t *swapBuf = palette;
	for (int i = 0; i < lengthof(palette); ++i) {
		swapBuf[i] = SDL_SwapBE32(swapBuf[i]);
	}
	return load((const uint8_t *)palette, sizeof(palette));
}

bool Palette::nippon() {
	uint32_t palette[PaletteMaxColors] = {
		0xffffffff, 0xffb49fdc, 0xff8c6be1, 0xff4a358e, 0xffcdc3f8, 0xffb9a7f4, 0xff3c3664, 0xffaa96f5, 0xff5b49b5,
		0xff907ae8, 0xff6e5ad0, 0xff6d4ddb, 0xffe1dffe, 0xff7a7a9e, 0xff4c10d0, 0xff3a359f, 0xff451bcb, 0xffa9a9ee,
		0xff6667bf, 0xff3f4786, 0xff9396b1, 0xff777aeb, 0xff454a95, 0xff6063a9, 0xff4240cb, 0xff3a3bab, 0xffbbc4d7,
		0xff404890, 0xff384373, 0xff3a3ec7, 0xff364255, 0xff394699, 0xff8394f1, 0xff3444b5, 0xff7d88b9, 0xff677cf1,
		0xff3a4c88, 0xff1530e8, 0xff5554d7, 0xff4c5db5, 0xff364885, 0xff475ea3, 0xff3a54cc, 0xff324872, 0xff2f5cf7,
		0xff28406a, 0xff34509a, 0xff4362c4, 0xff3c5faf, 0xff6e96fb, 0xff384972, 0xff5771b4, 0xff718edb, 0xff1c5ef0,
		0xff4a78ed, 0xff5378ca, 0xff375cb3, 0xff2e3f56, 0xff6e91e3, 0xff3c5a8f, 0xff86a9f0, 0xff4b67a0, 0xff3c69c1,
		0xff6699fb, 0xff6d7a94, 0xff3663a3, 0xff6094e7, 0xff2c537d, 0xff5085c7, 0xff2a5f98, 0xff79a6e1, 0xff325b85,
		0xff4d9ffc, 0xff84baff, 0xff2a8be9, 0xff68a3e9, 0xff4478b1, 0xff2e6396, 0xff2c7aca, 0xff1b3443, 0xff8ab8ec,
		0xff2b5578, 0xff3677b0, 0xff497296, 0xff3b94e2, 0xff2d80c7, 0xff236e9b, 0xff2f556e, 0xff71b4eb, 0xff8eb9d7,
		0xff3a6682, 0xff558eb6, 0xff779fbc, 0xff336687, 0xff268ac1, 0xff1bb1ff, 0xff2698d1, 0xff2da5dd, 0xff3398c9,
		0xff45bff9, 0xff79b8dc, 0xff3291ba, 0xff47b6e8, 0xff42c2f7, 0xff466c7d, 0xffa6c9da, 0xff89d6fa, 0xff42abd9,
		0xff55c5f6, 0xff08c4ff, 0xff24bbef, 0xff5fadca, 0xff2a748d, 0xff82a5b4, 0xff6c7f87, 0xff557d89, 0xff3e6774,
		0xff378ca2, 0xff24606c, 0xff357886, 0xff2c5962, 0xff4ccde9, 0xff4cd9f7, 0xff51e2fb, 0xff90cdd9, 0xff42a1ad,
		0xff3bd2dd, 0xff51a0a5, 0xff3fc2be, 0xff2d6a6c, 0xff509693, 0xff2d8a83, 0xff79b4b1, 0xff386161, 0xff2a4e4b,
		0xff2e625b, 0xff39514d, 0xff6b9189, 0xff4bb490, 0xff70ad91, 0xffa0cab5, 0xff586a64, 0xff3fa27b, 0xff66c186,
		0xff3d594a, 0xff2d6042, 0xff416e51, 0xff93b491, 0xff7c8f80, 0xff3e811b, 0xff81ac5d, 0xff3c5636, 0xff517d22,
		0xffb9d8a8, 0xff72836a, 0xff4b6d2d, 0xff4c5d46, 0xff6e9324, 0xff97a686, 0xff6c8900, 0xff486109, 0xff4f6020,
		0xff3a4c0f, 0xff6c724f, 0xff90aa00, 0xffacb069, 0xff3d4526, 0xffb7ba66, 0xff858726, 0xff555b40, 0xff565a30,
		0xffc4c278, 0xff6d6b37, 0xffe4dea5, 0xff9a9677, 0xffa19966, 0xffd4c781, 0xffb8a633, 0xff42480c, 0xff61560d,
		0xffa78900, 0xff746733, 0xff595325, 0xffa8881e, 0xff736c56, 0xff8a7c57, 0xffdcb258, 0xff755f2b, 0xffb78f3a,
		0xff6e5c2e, 0xff846200, 0xffdeb97d, 0xffdda851, 0xffdfa92e, 0xff13100b, 0xff40250f, 0xff2d1908, 0xffaf5c00,
		0xff6e340b, 0xffd2907b, 0xffa4756e, 0xff471e26, 0xff853211, 0xff974f4e, 0xff551e21, 0xffc3818b, 0xff9a6470,
		0xffc2909b, 0xffbe6b8a, 0xff9c4c6a, 0xffb5778f, 0xff5b3d53, 0xffce8fb2, 0xffb26d98, 0xff8d4277, 0xff412f3c,
		0xff5d224a, 0xff7c3266, 0xff632c59, 0xff81336f, 0xff574c57, 0xffbb81b4, 0xff362b3f, 0xff3f2a57, 0xff503d5e,
		0xff6e6372, 0xff542962, 0xff5b2e6d, 0xff8e32c1, 0xff7a49a8, 0xff372e56, 0xff8a3ce0, 0xff3e3760, 0xfff2fafc,
		0xfffbffff, 0xffbac0bd, 0xff9f9891, 0xff7b7d78, 0xff747c70, 0xff656765, 0xff535953, 0xff484f4f, 0xff3d4352,
		0xff383c37, 0xff26323a, 0xffeeeeee, 0xffdddddd, 0xffbbbbbb, 0xffaaaaaa, 0xff888888, 0xff777777, 0xff555555,
		0xff444444, 0xff222222, 0xff111111, 0xff000000
	};
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
	const int imageWidth = image->width();
	const int imageHeight = image->height();
	core::Buffer<core::RGBA, 1024> colors;
	Log::debug("Create palette for image: %s", image->name().c_str());
	for (int x = 0; x < imageWidth; ++x) {
		for (int y = 0; y < imageHeight; ++y) {
			const core::RGBA data = image->colorAt(x, y);
			colors.push_back(data);
		}
	}
	palette.quantize(colors.data(), colors.size());
	palette.markDirty();
	return true;
}

bool Palette::hasGlow(uint8_t idx) const {
	return glowColors[idx] != 0u;
}

void Palette::removeGlow(uint8_t idx) {
	glowColors[idx] = 0;
	markDirty();
}

void Palette::setGlow(uint8_t idx, float factor) {
	// TODO: handle factor
	glowColors[idx] = colors[idx];
	markDirty();
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
