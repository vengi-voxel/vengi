/**
 * @file
 */

#include "NormalPalette.h"
#include "app/App.h"
#include "core/ArrayLength.h"
#include "core/Common.h"
#include "core/Hash.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "io/FileStream.h"
#include "palette/Palette.h"
#include "palette/private/PaletteFormat.h"

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace palette {
namespace priv {

static const core::RGBA tsnormals[]{
	{213, 152, 36},	 {161, 202, 29},  {122, 139, 0},   {54, 115, 23},  {105, 54, 25},  {173, 88, 15},
	{230, 83, 67},	 {140, 247, 85},  {86, 202, 32},   {25, 170, 64},  {42, 52, 69},   {167, 25, 62},
	{251, 146, 104}, {214, 214, 94},  {61, 233, 100},  {4, 104, 100},  {94, 7, 98},	   {155, 3, 139},
	{245, 98, 166},	 {116, 251, 156}, {52, 216, 179},  {9, 174, 139},  {37, 39, 149},  {220, 40, 124},
	{236, 175, 173}, {187, 234, 162}, {115, 210, 223}, {12, 107, 178}, {100, 18, 187}, {191, 41, 196},
	{202, 113, 229}, {183, 185, 226}, {122, 138, 254}, {51, 155, 225}, {62, 76, 225},  {136, 66, 239},
};

static const core::RGBA ra2normals[]{
	{194, 81, 29},	 {146, 183, 240}, {180, 221, 59},  {137, 244, 77},	{87, 246, 104},	 {28, 206, 113},
	{12, 182, 118},	 {0, 126, 133},	  {2, 107, 111},   {11, 81, 102},	{47, 35, 89},	 {87, 24, 63},
	{146, 23, 56},	 {36, 38, 118},	  {191, 112, 18},  {185, 238, 105}, {126, 112, 0},	 {114, 85, 7},
	{198, 223, 83},	 {119, 232, 55},  {88, 229, 60},   {41, 213, 87},	{28, 111, 205},	 {9, 162, 94},
	{38, 57, 69},	 {55, 61, 45},	  {134, 8, 82},	   {224, 200, 87},	{127, 166, 6},	 {119, 1, 108},
	{208, 133, 29},	 {194, 158, 23},  {161, 208, 35},  {133, 213, 33},	{104, 213, 36},	 {76, 208, 43},
	{57, 187, 39},	 {29, 138, 47},	  {42, 112, 34},   {58, 86, 28},	{78, 60, 30},	 {94, 39, 41},
	{125, 38, 35},	 {166, 66, 22},	  {214, 102, 37},  {96, 112, 4},	{229, 124, 51},	 {80, 139, 9},
	{85, 85, 14},	 {106, 60, 21},	  {143, 87, 7},	   {172, 92, 13},	{158, 116, 4},	 {177, 137, 10},
	{160, 161, 9},	 {146, 188, 17},  {85, 188, 23},   {67, 113, 15},	{231, 94, 62},	 {67, 164, 21},
	{226, 177, 65},	 {207, 177, 41},  {221, 153, 45},  {188, 199, 42},	{176, 181, 22},	 {115, 191, 17},
	{97, 165, 9},	 {110, 139, 1},	  {22, 165, 65},   {205, 47, 65},	{136, 61, 18},	 {156, 42, 36},
	{187, 54, 41},	 {177, 32, 57},	  {227, 66, 77},   {250, 144, 97},	{239, 149, 70},	 {208, 202, 63},
	{150, 229, 54},	 {150, 224, 206}, {58, 213, 64},   {40, 164, 41},	{52, 139, 25},	 {22, 110, 57},
	{36, 84, 49},	 {149, 13, 180},  {116, 20, 58},   {163, 15, 79},	{213, 73, 50},	 {235, 62, 107},
	{252, 114, 103}, {235, 194, 114}, {213, 219, 108}, {168, 238, 80},	{104, 243, 80},	 {70, 232, 83},
	{37, 190, 62},	 {2, 135, 101},	  {15, 74, 158},   {20, 81, 74},	{63, 39, 61},	 {103, 9, 85},
	{152, 3, 108},	 {193, 27, 83},	  {243, 89, 89},   {253, 111, 132}, {254, 140, 126}, {224, 209, 136},
	{153, 249, 102}, {122, 252, 102}, {71, 242, 128},  {22, 188, 88},	{12, 137, 72},	 {9, 109, 83},
	{26, 56, 96},	 {72, 19, 88},	  {126, 4, 160},   {202, 24, 116},	{216, 42, 95},	 {240, 173, 91},
	{227, 152, 203}, {193, 192, 215}, {178, 215, 203}, {107, 242, 179}, {43, 195, 194},	 {32, 170, 200},
	{47, 121, 226},	 {168, 95, 243},  {56, 179, 220},  {26, 140, 203},	{45, 39, 170},	 {111, 33, 212},
	{140, 27, 206},	 {179, 66, 226},  {216, 57, 187},  {251, 126, 156}, {248, 167, 121}, {199, 232, 130},
	{169, 248, 128}, {105, 253, 126}, {42, 221, 134},  {22, 198, 142},	{4, 142, 156},	 {5, 89, 135},
	{17, 63, 126},	 {59, 20, 115},	  {106, 2, 136},   {137, 0, 132},	{222, 42, 127},	 {247, 85, 119},
	{247, 91, 152},	 {225, 197, 168}, {209, 220, 157}, {137, 254, 128}, {121, 252, 154}, {59, 232, 154},
	{38, 211, 164},	 {3, 155, 125},	  {4, 109, 157},   {29, 49, 148},	{70, 20, 167},	 {77, 10, 139},
	{177, 10, 133},	 {201, 25, 146},  {237, 65, 139},  {242, 141, 179}, {249, 157, 151}, {190, 225, 178},
	{152, 249, 155}, {181, 239, 153}, {79, 235, 174},  {59, 218, 185},	{15, 157, 179},	 {12, 125, 181},
	{31, 62, 180},	 {63, 38, 192},	  {97, 9, 165},	   {156, 5, 152},	{221, 46, 158},	 {244, 119, 76},
	{242, 106, 177}, {237, 171, 176}, {207, 204, 190}, {164, 236, 181}, {136, 242, 181}, {91, 247, 150},
	{54, 230, 108},	 {141, 140, 1},	  {88, 7, 111},	   {50, 26, 143},	{90, 23, 192},	 {119, 15, 188},
	{179, 17, 165},	 {199, 34, 177},  {234, 73, 170},  {231, 122, 200}, {219, 180, 198}, {239, 184, 145},
	{178, 12, 104},	 {120, 228, 205}, {90, 224, 200},  {70, 205, 210},	{46, 151, 222},	 {16, 94, 180},
	{52, 60, 206},	 {82, 45, 214},	  {132, 49, 228},  {171, 28, 193},	{191, 48, 204},	 {228, 88, 195},
	{211, 135, 222}, {204, 165, 221}, {176, 175, 234}, {158, 154, 248}, {129, 160, 250}, {114, 185, 240},
	{133, 207, 226}, {72, 70, 227},	  {72, 102, 239},  {94, 84, 243},	{110, 107, 252}, {140, 100, 251},
	{152, 70, 238},	 {207, 73, 210},  {215, 105, 217}, {162, 124, 250}, {186, 147, 238}, {163, 201, 224},
	{23, 186, 173},	 {103, 209, 222}, {84, 188, 230},  {10, 171, 150},	{132, 130, 254}, {33, 82, 200},
	{50, 90, 222},	 {103, 57, 231},  {124, 76, 244},  {161, 44, 218},	{193, 91, 230},	 {190, 119, 238},
	{85, 145, 246},	 {85, 145, 246},  {85, 145, 246},  {85, 145, 246},
};
} // namespace priv

static inline core::RGBA toRGBA(const glm::vec3 &normal) {
	// Map the normal components back to [0, 1] range
	const float rf = (normal.x + 1.0f) / 2.0f; // X component to [0, 1]
	const float gf = (normal.y + 1.0f) / 2.0f; // Y component to [0, 1]
	const float bf = (normal.z + 1.0f) / 2.0f; // Z component to [0, 1]

	// Convert to [0, 255] for RGB
	const uint8_t r = (uint8_t)(rf * 255.0f);
	const uint8_t g = (uint8_t)(gf * 255.0f);
	const uint8_t b = (uint8_t)(bf * 255.0f);
	return core::RGBA(r, g, b);
}

static inline glm::vec3 toVec3(const core::RGBA &rgba) {
	// Normalize RGB values to the range [0, 1]
	const float r = rgba.r / 255.0f;
	const float g = rgba.g / 255.0f;
	const float b = rgba.b / 255.0f;

	// Map to the correct range [-1, 1] for X, Y, and Z
	const float nx = 2.0f * r - 1.0f;
	const float ny = 2.0f * g - 1.0f;
	const float nz = 2.0f * b - 1.0f;

	return glm::vec3(nx, ny, nz);
}

uint8_t NormalPalette::getClosestMatch(const glm::vec3 &normal) const {
	uint8_t closestIndex = 0;
	float maxDot = -1.0f;

	for (size_t i = 0; i < _size; ++i) {
		const float dot = glm::dot(normal, toVec3(_normals[i]));

		if (dot > maxDot) {
			maxDot = dot;
			closestIndex = i;
		}
	}
	return closestIndex;
}

void NormalPalette::setNormal(uint8_t index, const glm::vec3 &normal) {
	_normals[index] = toRGBA(normal);
	_size = core_max(index, _size);
	markDirty();
}

void NormalPalette::loadNormalMap(const glm::vec3 *normals, uint8_t size) {
	for (uint8_t i = 0; i < size; i++) {
		_normals[i] = toRGBA(normals[i]);
	}
	for (uint8_t i = size; i < NormalPaletteMaxNormals; i++) {
		_normals[i] = core::RGBA(0);
	}
	_size = size;
	markDirty();
}

void NormalPalette::loadNormalMap(const core::RGBA *normals, uint8_t size) {
	for (uint8_t i = 0; i < size; i++) {
		_normals[i] = normals[i];
	}
	for (uint8_t i = size; i < NormalPaletteMaxNormals; i++) {
		_normals[i] = core::RGBA(0);
	}
	_size = size;
	markDirty();
}

void NormalPalette::tiberianSun() {
	loadNormalMap(priv::tsnormals, (uint8_t)lengthof(priv::tsnormals));
	_name = builtIn[1];
}

void NormalPalette::redAlert2() {
	loadNormalMap(priv::ra2normals, (uint8_t)lengthof(priv::ra2normals));
	_name = builtIn[0];
}

bool NormalPalette::isTiberianSun() const {
	return _name == builtIn[1];
}

bool NormalPalette::isRedAlert2() const {
	return _name == builtIn[0];
}

bool NormalPalette::isBuiltIn() const {
	for (int i = 0; i < lengthof(builtIn); ++i) {
		if (_name.equals(builtIn[i])) {
			return true;
		}
	}
	return false;
}

glm::vec3 NormalPalette::normal3f(uint8_t index) const {
	return toVec3(_normals[index]);
}

void NormalPalette::toVec4f(core::DynamicArray<glm::vec4> &vec4f) const {
	vec4f.reserve(NormalPaletteMaxNormals);
	for (int i = 0; i < _size; ++i) {
		const glm::vec3 &n = toVec3(_normals[i]);
		vec4f.emplace_back(n.x, n.y, n.z, 0.0f);
	}
	for (int i = _size; i < NormalPaletteMaxNormals; ++i) {
		vec4f.emplace_back(0.0f);
	}
}

void NormalPalette::toVec4f(glm::highp_vec4 *vec4f) const {
	for (int i = 0; i < _size; ++i) {
		const glm::vec3 &norm = toVec3(_normals[i]);
		vec4f[i] = {norm.x, norm.y, norm.z, 0.0f};
	}
	for (int i = _size; i < NormalPaletteMaxNormals; ++i) {
		vec4f[i] = {0.0f, 0.0f, 0.0f, 0.0f};
	}
}

void NormalPalette::markDirty() {
	core::DirtyState::markDirty();
	_hash = core::hash(_normals, sizeof(_normals));
}

bool NormalPalette::load(const char *paletteName) {
	if (paletteName == nullptr || paletteName[0] == '\0') {
		return false;
	}

	// this is handled in the scene manager it is just ignored here
	if (SDL_strncmp(paletteName, "node:", 5) == 0) {
		if (_size == 0) {
			redAlert2();
		}
		_name = paletteName + 5;
		return false;
	}

	if (SDL_strcmp(paletteName, builtIn[0]) == 0) {
		redAlert2();
		return true;
	} else if (SDL_strcmp(paletteName, builtIn[1]) == 0) {
		tiberianSun();
		return true;
	}
	static_assert(lengthof(builtIn) == 2, "Unexpected amount of built-in palettes");

	const io::FilesystemPtr &filesystem = io::filesystem();
	io::FilePtr paletteFile = filesystem->open(paletteName);
	if (!paletteFile->validHandle()) {
		paletteFile = filesystem->open(core::String::format("normals-%s.png", paletteName));
		if (!paletteFile->validHandle()) {
			Log::error("Failed to load normal palette file %s", paletteName);
			return false;
		}
	}
	io::FileStream stream(paletteFile);
	if (!stream.valid()) {
		Log::error("Failed to load image %s", paletteFile->name().c_str());
		return false;
	}

	palette::Palette paletteToLoad;
	if (!palette::loadPalette(paletteFile->name(), stream, paletteToLoad)) {
		const image::ImagePtr &img = image::loadImage(paletteFile);
		if (!img->isLoaded()) {
			Log::error("Failed to load image %s", paletteFile->name().c_str());
			return false;
		}
		return load(img);
	}
	_size = paletteToLoad.colorCount();
	for (uint8_t i = 0; i < _size; ++i) {
		_normals[i] = paletteToLoad.color(i);
	}
	for (uint8_t i = _size; i < NormalPaletteMaxNormals; ++i) {
		_normals[i] = core::RGBA(0);
	}
	markDirty();
	return true;
}

bool NormalPalette::load(const image::ImagePtr &img) {
	if (img->components() != 4) {
		Log::warn("Palette image has invalid depth (expected: 4bpp, got %i)", img->components());
		return false;
	}
	if (img->width() * img->height() > NormalPaletteMaxNormals) {
		Log::warn("Palette image has invalid dimensions - we need max 256x1");
		return false;
	}
	int ncolors = img->width();
	if (ncolors > NormalPaletteMaxNormals) {
		ncolors = NormalPaletteMaxNormals;
		Log::warn("Palette image has invalid dimensions - we need max 256x1(depth: 4)");
	}
	_size = ncolors;
	for (int i = 0; i < _size; ++i) {
		_normals[i] = img->colorAt(i, 0);
	}
	for (int i = _size; i < NormalPaletteMaxNormals; ++i) {
		_normals[i] = core::RGBA(0);
	}
	_name = img->name();
	markDirty();
	Log::debug("Set up %i normals", _size);
	return true;
}

bool NormalPalette::save(const char *name) const {
	if (name == nullptr || name[0] == '\0') {
		if (_name.empty()) {
			Log::error("No name given to save the current palette");
			return false;
		}
		name = _name.c_str();
	}
	const core::String ext = core::string::extractExtension(name);
	if (ext.empty()) {
		Log::error("No extension found for %s - can't determine the palette format", name);
		return false;
	}
	const io::FilePtr &file = io::filesystem()->open(name, io::FileMode::SysWrite);
	io::FileStream stream(file);
	if (!stream.valid()) {
		Log::error("Failed to open file %s for writing", name);
		return false;
	}
	palette::Palette palForSave;
	palForSave.setSize(_size);
	for (uint8_t i = 0; i < _size; i++) {
		palForSave.setColor(i, _normals[i]);
	}
	return palette::savePalette(palForSave, name, stream);
}

} // namespace palette
