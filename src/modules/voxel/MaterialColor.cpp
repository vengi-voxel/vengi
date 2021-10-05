/**
 * @file
 */

#include "MaterialColor.h"
#include "app/App.h"
#include "core/Enum.h"
#include "math/Random.h"
#include "core/Color.h"
#include "core/GLM.h"
#include "io/Filesystem.h"
#include "core/StringUtil.h"
#include "core/Assert.h"
#include "core/collection/Map.h"
#include "core/collection/Array.h"
#include "commonlua/LUA.h"
#include "commonlua/LUAFunctions.h"
#include "voxel/Voxel.h"

namespace voxel {

class MaterialColor;

static const char* luamaterial_materialcolorid() {
	return "__global_materialcolor";
}

static MaterialColor* luamaterial_getmaterialcolor(lua_State*s) {
	lua_getglobal(s, luamaterial_materialcolorid());
	MaterialColor *provider = (MaterialColor *)lua_touserdata(s, -1);
	lua_pop(s, 1);
	return provider;
}

class MaterialColor {
private:
	MaterialColorArray _materialColors;
	core::Map<VoxelType, MaterialColorIndices, 8, EnumClassHash> _colorMapping;
	bool _initialized = false;
	bool _dirty = false;
public:
	MaterialColor() {
	}

	bool initialized() const {
		return _initialized;
	}

	bool init(const uint8_t* paletteBuffer, size_t paletteBufferSize, const core::String& luaString) {
		if (_initialized) {
			Log::debug("MaterialColors are already initialized");
			return true;
		}
		_initialized = true;
		_dirty = true;
		_materialColors.reserve(256);
		const size_t colors = paletteBufferSize / 4;
		if (colors != _materialColors.capacity()) {
			Log::error("Palette image has invalid dimensions - we need 256x1(depth: 4)");
			return false;
		}
		const uint32_t* paletteData = (const uint32_t*)paletteBuffer;
		for (size_t i = 0; i < colors; ++i) {
			_materialColors.emplace_back(core::Color::fromRGBA(*paletteData));
			++paletteData;
		}
		Log::debug("Set up %i material colors", (int)_materialColors.size());

		if (_materialColors.size() != colors) {
			Log::warn("Color amount mismatch");
			return false;
		}

		MaterialColorIndices generic;
		generic.reserve(colors);
		for (size_t i = 0; i < colors; ++i) {
			generic.push_back(i);
		}
		_colorMapping.put(voxel::VoxelType::Generic, generic);

		if (luaString.empty()) {
			Log::warn("No materials defined in palette lua script");
			return true;
		}

		constexpr size_t funcSize = 2u + ((size_t)(voxel::VoxelType::Max) - ((size_t)voxel::VoxelType::Air + 1u));
		core::Array<luaL_Reg, funcSize> funcs;
		static_assert((int)voxel::VoxelType::Air == 0, "Air must be 0");
		size_t aindex = 0u;
		for (int i = (int)voxel::VoxelType::Air + 1; i < (int)voxel::VoxelType::Max; ++i, ++aindex) {
			funcs[aindex] = luaL_Reg{ voxel::VoxelTypeStr[i], [] (lua_State* l) -> int {
				MaterialColor* mc = luamaterial_getmaterialcolor(l);
				const int index = luaL_checknumber(l, -1);
				// this is hacky - but we resolve the lua function name here to reverse lookup
				// the voxel type. This could maybe be done nicer with upvalues...
				lua_Debug entry;
				lua_getstack(l, 0, &entry);
				const int status = lua_getinfo(l, "Sln", &entry);
				core_assert_always(status == 1);
				for (int j = (int)voxel::VoxelType::Air + 1; j < (int)voxel::VoxelType::Max; ++j) {
					if (SDL_strcmp(voxel::VoxelTypeStr[j], entry.name) == 0) {
						auto i = mc->_colorMapping.find((VoxelType)j);
						if (i == mc->_colorMapping.end()) {
							MaterialColorIndices indices;
							indices.push_back(index);
							mc->_colorMapping.put((VoxelType)j, indices);
						} else {
							i->value.push_back(index);
						}
						break;
					}
				}
				return 0;
			}};
		}

		luaL_Reg getmaterial = { "material", [] (lua_State* l) -> int {
			MaterialColor* mc = luamaterial_getmaterialcolor(l);
			lua_newtable(l);
			const MaterialColorArray& colorArray = mc->getColors();
			lua_newtable(l);
			const int top = lua_gettop(l);
			for (size_t i = 0; i < colorArray.size(); ++i) {
				lua_pushinteger(l, i);
				clua_push(l, colorArray[i]);
				lua_settable(l, top);
			}
			return 1;
		}};

		lua::LUA lua;
		clua_vecregister<glm::vec4>(lua.state());
		funcs[aindex++] = getmaterial;
		funcs[aindex++] = { nullptr, nullptr };
		lua_pushlightuserdata(lua, this);
		lua_setglobal(lua, luamaterial_materialcolorid());
		clua_registerfuncsglobal(lua, &funcs[0], "__meta_material", "MAT");
		if (!lua.load(luaString)) {
			Log::error("Could not load lua script. Failed with error: %s",
					lua.error().c_str());
			return false;
		}
		if (!lua.execute("init")) {
			Log::error("Could not execute lua script. Failed with error: %s",
					lua.error().c_str());
			return false;
		}

		for (int j = (int)voxel::VoxelType::Air + 1; j < (int)voxel::VoxelType::Max; ++j) {
			const auto& i = _colorMapping.find((voxel::VoxelType)j);
			if (i == _colorMapping.end() || i->value.empty()) {
				Log::error("No colors are defined for VoxelType: %s", voxel::VoxelTypeStr[j]);
				return false;
			}
		}

		return true;
	}

