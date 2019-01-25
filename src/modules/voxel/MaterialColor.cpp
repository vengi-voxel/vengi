/**
 * @file
 */

#include "MaterialColor.h"
#include "image/Image.h"
#include "core/Singleton.h"
#include "core/App.h"
#include "math/Random.h"
#include "core/Color.h"
#include "core/GLM.h"
#include "io/Filesystem.h"
#include "commonlua/LUA.h"
#include "commonlua/LUAFunctions.h"
#include <unordered_map>
#include <algorithm>

namespace voxel {

class MaterialColor {
private:
	image::Image _image;
	MaterialColorArray _materialColors;
	std::unordered_map<VoxelType, MaterialColorIndices> _colorMapping;
	bool _initialized = false;
public:
	MaterialColor() :
			_image("**palette**") {
	}

	bool init(const io::FilePtr& paletteFile, const io::FilePtr& luaFile) {
		if (_initialized) {
			Log::debug("MaterialColors are already initialized");
			return true;
		}
		_initialized = true;
		if (!_image.load(paletteFile)) {
			Log::error("MaterialColors: failed to load image");
			return false;
		}
		if (!_image.isLoaded()) {
			Log::error("MaterialColors: image not fully loaded");
			return false;
		}
		const int colors = _image.width() * _image.height();
		if (colors != 256) {
			Log::error("Palette image has invalid dimensions: %i:%i", _image.width(), _image.height());
			return false;
		}
		if (_image.depth() != 4) {
			Log::error("Palette image has invalid depth: %i", _image.depth());
			return false;
		}
		_materialColors.reserve(colors);
		const uint32_t* paletteData = (const uint32_t*)_image.data();
		for (int i = 0; i < colors; ++i) {
			_materialColors.emplace_back(core::Color::fromRGBA(*paletteData));
			++paletteData;
		}
		Log::info("Set up %i material colors", (int)_materialColors.size());

		if (_materialColors.size() != (size_t)colors) {
			return false;
		}

		MaterialColorIndices& generic = _colorMapping[voxel::VoxelType::Generic];
		generic.resize(colors - 1);
		// 0 is VoxelType::Air - don't add it
		std::iota(std::begin(generic), std::end(generic), 1);

		std::vector<luaL_Reg> funcs;
		static_assert((int)voxel::VoxelType::Air == 0, "Air must be 0");
		for (int i = (int)voxel::VoxelType::Air + 1; i < (int)voxel::VoxelType::Max; ++i) {
			funcs.push_back(luaL_Reg{ voxel::VoxelTypeStr[i], [] (lua_State* l) -> int {
				MaterialColor* mc = lua::LUA::globalData<MaterialColor>(l, "MaterialColor");
				const int index = luaL_checknumber(l, -1);
				// this is hacky - but we resolve the lua function name here to reverse lookup
				// the voxel type. This could maybe be done nicer with upvalues...
				lua_Debug entry;
				lua_getstack(l, 0, &entry);
				const int status = lua_getinfo(l, "Sln", &entry);
				core_assert_always(status == 1);
				for (int j = (int)voxel::VoxelType::Air + 1; j < (int)voxel::VoxelType::Max; ++j) {
					if (!strcmp(voxel::VoxelTypeStr[j], entry.name)) {
						mc->_colorMapping[(VoxelType)j].push_back(index);
						break;
					}
				}
				return 0;
			}});
		}

		luaL_Reg getmaterial = { "material", [] (lua_State* l) -> int {
			MaterialColor* mc = lua::LUA::globalData<MaterialColor>(l, "MaterialColor");
			lua_newtable(l);
			const MaterialColorArray& colors = mc->getColors();
			lua_newtable(l);
			const int top = lua_gettop(l);
			for (size_t i = 0; i < colors.size(); ++i) {
				lua_pushinteger(l, i);
				clua_push(l, colors[i]);
				lua_settable(l, top);
			}
			return 1;
		}};

		lua::LUA lua;
		clua_vecregister<glm::vec4>(lua.state());
		funcs.push_back(getmaterial);
		funcs.push_back({ nullptr, nullptr });
		lua.newGlobalData<MaterialColor>("MaterialColor", this);
		lua.reg("MAT", &funcs.front());
		const std::string& luaString = luaFile->load();
		if (luaString.empty()) {
			Log::error("Could not load lua script file: %s", luaFile->fileName().c_str());
			return false;
		}
		if (!lua.load(luaString)) {
			Log::error("Could not load lua script: %s. Failed with error: %s",
					luaFile->fileName().c_str(), lua.error().c_str());
			return false;
		}
		if (!lua.execute("init")) {
			Log::error("Could not execute lua script file: %s. Failed with error: %s",
					luaFile->fileName().c_str(), lua.error().c_str());
			return false;
		}

		for (int j = (int)voxel::VoxelType::Air + 1; j < (int)voxel::VoxelType::Max; ++j) {
			const auto& e = _colorMapping[(voxel::VoxelType)j];
			if (e.empty()) {
				Log::error("No colors are defined for VoxelType: %s", voxel::VoxelTypeStr[j]);
				return false;
			}
		}

		return true;
	}

	inline const MaterialColorArray& getColors() const {
		core_assert_msg(_initialized, "Material colors are not yet initialized");
		core_assert_msg(!_materialColors.empty(), "Failed to initialize the material colors");
		return _materialColors;
	}

	inline const MaterialColorIndices& getColorIndices(VoxelType type) const {
		auto i = _colorMapping.find(type);
		if (i == _colorMapping.end()) {
			static MaterialColorIndices Empty(0);
			Log::warn("Could not find color indices for voxel type %s", VoxelTypeStr[(int)type]);
			return Empty;
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

bool initMaterialColors(const io::FilePtr& paletteFile, const io::FilePtr& luaFile) {
	if (!paletteFile->exists()) {
		Log::error("Failed to load %s", paletteFile->name().c_str());
		return false;
	}
	if (!luaFile->exists()) {
		Log::error("Failed to load %s", luaFile->name().c_str());
		return false;
	}
	return getInstance().init(paletteFile, luaFile);
}

bool initDefaultMaterialColors() {
	const io::FilesystemPtr& filesystem = core::App::getInstance()->filesystem();
	const io::FilePtr& paletteFile = filesystem->open("palette-nippon.png");
	const io::FilePtr& luaFile = filesystem->open("palette-nippon.lua");
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

}
