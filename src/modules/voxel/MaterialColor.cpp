/**
 * @file
 */

#include "MaterialColor.h"
#include "image/Image.h"
#include "core/Singleton.h"
#include "core/App.h"
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
			_materialColors.emplace_back(core::Color::FromRGBA(SDL_SwapBE32(*paletteData)));
			++paletteData;
		}
		Log::info("Set up %i material colors", (int)_materialColors.size());

		if (_materialColors.size() != (size_t)colors) {
			return false;
		}

		struct IndexVectors {
			MaterialColorIndices water;
			MaterialColorIndices grass;
			MaterialColorIndices wood;
			MaterialColorIndices leaf;
			MaterialColorIndices leaffir;
			MaterialColorIndices leafpine;
			MaterialColorIndices flower;
			MaterialColorIndices bloom;
			MaterialColorIndices mushroom;
			MaterialColorIndices rock;
			MaterialColorIndices sand;
			MaterialColorIndices cloud;
			MaterialColorIndices dirt;
			MaterialColorIndices generic;
		} iv;
		iv.generic.resize(colors - 1);
		// 0 is VoxelType::Air - don't add it
		std::iota(std::begin(iv.generic), std::end(iv.generic), 1);

		lua::LUA lua;
#define LUA_ACCESS(name) \
	luaL_Reg add##name = { #name, [] (lua_State* l) -> int { \
		IndexVectors* v = lua::LUA::globalData<IndexVectors>(l, "indexvector"); \
		const int index = luaL_checknumber(l, -1); \
		core_assert_msg(v != nullptr, "Could not find globl indexvector"); \
		v->name.push_back(index); \
		/*v->generic.erase(std::remove(v->generic.begin(), v->generic.end(), index), v->generic.end());*/ \
		return 0; \
	}};
		LUA_ACCESS(water);
		LUA_ACCESS(grass);
		LUA_ACCESS(wood);
		LUA_ACCESS(flower);
		LUA_ACCESS(bloom);
		LUA_ACCESS(mushroom);
		LUA_ACCESS(leaf);
		LUA_ACCESS(leaffir);
		LUA_ACCESS(leafpine);
		LUA_ACCESS(rock);
		LUA_ACCESS(sand);
		LUA_ACCESS(cloud);
		LUA_ACCESS(dirt);

		luaL_Reg getmaterial = { "material", [] (lua_State* l) -> int {
			MaterialColor* mc = lua::LUA::globalData<MaterialColor>(l, "MaterialColor");
			lua_newtable(l);
			const MaterialColorArray& colors = mc->getColors();
			lua_newtable(l);
			const int top = lua_gettop(l);
			for (size_t i = 0; i < colors.size(); ++i) {
				lua_pushinteger(l, i);
				clua_push<glm::vec4>(l, colors[i]);
				lua_settable(l, top);
			}
			return 1;
		}};

		clua_vecregister<glm::vec4>(lua.state());

		luaL_Reg eof = { nullptr, nullptr };
		luaL_Reg funcs[] = { addwater, addgrass, addwood, addflower, addbloom,
				addmushroom, addleaf, addleaffir, addleafpine, addrock, addsand,
				addcloud, adddirt, getmaterial, eof };
		lua.newGlobalData<IndexVectors>("indexvector", &iv);
		lua.newGlobalData<MaterialColor>("MaterialColor", this);
		lua.reg("MAT", funcs);

#undef LUA_ACCESS
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

		_colorMapping[VoxelType::Water] = std::move(iv.water);
		_colorMapping[VoxelType::Grass] = std::move(iv.grass);
		_colorMapping[VoxelType::Wood] = std::move(iv.wood);
		_colorMapping[VoxelType::Leaf] = std::move(iv.leaf);
		_colorMapping[VoxelType::LeafFir] = std::move(iv.leaffir);
		_colorMapping[VoxelType::LeafPine] = std::move(iv.leafpine);
		_colorMapping[VoxelType::Rock] = std::move(iv.rock);
		_colorMapping[VoxelType::Flower] = std::move(iv.flower);
		_colorMapping[VoxelType::Bloom] = std::move(iv.bloom);
		_colorMapping[VoxelType::Mushroom] = std::move(iv.mushroom);
		_colorMapping[VoxelType::Sand] = std::move(iv.sand);
		_colorMapping[VoxelType::Cloud] = std::move(iv.cloud);
		_colorMapping[VoxelType::Dirt] = std::move(iv.dirt);
		_colorMapping[VoxelType::Generic] = std::move(iv.generic);

		for (const auto& e : _colorMapping) {
			if (e.second.empty()) {
				Log::error("No colors are defined for VoxelType: %i", (int)std::enum_value(e.first));
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
				Log::error("Failed to get color indices for voxel type %i", (int)type);
			} else {
				const MaterialColorIndices& indices = i->second;
				if (indices.empty()) {
					Log::error("Failed to get color indices for voxel type %i", (int)type);
				} else {
					colorIndex %= (uint32_t)indices.size();
					index = indices[colorIndex];
				}
			}
		}
		return voxel::createVoxel(type, index);
	}

	inline Voxel createRandomColorVoxel(VoxelType type, core::Random& random) const {
		core_assert_msg(_initialized, "Material colors are not yet initialized");
		uint8_t index = 0;
		if (type != VoxelType::Air) {
			auto i = _colorMapping.find(type);
			if (i == _colorMapping.end()) {
				Log::error("Failed to get color indices for voxel type %i", (int)type);
			} else {
				const MaterialColorIndices& indices = i->second;
				if (indices.empty()) {
					Log::error("Failed to get color indices for voxel type %i", (int)type);
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
	const io::FilePtr& paletteFile = core::App::getInstance()->filesystem()->open("palette-nippon.png");
	const io::FilePtr& luaFile = core::App::getInstance()->filesystem()->open("palette-nippon.lua");
	return initMaterialColors(paletteFile, luaFile);
}

const MaterialColorArray& getMaterialColors() {
	return getInstance().getColors();
}

const MaterialColorIndices& getMaterialIndices(VoxelType type) {
	return getInstance().getColorIndices(type);
}

Voxel createRandomColorVoxel(VoxelType type) {
	core::Random random;
	return createRandomColorVoxel(type, random);
}

Voxel createColorVoxel(VoxelType type, uint32_t colorIndex) {
	return getInstance().createColorVoxel(type, colorIndex);
}

Voxel createRandomColorVoxel(VoxelType type, core::Random& random) {
	return getInstance().createRandomColorVoxel(type, random);
}

}