	void shutdown() {
		_materialColors.clear();
		_colorMapping.clear();
		_initialized = false;
		_dirty = false;
	}

	inline void markClean() {
		_dirty = false;
	}

	inline bool isDirty() const {
		return _dirty;
	}

	inline const MaterialColorArray& getColors() const {
		core_assert_msg(_initialized, "Material colors are not yet initialized");
		core_assert_msg(!_materialColors.empty(), "Failed to initialize the material colors");
		return _materialColors;
	}

	inline const MaterialColorIndices& getColorIndices(VoxelType type) const {
		auto i = _colorMapping.find(type);
		if (i == _colorMapping.end()) {
			Log::warn("Could not find color indices for voxel type %s - use generic", VoxelTypeStr[(int)type]);
			i = _colorMapping.find(VoxelType::Generic);
			if (i == _colorMapping.end()) {
				Log::error("Could not find color indices for voxel type generic");
				static MaterialColorIndices Empty(0);
				return Empty;
			}
		}
		return i->second;
	}

	inline Voxel createColorVoxel(VoxelType type, uint32_t colorIndex) {
		core_assert_msg(_initialized, "Material colors are not yet initialized");
		uint8_t index = 0;
		if (type != VoxelType::Air) {
			auto i = _colorMapping.find(type);
			if (i == _colorMapping.end()) {
				Log::error("Failed to get color indices for voxel type %s", VoxelTypeStr[(int)type]);
			} else {
				const MaterialColorIndices& indices = i->second;
				if (indices.empty()) {
					Log::error("Failed to get color indices for voxel type %s", VoxelTypeStr[(int)type]);
				} else {
					colorIndex %= (uint32_t)indices.size();
					index = indices[colorIndex];
				}
			}
		}
		return voxel::createVoxel(type, index);
	}

