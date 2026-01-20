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

static const color::RGBA tsnormals[]{
	{213, 152, 36},	 {161, 202, 29},  {122, 139, 0},   {54, 115, 23},  {105, 54, 25},  {173, 88, 15},
	{230, 83, 67},	 {140, 247, 85},  {86, 202, 32},   {25, 170, 64},  {42, 52, 69},   {167, 25, 62},
	{251, 146, 104}, {214, 214, 94},  {61, 233, 100},  {4, 104, 100},  {94, 7, 98},	   {155, 3, 139},
	{245, 98, 166},	 {116, 251, 156}, {52, 216, 179},  {9, 174, 139},  {37, 39, 149},  {220, 40, 124},
	{236, 175, 173}, {187, 234, 162}, {115, 210, 223}, {12, 107, 178}, {100, 18, 187}, {191, 41, 196},
	{202, 113, 229}, {183, 185, 226}, {122, 138, 254}, {51, 155, 225}, {62, 76, 225},  {136, 66, 239},
};

static const color::RGBA ra2normals[]{
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

// normals from slab6
static const color::RGBA slab6normals[]{
	{138, 127, 0},	 {113, 140, 1},	  {129, 102, 2},   {145, 151, 3},	{94, 121, 4},	 {158, 107, 5},
	{117, 166, 6},	 {107, 89, 7},	  {170, 143, 8},   {82, 145, 9},	{148, 81, 10},	 {143, 177, 11},
	{79, 99, 12},	 {183, 115, 13},  {93, 175, 14},   {119, 67, 15},	{175, 167, 16},	 {63, 130, 17},
	{174, 80, 18},	 {124, 195, 19},  {83, 74, 20},	   {197, 136, 21},	{68, 168, 22},	 {143, 55, 23},
	{164, 192, 24},	 {54, 104, 25},	  {198, 94, 26},   {96, 200, 27},	{100, 51, 28},	 {199, 165, 29},
	{47, 148, 30},	 {172, 56, 31},	  {141, 211, 32},  {59, 74, 33},	{214, 120, 34},	 {67, 192, 35},
	{127, 38, 36},	 {188, 194, 37},  {36, 119, 38},   {200, 71, 39},	{110, 219, 40},	 {77, 47, 41},
	{219, 152, 42},	 {42, 171, 43},	  {161, 36, 44},   {164, 217, 45},	{38, 85, 46},	 {222, 98, 47},
	{76, 213, 48},	 {106, 28, 49},	  {210, 186, 50},  {25, 140, 51},	{194, 48, 52},	 {131, 231, 53},
	{54, 53, 54},	 {232, 132, 55},  {45, 194, 56},   {142, 21, 57},	{188, 215, 58},	 {22, 103, 59},
	{221, 74, 60},	 {93, 231, 61},	  {81, 27, 62},	   {229, 170, 63},	{23, 164, 64},	 {179, 29, 65},
	{155, 235, 66},	 {33, 66, 67},	  {238, 108, 68},  {57, 216, 69},	{118, 13, 70},	 {211, 205, 71},
	{12, 126, 72},	 {213, 50, 73},	  {116, 242, 74},  {57, 34, 75},	{242, 148, 76},	 {27, 189, 77},
	{159, 14, 78},	 {180, 232, 79},  {16, 85, 80},	   {237, 83, 81},	{75, 235, 82},	 {92, 12, 83},
	{230, 188, 84},	 {9, 152, 85},	  {197, 29, 86},   {141, 247, 87},	{35, 48, 88},	 {249, 123, 89},
	{40, 212, 90},	 {134, 5, 91},	  {205, 222, 92},  {5, 109, 93},	{229, 58, 94},	 {99, 247, 95},
	{67, 19, 96},	 {245, 166, 97},  {14, 178, 98},   {176, 13, 99},	{168, 245, 100}, {17, 68, 101},
	{248, 96, 102},	 {58, 232, 103},  {107, 3, 104},   {226, 205, 105}, {2, 136, 106},	 {213, 35, 107},
	{125, 253, 108}, {43, 33, 109},	  {253, 140, 110}, {26, 202, 111},	{151, 3, 112},	 {193, 235, 113},
	{5, 92, 114},	 {240, 70, 115},  {82, 246, 116},  {80, 9, 117},	{241, 183, 118}, {5, 163, 119},
	{192, 18, 120},	 {153, 252, 121}, {24, 52, 122},   {254, 112, 123}, {43, 223, 124},	 {124, 0, 125},
	{216, 219, 126}, {0, 119, 127},	  {226, 47, 128},  {108, 253, 129}, {56, 21, 130},	 {251, 157, 131},
	{16, 189, 132},	 {167, 6, 133},	  {179, 243, 134}, {10, 76, 135},	{247, 86, 136},	 {66, 239, 137},
	{96, 4, 138},	 {233, 197, 139}, {2, 147, 140},   {206, 28, 141},	{136, 253, 142}, {35, 40, 143},
	{253, 129, 144}, {32, 210, 145},  {140, 2, 146},   {202, 228, 147}, {4, 103, 148},	 {234, 62, 149},
	{92, 247, 150},	 {71, 15, 151},	  {244, 172, 152}, {11, 172, 153},	{182, 15, 154},	 {162, 246, 155},
	{21, 63, 156},	 {249, 103, 157}, {54, 227, 158},  {114, 4, 159},	{220, 208, 160}, {4, 130, 161},
	{216, 42, 162},	 {119, 249, 163}, {51, 32, 164},   {247, 146, 165}, {26, 194, 166},	 {156, 9, 167},
	{185, 233, 168}, {13, 88, 169},	  {237, 79, 170},  {79, 236, 171},	{89, 14, 172},	 {231, 184, 173},
	{12, 155, 174},	 {193, 29, 175},  {145, 243, 176}, {36, 53, 177},	{244, 120, 178}, {46, 211, 179},
	{130, 11, 180},	 {203, 214, 181}, {13, 114, 182},  {220, 60, 183},	{104, 239, 184}, {68, 30, 185},
	{235, 159, 186}, {26, 176, 187},  {168, 23, 188},  {167, 231, 189}, {28, 78, 190},	 {233, 97, 191},
	{70, 221, 192},	 {106, 20, 193},  {214, 192, 194}, {20, 138, 195},	{198, 47, 196},	 {129, 234, 197},
	{54, 50, 198},	 {232, 134, 199}, {45, 192, 200},  {143, 24, 201},	{184, 213, 202}, {28, 102, 203},
	{216, 79, 204},	 {94, 222, 205},  {87, 35, 206},   {218, 168, 207}, {34, 158, 208},	 {175, 42, 209},
	{149, 221, 210}, {48, 73, 211},	  {221, 113, 212}, {68, 200, 213},	{122, 34, 214},	 {193, 191, 215},
	{36, 124, 216},	 {195, 68, 217},  {117, 216, 218}, {75, 55, 219},	{212, 145, 220}, {53, 171, 221},
	{151, 46, 222},	 {163, 203, 223}, {50, 96, 224},   {203, 98, 225},	{91, 199, 226},	 {106, 51, 227},
	{193, 168, 228}, {52, 141, 229},  {172, 67, 230},  {134, 200, 231}, {73, 79, 232},	 {198, 126, 233},
	{76, 174, 234},	 {132, 59, 235},  {168, 179, 236}, {63, 117, 237},	{180, 93, 238},	 {112, 186, 239},
	{99, 75, 240},	 {181, 146, 241}, {76, 149, 242},  {149, 79, 243},	{143, 175, 244}, {85, 103, 245},
	{172, 117, 246}, {103, 163, 247}, {121, 87, 248},  {156, 151, 249}, {93, 129, 250},	 {148, 106, 251},
	{126, 152, 252}, {114, 112, 253}, {138, 128, 254}, {127, 127, 255}
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
	// this palette uses 256 entries - more than we support
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

void NormalPalette::toVec4f(core::Buffer<glm::vec4> &vec4f) const {
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
