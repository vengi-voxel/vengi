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
#include "image/Image.h"
#include "io/FileStream.h"
#include "palette/Palette.h"
#include "palette/private/PaletteFormat.h"

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace palette {
namespace priv {

// VXL normals converted to vengi coordinates: (x, z, -y).
static const color::RGBA tsnormals[]{
	{213, 36, 103},  {161, 29, 53},   {122, 0, 116},   {54, 23, 140},   {105, 25, 201},  {173, 15, 167},
	{230, 67, 172},  {140, 85, 8},    {86, 32, 53},    {25, 64, 85},    {42, 69, 203},   {167, 62, 230},
	{251, 104, 109}, {214, 94, 41},   {61, 100, 22},   {4, 100, 151},   {94, 98, 248},   {155, 139, 252},
	{245, 166, 157}, {116, 156, 4},   {52, 179, 39},   {9, 139, 81},    {37, 149, 216},  {220, 124, 215},
	{236, 173, 80},  {187, 162, 21},  {115, 223, 45},  {12, 178, 148},  {100, 187, 237}, {191, 196, 214},
	{202, 229, 142}, {183, 226, 70},  {122, 254, 117}, {51, 225, 100},  {62, 225, 179},  {136, 239, 189},
};

// VXL normals converted to vengi coordinates: (x, z, -y).
static const color::RGBA ra2normals[]{
	{194, 29, 174},  {146, 240, 72},  {180, 59, 34},   {137, 77, 11},   {87, 104, 9},    {28, 113, 49},
	{12, 118, 73},   {0, 133, 129},   {2, 111, 148},   {11, 102, 174},  {47, 89, 220},   {87, 63, 231},
	{146, 56, 232},  {36, 118, 217},  {191, 18, 143},  {185, 105, 17},  {126, 0, 143},   {114, 7, 170},
	{198, 83, 32},   {119, 55, 23},   {88, 60, 26},    {41, 87, 42},    {28, 205, 144},  {9, 94, 93},
	{38, 69, 198},   {55, 45, 194},   {134, 82, 247},  {224, 87, 55},   {127, 6, 89},    {119, 108, 254},
	{208, 29, 122},  {194, 23, 97},   {161, 35, 47},   {133, 33, 42},   {104, 36, 42},   {76, 43, 47},
	{57, 39, 68},    {29, 47, 117},   {42, 34, 143},   {58, 28, 169},   {78, 30, 195},   {94, 41, 216},
	{125, 35, 217},  {166, 22, 189},  {214, 37, 153},  {96, 4, 143},    {229, 51, 131},  {80, 9, 116},
	{85, 14, 170},   {106, 21, 195},  {143, 7, 168},   {172, 13, 163},  {158, 4, 139},   {177, 10, 118},
	{160, 9, 94},    {146, 17, 67},   {85, 23, 67},    {67, 15, 142},   {231, 62, 161},  {67, 21, 91},
	{226, 65, 78},   {207, 41, 78},   {221, 45, 102},  {188, 42, 56},   {176, 22, 74},   {115, 17, 64},
	{97, 9, 90},     {110, 1, 116},   {22, 65, 90},    {205, 65, 208},  {136, 18, 194},  {156, 36, 213},
	{187, 41, 201},  {177, 57, 223},  {227, 77, 189},  {250, 97, 111},  {239, 70, 106},  {208, 63, 53},
	{150, 54, 26},   {150, 206, 31},  {58, 64, 42},    {40, 41, 91},    {52, 25, 116},   {22, 57, 145},
	{36, 49, 171},   {149, 180, 242}, {116, 58, 235},  {163, 79, 240},  {213, 50, 182},  {235, 107, 193},
	{252, 103, 141}, {235, 114, 61},  {213, 108, 36},  {168, 80, 17},   {104, 80, 12},   {70, 83, 23},
	{37, 62, 65},    {2, 101, 120},   {15, 158, 181},  {20, 74, 174},   {63, 61, 216},   {103, 85, 246},
	{152, 108, 252}, {193, 83, 228},  {243, 89, 166},  {253, 132, 144}, {254, 126, 115}, {224, 136, 46},
	{153, 102, 6},   {122, 102, 3},   {71, 128, 13},   {22, 88, 67},    {12, 72, 118},   {9, 83, 146},
	{26, 96, 199},   {72, 88, 236},   {126, 160, 251}, {202, 116, 231}, {216, 95, 213},  {240, 91, 82},
	{227, 203, 103}, {193, 215, 63},  {178, 203, 40},  {107, 179, 13},  {43, 194, 60},   {32, 200, 85},
	{47, 226, 134},  {168, 243, 160}, {56, 220, 76},   {26, 203, 115},  {45, 170, 216},  {111, 212, 222},
	{140, 206, 228}, {179, 226, 189}, {216, 187, 198}, {251, 156, 129}, {248, 121, 88},  {199, 130, 23},
	{169, 128, 7},   {105, 126, 2},   {42, 134, 34},   {22, 142, 57},   {4, 156, 113},   {5, 135, 166},
	{17, 126, 192},  {59, 115, 235},  {106, 136, 253}, {137, 132, 255}, {222, 127, 213}, {247, 119, 170},
	{247, 152, 164}, {225, 168, 58},  {209, 157, 35},  {137, 128, 1},   {121, 154, 3},   {59, 154, 23},
	{38, 164, 44},   {3, 125, 100},   {4, 157, 146},   {29, 148, 206},  {70, 167, 235},  {77, 139, 245},
	{177, 133, 245}, {201, 146, 230}, {237, 139, 190}, {242, 179, 114}, {249, 151, 98},  {190, 178, 30},
	{152, 155, 6},   {181, 153, 16},  {79, 174, 20},   {59, 185, 37},   {15, 179, 98},   {12, 181, 130},
	{31, 180, 193},  {63, 192, 217},  {97, 165, 246},  {156, 152, 250}, {221, 158, 209}, {244, 76, 136},
	{242, 177, 149}, {237, 176, 84},  {207, 190, 51},  {164, 181, 19},  {136, 181, 13},  {91, 150, 8},
	{54, 108, 25},   {141, 1, 115},   {88, 111, 248},  {50, 143, 229},  {90, 192, 232},  {119, 188, 240},
	{179, 165, 238}, {199, 177, 221}, {234, 170, 182}, {231, 200, 133}, {219, 198, 75},  {239, 145, 71},
	{178, 104, 243}, {120, 205, 27},  {90, 200, 31},   {70, 210, 50},   {46, 222, 104},  {16, 180, 161},
	{52, 206, 195},  {82, 214, 210},  {132, 228, 206}, {171, 193, 227}, {191, 204, 207}, {228, 195, 167},
	{211, 222, 120}, {204, 221, 90},  {176, 234, 80},  {158, 248, 101}, {129, 250, 95},  {114, 240, 70},
	{133, 226, 48},  {72, 227, 185},  {72, 239, 153},  {94, 243, 171},  {110, 252, 148}, {140, 251, 155},
	{152, 238, 185}, {207, 210, 182}, {215, 217, 150}, {162, 250, 131}, {186, 238, 108}, {163, 224, 54},
	{23, 173, 69},   {103, 222, 46},  {84, 230, 67},   {10, 150, 84},   {132, 254, 125}, {33, 200, 173},
	{50, 222, 165},  {103, 231, 198}, {124, 244, 179}, {161, 218, 211}, {193, 230, 164}, {190, 238, 136},
	{85, 246, 110},  {85, 246, 110},  {85, 246, 110},  {85, 246, 110},
};

// normals from slab6
// Slab6 normals converted to vengi coordinates: (x, -z, y). Index 255 has no direction.
static const color::RGBA slab6normals[]{
	{138, 255, 127}, {113, 254, 140}, {129, 253, 102}, {145, 252, 151}, {94, 251, 121},  {158, 250, 107},
	{117, 249, 166}, {107, 248, 89},  {170, 247, 143}, {82, 246, 145},  {148, 245, 81},  {143, 244, 177},
	{79, 243, 99},   {183, 242, 115}, {93, 241, 175},  {119, 240, 67},  {175, 239, 167}, {63, 238, 130},
	{174, 237, 80},  {124, 236, 195}, {83, 235, 74},   {197, 234, 136}, {68, 233, 168},  {143, 232, 55},
	{164, 231, 192}, {54, 230, 104},  {198, 229, 94},  {96, 228, 200},  {100, 227, 51},  {199, 226, 165},
	{47, 225, 148},  {172, 224, 56},  {141, 223, 211}, {59, 222, 74},   {214, 221, 120}, {67, 220, 192},
	{127, 219, 38},  {188, 218, 194}, {36, 217, 119},  {200, 216, 71},  {110, 215, 219}, {77, 214, 47},
	{219, 213, 152}, {42, 212, 171},  {161, 211, 36},  {164, 210, 217}, {38, 209, 85},   {222, 208, 98},
	{76, 207, 213},  {106, 206, 28},  {210, 205, 186}, {25, 204, 140},  {194, 203, 48},  {131, 202, 231},
	{54, 201, 53},   {232, 200, 132}, {45, 199, 194},  {142, 198, 21},  {188, 197, 215}, {22, 196, 103},
	{221, 195, 74},  {93, 194, 231},  {81, 193, 27},   {229, 192, 170}, {23, 191, 164},  {179, 190, 29},
	{155, 189, 235}, {33, 188, 66},   {238, 187, 108}, {57, 186, 216},  {118, 185, 13},  {211, 184, 205},
	{12, 183, 126},  {213, 182, 50},  {116, 181, 242}, {57, 180, 34},   {242, 179, 148}, {27, 178, 189},
	{159, 177, 14},  {180, 176, 232}, {16, 175, 85},   {237, 174, 83},  {75, 173, 235},  {92, 172, 12},
	{230, 171, 188}, {9, 170, 152},   {197, 169, 29},  {141, 168, 247}, {35, 167, 48},   {249, 166, 123},
	{40, 165, 212},  {134, 164, 5},   {205, 163, 222}, {5, 162, 109},   {229, 161, 58},  {99, 160, 247},
	{67, 159, 19},   {245, 158, 166}, {14, 157, 178},  {176, 156, 13},  {168, 155, 245}, {17, 154, 68},
	{248, 153, 96},  {58, 152, 232},  {107, 151, 3},   {226, 150, 205}, {2, 149, 136},   {213, 148, 35},
	{125, 147, 253}, {43, 146, 33},   {253, 145, 140}, {26, 144, 202},  {151, 143, 3},   {193, 142, 235},
	{5, 141, 92},    {240, 140, 70},  {82, 139, 246},  {80, 138, 9},    {241, 137, 183}, {5, 136, 163},
	{192, 135, 18},  {153, 134, 252}, {24, 133, 52},   {254, 132, 112}, {43, 131, 223},  {124, 130, 0},
	{216, 129, 219}, {0, 128, 119},   {226, 127, 47},  {108, 126, 253}, {56, 125, 21},   {251, 124, 157},
	{16, 123, 189},  {167, 122, 6},   {179, 121, 243}, {10, 120, 76},   {247, 119, 86},  {66, 118, 239},
	{96, 117, 4},    {233, 116, 197}, {2, 115, 147},   {206, 114, 28},  {136, 113, 253}, {35, 112, 40},
	{253, 111, 129}, {32, 110, 210},  {140, 109, 2},   {202, 108, 228}, {4, 107, 103},   {234, 106, 62},
	{92, 105, 247},  {71, 104, 15},   {244, 103, 172}, {11, 102, 172},  {182, 101, 15},  {162, 100, 246},
	{21, 99, 63},    {249, 98, 103},  {54, 97, 227},   {114, 96, 4},    {220, 95, 208},  {4, 94, 130},
	{216, 93, 42},   {119, 92, 249},  {51, 91, 32},    {247, 90, 146},  {26, 89, 194},   {156, 88, 9},
	{185, 87, 233},  {13, 86, 88},    {237, 85, 79},   {79, 84, 236},   {89, 83, 14},    {231, 82, 184},
	{12, 81, 155},   {193, 80, 29},   {145, 79, 243},  {36, 78, 53},    {244, 77, 120},  {46, 76, 211},
	{130, 75, 11},   {203, 74, 214},  {13, 73, 114},   {220, 72, 60},   {104, 71, 239},  {68, 70, 30},
	{235, 69, 159},  {26, 68, 176},   {168, 67, 23},   {167, 66, 231},  {28, 65, 78},    {233, 64, 97},
	{70, 63, 221},   {106, 62, 20},   {214, 61, 192},  {20, 60, 138},   {198, 59, 47},   {129, 58, 234},
	{54, 57, 50},    {232, 56, 134},  {45, 55, 192},   {143, 54, 24},   {184, 53, 213},  {28, 52, 102},
	{216, 51, 79},   {94, 50, 222},   {87, 49, 35},    {218, 48, 168},  {34, 47, 158},   {175, 46, 42},
	{149, 45, 221},  {48, 44, 73},    {221, 43, 113},  {68, 42, 200},   {122, 41, 34},   {193, 40, 191},
	{36, 39, 124},   {195, 38, 68},   {117, 37, 216},  {75, 36, 55},    {212, 35, 145},  {53, 34, 171},
	{151, 33, 46},   {163, 32, 203},  {50, 31, 96},    {203, 30, 98},   {91, 29, 199},   {106, 28, 51},
	{193, 27, 168},  {52, 26, 141},   {172, 25, 67},   {134, 24, 200},  {73, 23, 79},    {198, 22, 126},
	{76, 21, 174},   {132, 20, 59},   {168, 19, 179},  {63, 18, 117},   {180, 17, 93},   {112, 16, 186},
	{99, 15, 75},    {181, 14, 146},  {76, 13, 149},   {149, 12, 79},   {143, 11, 175},  {85, 10, 103},
	{172, 9, 117},   {103, 8, 163},   {121, 7, 87},    {156, 6, 151},   {93, 5, 129},    {148, 4, 106},
	{126, 3, 152},   {114, 2, 112},   {138, 1, 128},
};

} // namespace priv

const char *NormalPalette::getDefaultPaletteName() {
	return builtIn[0];
}

color::RGBA NormalPalette::toRGBA(const glm::vec3 &normal) {
	// Map the normal components back to [0, 1] range
	const float rf = (normal.x + 1.0f) / 2.0f; // X component to [0, 1]
	const float gf = (normal.y + 1.0f) / 2.0f; // Y component to [0, 1]
	const float bf = (normal.z + 1.0f) / 2.0f; // Z component to [0, 1]

	// Convert to [0, 255] for RGB
	const uint8_t r = (uint8_t)(rf * 255.0f);
	const uint8_t g = (uint8_t)(gf * 255.0f);
	const uint8_t b = (uint8_t)(bf * 255.0f);
	return color::RGBA(r, g, b);
}

glm::vec3 NormalPalette::toVec3(const color::RGBA &rgba) {
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

int NormalPalette::getClosestMatch(const glm::vec3 &normal) const {
	int closestIndex = PaletteNormalNotFound;
	float maxDot = -1.0f;

	for (int i = 0; i < _size; ++i) {
		const float dot = glm::dot(normal, toVec3(_normals[i]));

		if (dot > maxDot) {
			maxDot = dot;
			closestIndex = i;
		}
	}
	return closestIndex;
}

void NormalPalette::createRemap(const NormalPalette &target, uint8_t *remap) const {
	for (int i = 0; i < NormalPaletteMaxNormals; ++i) {
		remap[i] = 255;
		if (i >= _size) {
			continue;
		}
		if (i < (int)target.size() && normal(i) == target.normal(i)) {
			remap[i] = (uint8_t)i;
			continue;
		}
		const glm::vec3 source = glm::normalize(normal3f(i));
		float maxDot = -2.0f;
		for (int j = 0; j < (int)target.size(); ++j) {
			const float dot = glm::dot(source, glm::normalize(target.normal3f(j)));
			if (dot > maxDot) {
				maxDot = dot;
				remap[i] = (uint8_t)j;
			}
		}
	}
}

void NormalPalette::setNormal(uint8_t index, const glm::vec3 &normal) {
	_normals[index] = toRGBA(normal);
	_size = core_max(index, _size);
	markDirty();
}

void NormalPalette::loadNormalMap(const glm::vec3 *normals, int size) {
	size = core_min(size, NormalPaletteMaxNormals);
	for (int i = 0; i < size; i++) {
		_normals[i] = toRGBA(normals[i]);
	}
	for (int i = size; i < NormalPaletteMaxNormals; i++) {
		_normals[i] = color::RGBA(0);
	}
	_size = size;
	markDirty();
}

void NormalPalette::loadNormalMap(const color::RGBA *normals, int size) {
	size = core_min(size, NormalPaletteMaxNormals);
	for (int i = 0; i < size; i++) {
		_normals[i] = normals[i];
	}
	for (int i = size; i < NormalPaletteMaxNormals; i++) {
		_normals[i] = color::RGBA(0);
	}
	_size = size;
	markDirty();
}

void NormalPalette::tiberianSun() {
	loadNormalMap(priv::tsnormals, lengthof(priv::tsnormals));
	_name = builtIn[1];
}

void NormalPalette::redAlert2() {
	loadNormalMap(priv::ra2normals, lengthof(priv::ra2normals));
	_name = builtIn[0];
}

void NormalPalette::slab6() {
	// KV6 index 255 means no normal and is not included in the palette.
	loadNormalMap(priv::slab6normals, lengthof(priv::slab6normals));
	_name = builtIn[2];
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

void NormalPalette::toVec4f(glm::highp_vec4 *vec4f) const {
	core_memset(vec4f, 0, sizeof(glm::highp_vec4) * NormalPaletteMaxNormals);
	constexpr float scale = 2.0f / 255.0f;
	for (int i = 0; i < _size; ++i) {
		const color::RGBA &rgba = _normals[i];
		vec4f[i].x = (float)rgba.r * scale - 1.0f;
		vec4f[i].y = (float)rgba.g * scale - 1.0f;
		vec4f[i].z = (float)rgba.b * scale - 1.0f;
	}
}

uint32_t NormalPalette::hash() const {
	if (_hashDirty) {
		_hashDirty = false;
		_hash = core::hash(_normals, sizeof(_normals));
	}
	return _hash;
}


void NormalPalette::markDirty() {
	core::DirtyState::markDirty();
	_hashDirty = true;
}

bool NormalPalette::load(const char *paletteName) {
	if (paletteName == nullptr || paletteName[0] == '\0') {
		return false;
	}

	// this is handled in the scene manager it is just ignored here
	if (SDL_strncmp(paletteName, NODE_PALETTE_PREFIX, 5) == 0) {
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
	} else if (SDL_strcmp(paletteName, builtIn[2]) == 0) {
		slab6();
		return true;
	}
	static_assert(lengthof(builtIn) == 3, "Unexpected amount of built-in palettes");

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
	for (int i = 0; i < _size; ++i) {
		_normals[i] = paletteToLoad.color(i);
	}
	for (int i = _size; i < NormalPaletteMaxNormals; ++i) {
		_normals[i] = color::RGBA(0);
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
		_normals[i] = color::RGBA(0);
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
	for (int i = 0; i < _size; i++) {
		palForSave.setColor(i, _normals[i]);
	}
	return palette::savePalette(palForSave, name, stream);
}

} // namespace palette
