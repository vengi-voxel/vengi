/**
 * @file
 */

#include "MaterialColor.h"
#include "core/Singleton.h"
#include "core/App.h"
#include "math/Random.h"
#include "core/Color.h"
#include "core/GLM.h"
#include "core/io/Filesystem.h"
#include "commonlua/LUA.h"
#include "commonlua/LUAFunctions.h"
#include <unordered_map>
#include <algorithm>
#include <numeric>

namespace voxel {

class MaterialColor {
private:
	MaterialColorArray _materialColors;
	std::unordered_map<VoxelType, MaterialColorIndices, EnumClassHash> _colorMapping;
	bool _initialized = false;
	bool _dirty = false;
public:
	MaterialColor() {
	}

	bool initialized() const {
		return _initialized;
	}

	bool init(const uint8_t* paletteBuffer, size_t paletteBufferSize, const std::string& luaString) {
		if (_initialized) {
			Log::debug("MaterialColors are already initialized");
			return true;
		}
		_initialized = true;
		_dirty = true;
		const int colors = paletteBufferSize / 4;
		if (colors != 256) {
			Log::error("Palette image has invalid dimensions - we need 256x1(depth: 4)");
			return false;
		}
		_materialColors.reserve(colors);
		const uint32_t* paletteData = (const uint32_t*)paletteBuffer;
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

		if (luaString.empty()) {
			Log::warn("No materials defined in lua script");
			return true;
		}

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
		funcs.push_back(getmaterial);
		funcs.push_back({ nullptr, nullptr });
		lua.newGlobalData<MaterialColor>("MaterialColor", this);
		lua.reg("MAT", &funcs.front());
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
			const auto& e = _colorMapping[(voxel::VoxelType)j];
			if (e.empty()) {
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

bool initMaterialColors(const uint8_t* paletteBuffer, size_t paletteBufferSize, const std::string& luaBuffer) {
	return getInstance().init(paletteBuffer, paletteBufferSize, luaBuffer);
}

bool overrideMaterialColors(const uint8_t* paletteBuffer, size_t paletteBufferSize, const std::string& luaBuffer) {
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

bool initMaterialColors(const io::FilePtr& paletteFile, const io::FilePtr& luaFile) {
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
	return initMaterialColors(img->data(), img->width() * img->height() * img->depth(), luaFile->load());
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

std::string extractPaletteName(const std::string& file) {
	if (!core::string::startsWith(file, "palette-")) {
		return "";
	}
	const std::string& nameWithExtension = file.substr(8);
	const size_t extPos = nameWithExtension.rfind('.');
	if (extPos != std::string::npos) {
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
	std::unordered_set<uint32_t> colorset;
	uint16_t paletteIndex = 0;
	uint32_t empty = core::Color::getRGBA(core::Color::White);
	colorset.insert(empty);
	colorsBuffer[paletteIndex++] = empty;
	for (int x = 0; x < imageWidth; ++x) {
		for (int y = 0; y < imageHeight; ++y) {
			const uint8_t* data = image->at(x, y);
			const uint32_t rgba = core::Color::getRGBA(core::Color::alpha(core::Color::fromRGBA(*(uint32_t*)data), 1.0f));
			if (colorset.insert(rgba).second) {
				if (paletteIndex >= colors) {
					Log::warn("palette indices exceeded");
					return false;
				}
				colorsBuffer[paletteIndex++] = rgba;
			}
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
