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
#include "voxel/Palette.h"

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
	core::Map<VoxelType, MaterialColorIndices, 8, EnumClassHash> _colorMapping;
	bool _initialized = false;
	bool _dirty = false;
	Palette _palette;
public:

	bool initialized() const {
		return _initialized;
	}

	inline Palette &palette() {
		return _palette;
	}

	bool init(const Palette& palette) {
		if (_initialized) {
			Log::debug("MaterialColors are already initialized");
			return true;
		}
		_palette = palette;
		_initialized = true;
		_dirty = true;

		MaterialColorIndices generic;
		generic.reserve(_palette.colorCount);
		for (int i = 0; i < _palette.colorCount; ++i) {
			generic.push_back(i);
		}
		_colorMapping.put(voxel::VoxelType::Generic, generic);

		if (palette.lua.empty()) {
			Log::debug("No materials defined in palette lua script");
			return true;
		}

		constexpr size_t funcSize = 2u + ((size_t)(voxel::VoxelType::Max) - ((size_t)voxel::VoxelType::Air + 1u));
		core::Array<luaL_Reg, funcSize> funcs;
		static_assert((int)voxel::VoxelType::Air == 0, "Air must be 0");
		size_t aindex = 0u;
		for (int i = (int)voxel::VoxelType::Air + 1; i < (int)voxel::VoxelType::Max; ++i, ++aindex) {
			funcs[aindex] = luaL_Reg{ voxel::VoxelTypeStr[i], [] (lua_State* l) -> int {
				MaterialColor* mc = luamaterial_getmaterialcolor(l);
				const int index = (int)luaL_checknumber(l, -1);
				// this is hacky - but we resolve the lua function name here to reverse lookup
				// the voxel type. This could maybe be done nicer with upvalues...
				lua_Debug entry;
				lua_getstack(l, 0, &entry);
				lua_getinfo(l, "Sln", &entry);
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
			lua_newtable(l);
			const int top = lua_gettop(l);
			for (int i = 0; i < PaletteMaxColors; ++i) {
				lua_pushinteger(l, i);
				clua_push(l, core::Color::fromRGBA(mc->palette().colors[i]));
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
		if (!lua.load(palette.lua)) {
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

	const MaterialColorIndices& getColorIndices(VoxelType type) const {
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

	Voxel createColorVoxel(VoxelType type, uint32_t colorIndex) {
		core_assert_msg(_initialized, "Material colors are not yet initialized");
		uint8_t index = 0;
		if (type != VoxelType::Air) {
			auto i = _colorMapping.find(type);
			if (i == _colorMapping.end()) {
				Log::debug("Failed to find color indices for voxel type %s", VoxelTypeStr[(int)type]);
			} else {
				const MaterialColorIndices& indices = i->second;
				if (indices.empty()) {
					Log::debug("Failed to get color indices for voxel type %s", VoxelTypeStr[(int)type]);
				} else {
					colorIndex %= (uint32_t)indices.size();
					index = indices[colorIndex];
				}
			}
		}
		return voxel::createVoxel(type, index);
	}

	Voxel createRandomColorVoxel(VoxelType type, math::Random& random) const {
		core_assert_msg(_initialized, "Material colors are not yet initialized");
		uint8_t index = 0;
		if (type != VoxelType::Air) {
			auto i = _colorMapping.find(type);
			if (i == _colorMapping.end()) {
				Log::debug("Failed to find color indices for voxel type %s (random color)", VoxelTypeStr[(int)type]);
			} else {
				const MaterialColorIndices& indices = i->second;
				if (indices.empty()) {
					Log::debug("Failed to get color indices for voxel type %s (random color)", VoxelTypeStr[(int)type]);
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

bool initPalette(const Palette& palette) {
	return getInstance().init(palette);
}

void shutdownMaterialColors() {
	return getInstance().shutdown();
}

bool overridePalette(const voxel::Palette &palette) {
	shutdownMaterialColors();
	if (!initPalette(palette)) {
		return initDefaultPalette();
	}
	return true;
}

bool initDefaultPalette() {
	Palette palette;
	if (!palette.load(Palette::getDefaultPaletteName())) {
		if (!palette.magicaVoxel()) {
			return false;
		}
	}
	return initPalette(palette);
}

Palette& getPalette() {
	return getInstance().palette();
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

}