	inline Voxel createRandomColorVoxel(VoxelType type, math::Random& random) const {
		core_assert_msg(_initialized, "Material colors are not yet initialized");
		uint8_t index = 0;
		if (type != VoxelType::Air) {
			auto i = _colorMapping.find(type);
			if (i == _colorMapping.end()) {
				Log::error("Failed to get color indices for voxel type %s", VoxelTypeStr[(int)type]);
			} else {
				const MaterialColorIndices& indices = i->second;
				if (indices.empty()) {
					Log::error("Failed to get color indices for voxel type %s", VoxelTypeStr[(int)type]);
				} else if (indices.size() == 1) {
					index = indices.front();
				} else {
					index = *random.randomElement(indices.begin(), indices.end());
				}
			}
		}
		return voxel::createVoxel(type, index);
	}
};

static MaterialColor& getInstance() {
	static MaterialColor color;
	return color;
}

bool initMaterialColors(const uint8_t* paletteBuffer, size_t paletteBufferSize, const core::String& luaBuffer) {
	return getInstance().init(paletteBuffer, paletteBufferSize, luaBuffer);
}

bool overrideMaterialColors(const uint8_t* paletteBuffer, size_t paletteBufferSize, const core::String& luaBuffer) {
	shutdownMaterialColors();
	if (!initMaterialColors(paletteBuffer, paletteBufferSize, luaBuffer)) {
		return initDefaultMaterialColors();
	}
	return true;
}

bool materialColorChanged() {
	return getInstance().isDirty();
}

void materialColorMarkClean() {
	getInstance().markClean();
}

void shutdownMaterialColors() {
	return getInstance().shutdown();
}

bool initMinecraftMaterialColors() {
	static const uint32_t palette[] = {
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
		0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xff242132
	};
	return getInstance().init((const uint8_t* )palette, sizeof(palette), "");
}

bool initMaterialColors(const io::FilePtr& paletteFile, const io::FilePtr& luaFile) {
	if (!paletteFile->exists()) {
		Log::error("%s doesn't exist", paletteFile->name().c_str());
		return false;
	}
	core::String luaString = "";
	if (luaFile) {
		if (!luaFile->exists()) {
			Log::warn("Failed to load %s", luaFile->name().c_str());
		} else {
			luaString = luaFile->load();
			if (luaString.empty()) {
				Log::warn("No lua material definitions in %s", luaFile->name().c_str());
			}
		}
	} else {
		Log::warn("No lua material definition file given");
	}
	const image::ImagePtr& img = image::loadImage(paletteFile, false);
	if (!img->isLoaded()) {
		Log::error("Failed to load image %s", paletteFile->name().c_str());
		return false;
	}
	return initMaterialColors(img->data(), img->width() * img->height() * img->depth(), luaString);
}

bool overrideMaterialColors(const io::FilePtr& paletteFile, const io::FilePtr& luaFile) {
	if (!paletteFile->exists()) {
		Log::error("%s doesn't exist", paletteFile->name().c_str());
		return false;
	}
	if (!luaFile->exists()) {
		Log::error("Failed to load %s", luaFile->name().c_str());
		return false;
	}
	const image::ImagePtr& img = image::loadImage(paletteFile, false);
	if (!img->isLoaded()) {
		Log::error("Failed to load image %s", paletteFile->name().c_str());
		return false;
	}
	return overrideMaterialColors(img->data(), img->width() * img->height() * img->depth(), luaFile->load());
}

const char* getDefaultPaletteName() {
	return "nippon";
}

core::String extractPaletteName(const core::String& file) {
	if (!core::string::startsWith(file, "palette-")) {
		return "";
	}
	const core::String& nameWithExtension = file.substr(8);
	const size_t extPos = nameWithExtension.rfind('.');
	if (extPos != core::String::npos) {
		return nameWithExtension.substr(0, extPos);
	}
	return nameWithExtension;
}

bool initDefaultMaterialColors() {
	const io::FilesystemPtr& filesystem = io::filesystem();
	const io::FilePtr& paletteFile = filesystem->open(core::string::format("palette-%s.png", getDefaultPaletteName()));
	const io::FilePtr& luaFile = filesystem->open(core::string::format("palette-%s.lua", getDefaultPaletteName()));
	return initMaterialColors(paletteFile, luaFile);
}

const MaterialColorArray& getMaterialColors() {
	return getInstance().getColors();
}

const glm::vec4& getMaterialColor(const Voxel& voxel) {
	return getMaterialColors()[voxel.getColor()];
}

const MaterialColorIndices& getMaterialIndices(VoxelType type) {
	return getInstance().getColorIndices(type);
}

Voxel createRandomColorVoxel(VoxelType type) {
	math::Random random;
	return createRandomColorVoxel(type, random);
}

Voxel createColorVoxel(VoxelType type, uint32_t colorIndex) {
	return getInstance().createColorVoxel(type, colorIndex);
}

Voxel createRandomColorVoxel(VoxelType type, math::Random& random) {
	return getInstance().createRandomColorVoxel(type, random);
}

bool createPalette(const image::ImagePtr& image, uint32_t *colorsBuffer, int colors) {
	if (!image || !image->isLoaded()) {
		return false;
	}
	const int imageWidth = image->width();
	const int imageHeight = image->height();
	Log::debug("Create palette for image: %s", image->name().c_str());
	core::Map<uint32_t, bool, 64> colorset;
	uint16_t paletteIndex = 0;
	uint32_t empty = core::Color::getRGBA(core::Color::White);
	colorset.put(empty, true);
	colorsBuffer[paletteIndex++] = empty;
	for (int x = 0; x < imageWidth; ++x) {
		for (int y = 0; y < imageHeight; ++y) {
			const uint8_t* data = image->at(x, y);
			const uint32_t rgba = core::Color::getRGBA(core::Color::alpha(core::Color::fromRGBA(*(uint32_t*)data), 1.0f));
			if (colorset.find(rgba) != colorset.end()) {
				continue;
			}
			colorset.put(rgba, true);
			if (paletteIndex >= colors) {
				Log::warn("Palette indices exceeded - only %i colors were loaded", colors);
				return true;
			}
			colorsBuffer[paletteIndex++] = rgba;
		}
	}
	for (int i = paletteIndex; i < colors; ++i) {
		colorsBuffer[i] = 0xFFFFFFFF;
	}
	return true;
}

bool createPaletteFile(const image::ImagePtr& image, const char *paletteFile) {
	if (!image || !image->isLoaded() || paletteFile == nullptr) {
		return false;
	}
	uint32_t buf[256];
	if (!createPalette(image, buf, lengthof(buf))) {
		return false;
	}
	const image::ImagePtr& paletteImg = image::createEmptyImage("**palette**");
	return paletteImg->writePng(paletteFile, (const uint8_t*)buf, 256, 1, 4);
}

}
