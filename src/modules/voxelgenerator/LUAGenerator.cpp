/**
 * @file
 */

#include "LUAGenerator.h"
#include "commonlua/LUAFunctions.h"
#include "core/StringUtil.h"
#include "image/Image.h"
#include "lauxlib.h"
#include "lua.h"
#include "math/Axis.h"
#include "voxel/MaterialColor.h"
#include "voxel/PaletteLookup.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Region.h"
#include "commonlua/LUA.h"
#include "voxel/Voxel.h"
#include "voxel/Palette.h"
#include "core/Color.h"
#include "io/Filesystem.h"
#include "noise/Simplex.h"
#include "app/App.h"
#include "voxelformat/SceneGraphUtil.h"
#include "voxelutil/ImageUtils.h"
#include "voxelutil/VolumeCropper.h"
#include "voxelutil/VolumeResizer.h"
#include "voxelformat/SceneGraph.h"
#include "voxelformat/SceneGraphNode.h"
#include "voxelutil/VolumeRotator.h"
#include "voxelutil/VoxelUtil.h"

#define GENERATOR_LUA_SANTITY 1

namespace voxelgenerator {

class LuaRawVolumeWrapper : public voxel::RawVolumeWrapper {
private:
	using Super = voxel::RawVolumeWrapper;
	voxelformat::SceneGraphNode *_node;
public:
	LuaRawVolumeWrapper(voxelformat::SceneGraphNode *node) : Super(node->volume()), _node(node) {
	}

	~LuaRawVolumeWrapper() {
		update();
	}

	voxelformat::SceneGraphNode *node() {
		return _node;
	}

	void update() {
		if (_node->volume() == volume()) {
			return;
		}
		_node->setVolume(volume(), true);
	}
};

static const char *luaVoxel_globalscenegraph() {
	return "__global_scenegraph";
}

static const char *luaVoxel_globalnodeid() {
	return "__global_nodeid";
}

static const char *luaVoxel_globalnoise() {
	return "__global_noise";
}

static const char *luaVoxel_globaldirtyregion() {
	return "__global_region";
}

static const char *luaVoxel_metascenegraphnode() {
	return "__meta_scenegraphnode";
}

static const char *luaVoxel_metascenegraph() {
	return "__meta_scenegraph";
}

static const char *luaVoxel_metavolumewrapper() {
	return "__meta_volumewrapper";
}

static const char *luaVoxel_metapalette() {
	return "__meta_palette";
}

static const char *luaVoxel_metanoise() {
	return "__meta_noise";
}

static inline const char *luaVoxel_metaregion() {
	return "__meta_region";
}

static voxel::Region* luaVoxel_toRegion(lua_State* s, int n) {
	return *(voxel::Region**)clua_getudata<voxel::Region*>(s, n, luaVoxel_metaregion());
}

static voxel::Palette* luaVoxel_toPalette(lua_State* s, int n) {
	return *(voxel::Palette**)clua_getudata<voxel::Palette*>(s, n, luaVoxel_metapalette());
}

static int luaVoxel_pushregion(lua_State* s, const voxel::Region* region) {
	if (region == nullptr) {
		return clua_error(s, "No region given - can't push");
	}
	return clua_pushudata(s, region, luaVoxel_metaregion());
}

static voxelformat::SceneGraphNode* luaVoxel_toscenegraphnode(lua_State* s, int n) {
	return *(voxelformat::SceneGraphNode**)clua_getudata<voxelformat::SceneGraphNode*>(s, n, luaVoxel_metascenegraphnode());
}

static int luaVoxel_pushscenegraphnode(lua_State* s, voxelformat::SceneGraphNode& node) {
	return clua_pushudata(s, &node, luaVoxel_metascenegraphnode());
}

static LuaRawVolumeWrapper* luaVoxel_tovolumewrapper(lua_State* s, int n) {
	return *(LuaRawVolumeWrapper**)clua_getudata<LuaRawVolumeWrapper*>(s, n, luaVoxel_metavolumewrapper());
}

static int luaVoxel_pushvolumewrapper(lua_State* s, voxelformat::SceneGraphNode* node) {
	if (node == nullptr) {
		return clua_error(s, "No node given - can't push");
	}
	LuaRawVolumeWrapper *wrapper = new LuaRawVolumeWrapper(node);
	return clua_pushudata(s, wrapper, luaVoxel_metavolumewrapper());
}

static int luaVoxel_pushpalette(lua_State* s, voxel::Palette* palette) {
	if (palette == nullptr) {
		return clua_error(s, "No palette given - can't push");
	}
	return clua_pushudata(s, palette, luaVoxel_metapalette());
}

static int luaVoxel_volumewrapper_voxel(lua_State* s) {
	const LuaRawVolumeWrapper* volume = luaVoxel_tovolumewrapper(s, 1);
	const int x = (int)luaL_checkinteger(s, 2);
	const int y = (int)luaL_checkinteger(s, 3);
	const int z = (int)luaL_checkinteger(s, 4);
	const voxel::Voxel& voxel = volume->voxel(x, y, z);
	if (voxel::isAir(voxel.getMaterial())) {
		lua_pushinteger(s, -1);
	} else {
		lua_pushinteger(s, voxel.getColor());
	}
	return 1;
}

static int luaVoxel_volumewrapper_region(lua_State* s) {
	const LuaRawVolumeWrapper* volume = luaVoxel_tovolumewrapper(s, 1);
	return luaVoxel_pushregion(s, &volume->region());
}

static int luaVoxel_volumewrapper_translate(lua_State* s) {
	LuaRawVolumeWrapper* volume = luaVoxel_tovolumewrapper(s, 1);
	const int x = (int)luaL_checkinteger(s, 2);
	const int y = (int)luaL_optinteger(s, 3, 0);
	const int z = (int)luaL_optinteger(s, 4, 0);
	volume->node()->translate(glm::ivec3(x, y, z));
	return 0;
}

static int luaVoxel_volumewrapper_resize(lua_State *s) {
	LuaRawVolumeWrapper *volume = luaVoxel_tovolumewrapper(s, 1);
	const int w = (int)luaL_checkinteger(s, 2);
	const int h = (int)luaL_optinteger(s, 3, 0);
	const int d = (int)luaL_optinteger(s, 4, 0);
	const bool extendMins = (int)clua_optboolean(s, 5, false);
	voxel::RawVolume *v = voxelutil::resize(volume->volume(), glm::ivec3(w, h, d), extendMins);
	if (v != nullptr) {
		volume->setVolume(v);
		volume->update();
	}
	return 0;
}

static voxel::Voxel luaVoxel_getVoxel(lua_State *s, int index, int defaultColor = 1) {
	const int color = (int)luaL_optinteger(s, index, defaultColor);
	if (color == -1) {
		return voxel::createVoxel(voxel::VoxelType::Air, 0);
	}
	return voxel::createVoxel(voxel::VoxelType::Generic, color);
}

static math::Axis luaVoxel_getAxis(lua_State *s, int index) {
	const char* axis = luaL_optstring(s, index, "y");
	return math::toAxis(axis);
}

static int luaVoxel_volumewrapper_mirroraxis(lua_State *s) {
	LuaRawVolumeWrapper *volume = luaVoxel_tovolumewrapper(s, 1);
	voxel::RawVolume* v = voxelutil::mirrorAxis(volume->volume(), luaVoxel_getAxis(s, 2));
	if (v != nullptr) {
		volume->setVolume(v);
		volume->update();
	}
	return 0;
}

static int luaVoxel_volumewrapper_rotateaxis(lua_State *s) {
	LuaRawVolumeWrapper *volume = luaVoxel_tovolumewrapper(s, 1);
	voxel::RawVolume* v = voxelutil::rotateAxis(volume->volume(), luaVoxel_getAxis(s, 2));
	if (v != nullptr) {
		volume->setVolume(v);
		volume->update();
	}
	return 0;
}

static int luaVoxel_volumewrapper_fillhollow(lua_State *s) {
	LuaRawVolumeWrapper *volume = luaVoxel_tovolumewrapper(s, 1);
	const voxel::Voxel voxel = luaVoxel_getVoxel(s, 2);
	voxelutil::fillHollow(*volume, voxel);
	return 0;
}

static int luaVoxel_volumewrapper_importheightmap(lua_State *s) {
	LuaRawVolumeWrapper *volume = luaVoxel_tovolumewrapper(s, 1);
	const core::String imageName = lua_tostring(s, 2);
	const image::ImagePtr &image = image::loadImage(imageName, false);
	if (!image || !image->isLoaded()) {
		return clua_error(s, "Image %s could not get loaded", imageName.c_str());
	}
	const voxel::Voxel dirt = voxel::createColorVoxel(voxel::VoxelType::Dirt, 0);
	const voxel::Voxel underground = luaVoxel_getVoxel(s, 3, dirt.getColor());
	const voxel::Voxel grass = voxel::createColorVoxel(voxel::VoxelType::Grass, 0);
	const voxel::Voxel surface = luaVoxel_getVoxel(s, 4, grass.getColor());
	voxelutil::importHeightmap(*volume, image, underground, surface);
	return 0;
}

static int luaVoxel_volumewrapper_importcoloredheightmap(lua_State *s) {
	LuaRawVolumeWrapper *volume = luaVoxel_tovolumewrapper(s, 1);
	const core::String imageName = lua_tostring(s, 2);
	const image::ImagePtr &image = image::loadImage(imageName, false);
	if (!image || !image->isLoaded()) {
		return clua_error(s, "Image %s could not get loaded", imageName.c_str());
	}
	voxel::PaletteLookup palLookup(voxel::getPalette());
	const voxel::Voxel dirt = voxel::createColorVoxel(voxel::VoxelType::Dirt, 0);
	const voxel::Voxel underground = luaVoxel_getVoxel(s, 3, dirt.getColor());
	voxelutil::importColoredHeightmap(*volume, palLookup, image, underground);
	return 0;
}

static int luaVoxel_volumewrapper_crop(lua_State *s) {
	LuaRawVolumeWrapper *volume = luaVoxel_tovolumewrapper(s, 1);
	voxel::RawVolume* v = voxelutil::cropVolume(volume->volume());
	if (v != nullptr) {
		volume->setVolume(v);
		volume->update();
	}
	return 0;
}

static int luaVoxel_volumewrapper_setvoxel(lua_State* s) {
	LuaRawVolumeWrapper* volume = luaVoxel_tovolumewrapper(s, 1);
	const int x = (int)luaL_checkinteger(s, 2);
	const int y = (int)luaL_checkinteger(s, 3);
	const int z = (int)luaL_checkinteger(s, 4);
	const voxel::Voxel voxel = luaVoxel_getVoxel(s, 5);
	const bool insideRegion = volume->setVoxel(x, y, z, voxel);
	lua_pushboolean(s, insideRegion ? 1 : 0);
	return 1;
}

static int luaVoxel_volumewrapper_gc(lua_State *s) {
	LuaRawVolumeWrapper* volume = luaVoxel_tovolumewrapper(s, 1);
	if (volume->dirtyRegion().isValid()) {
		voxel::Region* dirtyRegion = lua::LUA::globalData<voxel::Region>(s, luaVoxel_globaldirtyregion());
		if (dirtyRegion->isValid()) {
			dirtyRegion->accumulate(volume->dirtyRegion());
		} else {
			*dirtyRegion = volume->dirtyRegion();
		}
	}
	delete volume;
	return 0;
}

static int luaVoxel_palette_colors(lua_State* s) {
	const voxel::Palette &palette = voxel::getPalette();
	lua_createtable(s, palette.colorCount, 0);
	for (int i = 0; i < palette.colorCount; ++i) {
		const glm::vec4& c = core::Color::fromRGBA(palette.colors[i]);
		lua_pushinteger(s, i + 1);
		clua_push(s, c);
		lua_settable(s, -3);
	}
	return 1;
}

static int luaVoxel_palette_color(lua_State* s) {
	const uint8_t color = luaL_checkinteger(s, 1);
	const voxel::Palette &palette = voxel::getPalette();
	const glm::vec4& rgba = core::Color::fromRGBA(palette.colors[color]);
	return clua_push(s, rgba);
}

static int luaVoxel_palette_closestmatch(lua_State* s) {
	const voxel::Palette &palette = voxel::getPalette();
	core::DynamicArray<glm::vec4> materialColors;
	palette.toVec4f(materialColors);
	const float r = (float)luaL_checkinteger(s, 1) / 255.0f;
	const float g = (float)luaL_checkinteger(s, 2) / 255.0f;
	const float b = (float)luaL_checkinteger(s, 3) / 255.0f;
	const int match = core::Color::getClosestMatch(glm::vec4(r, b, g, 1.0f), materialColors);
	if (match < 0 || match > palette.colorCount) {
		return clua_error(s, "Given color index is not valid or palette is not loaded");
	}
	lua_pushinteger(s, match);
	return 1;
}

static int luaVoxel_palette_similar(lua_State* s) {
	const int paletteIndex = lua_tointeger(s, 1);
	const int colorCount = lua_tointeger(s, 2);
	const voxel::Palette &palette = voxel::getPalette();
	if (paletteIndex < 0 || paletteIndex >= palette.colorCount) {
		return clua_error(s, "Palette index out of bounds");
	}
	core::DynamicArray<glm::vec4> colors;
	palette.toVec4f(colors);
	const core::DynamicArray<glm::vec4> materialColors = colors;
	const glm::vec4 color = colors[paletteIndex];
	voxel::MaterialColorIndices newColorIndices;
	newColorIndices.resize(colorCount);
	int maxColorIndices = 0;
	colors.erase(paletteIndex);
	for (; maxColorIndices < colorCount; ++maxColorIndices) {
		const int index = core::Color::getClosestMatch(color, colors);
		if (index <= 0) {
			break;
		}
		const glm::vec4& c = colors[index];
		const int materialIndex = core::Color::getClosestMatch(c, materialColors);
		colors.erase(index);
		newColorIndices[maxColorIndices] = materialIndex;
	}
	if (maxColorIndices <= 0) {
		lua_pushnil(s);
		return 1;
	}

	lua_createtable(s, (int)newColorIndices.size(), 0);
	for (size_t i = 0; i < newColorIndices.size(); ++i) {
		lua_pushinteger(s, (int)i + 1);
		lua_pushinteger(s, newColorIndices[i]);
		lua_settable(s, -3);
	}

	return 1;
}

static int luaVoxel_region_width(lua_State* s) {
	const voxel::Region* region = luaVoxel_toRegion(s, 1);
	lua_pushinteger(s, region->getWidthInVoxels());
	return 1;
}

static int luaVoxel_region_height(lua_State* s) {
	const voxel::Region* region = luaVoxel_toRegion(s, 1);
	lua_pushinteger(s, region->getHeightInVoxels());
	return 1;
}

static int luaVoxel_region_depth(lua_State* s) {
	const voxel::Region* region = luaVoxel_toRegion(s, 1);
	lua_pushinteger(s, region->getDepthInVoxels());
	return 1;
}

static int luaVoxel_region_x(lua_State* s) {
	const voxel::Region* region = luaVoxel_toRegion(s, 1);
	lua_pushinteger(s, region->getLowerX());
	return 1;
}

static int luaVoxel_region_y(lua_State* s) {
	const voxel::Region* region = luaVoxel_toRegion(s, 1);
	lua_pushinteger(s, region->getLowerY());
	return 1;
}

static int luaVoxel_region_z(lua_State* s) {
	const voxel::Region* region = luaVoxel_toRegion(s, 1);
	lua_pushinteger(s, region->getLowerZ());
	return 1;
}

static int luaVoxel_region_center(lua_State* s) {
	const voxel::Region* region = luaVoxel_toRegion(s, 1);
	clua_push(s, region->getCenter());
	return 1;
}

static int luaVoxel_region_mins(lua_State* s) {
	const voxel::Region* region = luaVoxel_toRegion(s, 1);
	clua_push(s, region->getLowerCorner());
	return 1;
}

static int luaVoxel_region_maxs(lua_State* s) {
	const voxel::Region* region = luaVoxel_toRegion(s, 1);
	clua_push(s, region->getUpperCorner());
	return 1;
}

static int luaVoxel_region_size(lua_State* s) {
	const voxel::Region* region = luaVoxel_toRegion(s, 1);
	clua_push(s, region->getDimensionsInVoxels());
	return 1;
}

static int luaVoxel_region_setmins(lua_State* s) {
	voxel::Region* region = luaVoxel_toRegion(s, 1);
	const glm::ivec3& mins = clua_tovec<glm::ivec3>(s, 2);
	region->setLowerCorner(mins);
	return 0;
}

static int luaVoxel_region_setmaxs(lua_State* s) {
	voxel::Region* region = luaVoxel_toRegion(s, 1);
	const glm::ivec3& maxs = clua_tovec<glm::ivec3>(s, 2);
	region->setUpperCorner(maxs);
	return 0;
}

static int luaVoxel_region_tostring(lua_State *s) {
	const voxel::Region* region = luaVoxel_toRegion(s, 1);
	const glm::ivec3& mins = region->getLowerCorner();
	const glm::ivec3& maxs = region->getUpperCorner();
	lua_pushfstring(s, "region: [%i:%i:%i]/[%i:%i:%i]", mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);
	return 1;
}

static glm::vec2 to_vec2(lua_State*s, int &n) {
	if (clua_isvec<glm::vec2>(s, n)) {
		return clua_tovec<glm::vec2>(s, n);
	}
	const float x = (float)lua_tonumber(s, n++);
	const float y = (float)luaL_optnumber(s, n++, x);
	n += 1;
	return glm::vec2(x, y);
}

static glm::vec3 to_vec3(lua_State*s, int &n) {
	if (clua_isvec<glm::vec3>(s, n)) {
		return clua_tovec<glm::vec3>(s, n);
	}
	const float x = (float)lua_tonumber(s, n++);
	const float y = (float)luaL_optnumber(s, n++, x);
	const float z = (float)luaL_optnumber(s, n++, y);
	n += 2;
	return glm::vec3(x, y, z);
}

static glm::vec4 to_vec4(lua_State*s, int &n) {
	if (clua_isvec<glm::vec4>(s, n)) {
		return clua_tovec<glm::vec4>(s, n);
	}
	const float x = (float)lua_tonumber(s, n++);
	const float y = (float)luaL_optnumber(s, n++, x);
	const float z = (float)luaL_optnumber(s, n++, y);
	const float w = (float)luaL_optnumber(s, n++, z);
	n += 3;
	return glm::vec4(x, y, z, w);
}

static int luaVoxel_noise_simplex2(lua_State* s) {
	int n = 1;
	lua_pushnumber(s, noise::noise(to_vec2(s, n)));
	return 1;
}

static int luaVoxel_noise_simplex3(lua_State* s) {
	int n = 1;
	lua_pushnumber(s, noise::noise(to_vec3(s, n)));
	return 1;
}

static int luaVoxel_noise_simplex4(lua_State* s) {
	int n = 1;
	lua_pushnumber(s, noise::noise(to_vec4(s, n)));
	return 1;
}

static int luaVoxel_noise_fBm2(lua_State* s) {
	int n = 1;
	const uint8_t octaves = luaL_optinteger(s, n + 1, 4);
	const float lacunarity = (float)luaL_optnumber(s, n + 2, 2.0f);
	const float gain = (float)luaL_optnumber(s, n + 3, 0.5f);
	lua_pushnumber(s, noise::fBm(to_vec2(s, n), octaves, lacunarity, gain));
	return 1;
}

static int luaVoxel_noise_fBm3(lua_State* s) {
	int n = 1;
	const uint8_t octaves = luaL_optinteger(s, n + 1, 4);
	const float lacunarity = (float)luaL_optnumber(s, n + 2, 2.0f);
	const float gain = (float)luaL_optnumber(s, n + 3, 0.5f);
	lua_pushnumber(s, noise::fBm(to_vec3(s, n), octaves, lacunarity, gain));
	return 1;
}

static int luaVoxel_noise_fBm4(lua_State* s) {
	int n = 1;
	const uint8_t octaves = luaL_optinteger(s, n + 1, 4);
	const float lacunarity = (float)luaL_optnumber(s, n + 2, 2.0f);
	const float gain = (float)luaL_optnumber(s, n + 3, 0.5f);
	lua_pushnumber(s, noise::fBm(to_vec4(s, n), octaves, lacunarity, gain));
	return 1;
}

static int luaVoxel_noise_voronoi(lua_State* s) {
	noise::Noise* noise = lua::LUA::globalData<noise::Noise>(s, luaVoxel_globalnoise());
	int n = 1;
	const glm::vec3 v = to_vec3(s, n);
	const float frequency = (float)luaL_optnumber(s, n + 1, 1.0f);
	const int seed = (int)luaL_optinteger(s, n + 2, 0);
	bool enableDistance = clua_optboolean(s, n + 3, true);
	lua_pushnumber(s, noise->voronoi(v, enableDistance, frequency, seed));
	return 1;
}

static int luaVoxel_noise_swissturbulence(lua_State* s) {
	noise::Noise* noise = lua::LUA::globalData<noise::Noise>(s, luaVoxel_globalnoise());
	int n = 1;
	const glm::vec2 v = to_vec2(s, n);
	const float offset = (float)luaL_optnumber(s, n + 1, 1.0f);
	const uint8_t octaves = luaL_optinteger(s, n + 2, 4);
	const float lacunarity = (float)luaL_optnumber(s, n + 3, 2.0f);
	const float gain = (float)luaL_optnumber(s, n + 4, 0.6f);
	const float warp = (float)luaL_optnumber(s, n + 5, 0.15f);
	lua_pushnumber(s, noise->swissTurbulence(v, offset, octaves, lacunarity, gain, warp));
	return 1;
}

static int luaVoxel_noise_ridgedMF2(lua_State* s) {
	int n = 1;
	const glm::vec2 v = to_vec2(s, n);
	const float ridgeOffset = (float)luaL_optnumber(s, n + 1, 1.0f);
	const uint8_t octaves = luaL_optinteger(s, n + 2, 4);
	const float lacunarity = (float)luaL_optnumber(s, n + 3, 2.0f);
	const float gain = (float)luaL_optnumber(s, n + 4, 0.5f);
	lua_pushnumber(s, noise::ridgedMF(v, ridgeOffset, octaves, lacunarity, gain));
	return 1;
}

static int luaVoxel_noise_ridgedMF3(lua_State* s) {
	int n = 1;
	const glm::vec3 v = to_vec3(s, n);
	const float ridgeOffset = (float)luaL_optnumber(s, n + 1, 1.0f);
	const uint8_t octaves = luaL_optinteger(s, n + 2, 4);
	const float lacunarity = (float)luaL_optnumber(s, n + 3, 2.0f);
	const float gain = (float)luaL_optnumber(s, n + 4, 0.5f);
	lua_pushnumber(s, noise::ridgedMF(v, ridgeOffset, octaves, lacunarity, gain));
	return 1;
}

static int luaVoxel_noise_ridgedMF4(lua_State* s) {
	int n = 1;
	const glm::vec4 v = to_vec4(s, n);
	const float ridgeOffset = (float)luaL_optnumber(s, n + 1, 1.0f);
	const uint8_t octaves = luaL_optinteger(s, n + 2, 4);
	const float lacunarity = (float)luaL_optnumber(s, n + 3, 2.0f);
	const float gain = (float)luaL_optnumber(s, n + 4, 0.5f);
	lua_pushnumber(s, noise::ridgedMF(v, ridgeOffset, octaves, lacunarity, gain));
	return 1;
}

static int luaVoxel_noise_worley2(lua_State* s) {
	int n = 1;
	lua_pushnumber(s, noise::worleyNoise(to_vec2(s, n)));
	return 1;
}

static int luaVoxel_noise_worley3(lua_State* s) {
	int n = 1;
	lua_pushnumber(s, noise::worleyNoise(to_vec3(s, n)));
	return 1;
}

static int luaVoxel_scenegraph_new_node(lua_State* s) {
	const char *name = lua_tostring(s, 1);
	const voxel::Region* region = voxelgenerator::luaVoxel_toRegion(s, 2);
	const bool visible = clua_optboolean(s, 3, true);
	voxel::RawVolume *v = new voxel::RawVolume(*region);
	voxelformat::SceneGraphNode node;
	node.setVolume(v, true);
	node.setName(name);
	node.setVisible(visible);
	const glm::vec3 rp = v->region().getPivot();
	const glm::vec3 size = v->region().getDimensionsInVoxels();
	node.setPivot(0, rp, size);
	voxelformat::SceneGraph* sceneGraph = lua::LUA::globalData<voxelformat::SceneGraph>(s, luaVoxel_globalscenegraph());
	int* currentNodeId = lua::LUA::globalData<int>(s, luaVoxel_globalnodeid());
	const int nodeId = voxelformat::addNodeToSceneGraph(*sceneGraph, node, *currentNodeId);
	if (nodeId == -1) {
		return clua_error(s, "Failed to add new node");
	}

	return luaVoxel_pushscenegraphnode(s, sceneGraph->node(nodeId));
}

static int luaVoxel_scenegraph_get_node(lua_State* s) {
	int nodeId = (int)luaL_optinteger(s, 1, -1);
	voxelformat::SceneGraph* sceneGraph = lua::LUA::globalData<voxelformat::SceneGraph>(s, luaVoxel_globalscenegraph());
	if (nodeId == -1) {
		nodeId = sceneGraph->activeNode();
	}
	if (!sceneGraph->hasNode(nodeId)) {
		return clua_error(s, "Could not find node for id %d", nodeId);
	}
	voxelformat::SceneGraphNode& node = sceneGraph->node(nodeId);
	if (node.type() != voxelformat::SceneGraphNodeType::Model) {
		return clua_error(s, "Invalid node for id %d", nodeId);
	}
	return luaVoxel_pushscenegraphnode(s, node);
}

static int luaVoxel_scenegraphnode_volume(lua_State* s) {
	voxelformat::SceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	return luaVoxel_pushvolumewrapper(s, node);
}

static int luaVoxel_scenegraphnode_palette(lua_State* s) {
	voxelformat::SceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	voxel::Palette &palette = node->palette();
	return luaVoxel_pushpalette(s, &palette);
}

static int luaVoxel_scenegraphnode_name(lua_State* s) {
	voxelformat::SceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	lua_pushstring(s, node->name().c_str());
	return 1;
}

static int luaVoxel_scenegraphnode_setname(lua_State* s) {
	voxelformat::SceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	const char *newName = lua_tostring(s, 2);
	node->setName(newName);
	return 0;
}

static int luaVoxel_scenegraphnode_setpalette(lua_State* s) {
	voxelformat::SceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	voxel::Palette *palette = luaVoxel_toPalette(s, 2);
	node->setPalette(*palette);
	return 0;
}

static int luaVoxel_scenegraphnode_tostring(lua_State *s) {
	voxelformat::SceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	lua_pushfstring(s, "layer: [%d, %s]", node->id(), node->name().c_str());
	return 1;
}

static void prepareState(lua_State* s) {
	luaL_Reg volumeFuncs[] = {
		{"voxel", luaVoxel_volumewrapper_voxel},
		{"region", luaVoxel_volumewrapper_region},
		{"translate", luaVoxel_volumewrapper_translate},
		{"resize", luaVoxel_volumewrapper_resize},
		{"crop", luaVoxel_volumewrapper_crop},
		{"fillHollows", luaVoxel_volumewrapper_fillhollow},
		{"importHeightmap", luaVoxel_volumewrapper_importheightmap},
		{"importColoredHeightmap", luaVoxel_volumewrapper_importcoloredheightmap},
		{"mirrorAxis", luaVoxel_volumewrapper_mirroraxis},
		{"rotateAxis", luaVoxel_volumewrapper_rotateaxis},
		{"setVoxel", luaVoxel_volumewrapper_setvoxel},
		{"__gc", luaVoxel_volumewrapper_gc},
		{nullptr, nullptr}
	};
	clua_registerfuncs(s, volumeFuncs, luaVoxel_metavolumewrapper());

	static const luaL_Reg regionFuncs[] = {
		{"width", luaVoxel_region_width},
		{"height", luaVoxel_region_height},
		{"depth", luaVoxel_region_depth},
		{"x", luaVoxel_region_x},
		{"y", luaVoxel_region_y},
		{"z", luaVoxel_region_z},
		{"center", luaVoxel_region_center},
		{"mins", luaVoxel_region_mins},
		{"maxs", luaVoxel_region_maxs},
		{"size", luaVoxel_region_size},
		{"setMins", luaVoxel_region_setmins},
		{"setMaxs", luaVoxel_region_setmaxs},
		{"__tostring", luaVoxel_region_tostring},
		{nullptr, nullptr}
	};
	clua_registerfuncs(s, regionFuncs, luaVoxel_metaregion());

	static const luaL_Reg sceneGraphFuncs[] = {
		{"new", luaVoxel_scenegraph_new_node},
		{"get", luaVoxel_scenegraph_get_node},
		{nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, sceneGraphFuncs, luaVoxel_metascenegraph(), "scenegraph");

	static const luaL_Reg sceneGraphNodeFuncs[] = {
		{"name", luaVoxel_scenegraphnode_name},
		{"volume", luaVoxel_scenegraphnode_volume},
		{"palette", luaVoxel_scenegraphnode_palette},
		{"setName", luaVoxel_scenegraphnode_setname},
		{"setPalette", luaVoxel_scenegraphnode_setpalette},
		{"__tostring", luaVoxel_scenegraphnode_tostring},
		{nullptr, nullptr}
	};
	clua_registerfuncs(s, sceneGraphNodeFuncs, luaVoxel_metascenegraphnode());

	static const luaL_Reg paletteFuncs[] = {
		{"colors", luaVoxel_palette_colors},
		{"color", luaVoxel_palette_color},
		{"match", luaVoxel_palette_closestmatch},
		{"similar", luaVoxel_palette_similar},
		{nullptr, nullptr}
	};
	clua_registerfuncs(s, paletteFuncs, luaVoxel_metapalette());

	static const luaL_Reg noiseFuncs[] = {
		{"noise2", luaVoxel_noise_simplex2},
		{"noise3", luaVoxel_noise_simplex3},
		{"noise4", luaVoxel_noise_simplex4},
		{"fBm2", luaVoxel_noise_fBm2},
		{"fBm3", luaVoxel_noise_fBm3},
		{"fBm4", luaVoxel_noise_fBm4},
		{"swissTurbulence", luaVoxel_noise_swissturbulence},
		{"voronoi", luaVoxel_noise_voronoi},
		{"ridgedMF2", luaVoxel_noise_ridgedMF2},
		{"ridgedMF3", luaVoxel_noise_ridgedMF3},
		{"ridgedMF4", luaVoxel_noise_ridgedMF4},
		{"worley2", luaVoxel_noise_worley2},
		{"worley3", luaVoxel_noise_worley3},
		{nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, noiseFuncs, luaVoxel_metanoise(), "noise");

	clua_mathregister(s);
}

bool LUAGenerator::init() {
	if (!_noise.init()) {
		Log::warn("Failed to initialize noise");
	}
	return true;
}

void LUAGenerator::shutdown() {
	_noise.shutdown();
}

bool LUAGenerator::argumentInfo(const core::String& luaScript, core::DynamicArray<LUAParameterDescription>& params) {
	lua::LUA lua;

	// load and run once to initialize the global variables
	if (luaL_dostring(lua, luaScript.c_str())) {
		Log::error("%s", lua_tostring(lua, -1));
		return false;
	}

	const int preTop = lua_gettop(lua);

	// get help method
	lua_getglobal(lua, "arguments");
	if (!lua_isfunction(lua, -1)) {
		// this is no error - just no parameters are needed...
		return true;
	}

	const int error = lua_pcall(lua, 0, LUA_MULTRET, 0);
	if (error != LUA_OK) {
		Log::error("LUA generate arguments script: %s", lua_isstring(lua, -1) ? lua_tostring(lua, -1) : "Unknown Error");
		return false;
	}

	const int top = lua_gettop(lua);
	if (top <= preTop) {
		return true;
	}

	if (!lua_istable(lua, -1)) {
		Log::error("Expected to get a table return value");
		return false;
	}

	const int args = (int)lua_rawlen(lua, -1);

	for (int i = 0; i < args; ++i) {
		lua_pushinteger(lua, i + 1); // lua starts at 1
		lua_gettable(lua, -2);
		if (!lua_istable(lua, -1)) {
			Log::error("Expected to return tables of { name = 'name', desc = 'description', type = 'int' } at %i", i);
			return false;
		}

		core::String name = "";
		core::String description = "";
		core::String defaultValue = "";
		core::String enumValues = "";
		double minValue = 0.0;
		double maxValue = 100.0;
		LUAParameterType type = LUAParameterType::Max;
		lua_pushnil(lua);					// push nil, so lua_next removes it from stack and puts (k, v) on stack
		while (lua_next(lua, -2) != 0) {	// -2, because we have table at -1
			if (!lua_isstring(lua, -1) || !lua_isstring(lua, -2)) {
				Log::error("Expected to find string as parameter key and value");
				// only store stuff with string key and value
				return false;
			}
			const char *key = lua_tostring(lua, -2);
			const char *value = lua_tostring(lua, -1);
			if (!SDL_strcmp(key, "name")) {
				name = value;
			} else if (!SDL_strncmp(key, "desc", 4)) {
				description = value;
			} else if (!SDL_strncmp(key, "enum", 4)) {
				enumValues = value;
			} else if (!SDL_strcmp(key, "default")) {
				defaultValue = value;
			} else if (!SDL_strcmp(key, "min")) {
				minValue = SDL_atof(value);
			} else if (!SDL_strcmp(key, "max")) {
				maxValue = SDL_atof(value);
			} else if (!SDL_strcmp(key, "type")) {
				if (!SDL_strcmp(value, "int")) {
					type = LUAParameterType::Integer;
				} else if (!SDL_strcmp(value, "float")) {
					type = LUAParameterType::Float;
				} else if (!SDL_strcmp(value, "colorindex")) {
					type = LUAParameterType::ColorIndex;
				} else if (!SDL_strncmp(value, "str", 3)) {
					type = LUAParameterType::String;
				} else if (!SDL_strncmp(value, "enum", 4)) {
					type = LUAParameterType::Enum;
				} else if (!SDL_strncmp(value, "bool", 4)) {
					type = LUAParameterType::Boolean;
				} else {
					Log::error("Invalid type found: %s", value);
					return false;
				}
			} else {
				Log::warn("Invalid key found: %s", key);
			}
			lua_pop(lua, 1); // remove value, keep key for lua_next
		}

		if (name.empty()) {
			Log::error("No name = 'myname' key given");
			return false;
		}

		if (type == LUAParameterType::Max) {
			Log::error("No type = 'int', 'float', 'str', 'bool', 'enum' or 'colorindex' key given for '%s'", name.c_str());
			return false;
		}

		if (type == LUAParameterType::Enum && enumValues.empty()) {
			Log::error("No enum property given for argument '%s', but type is 'enum'", name.c_str());
			return false;
		}

		params.emplace_back(name, description, defaultValue, enumValues, minValue, maxValue, type);
		lua_pop(lua, 1); // remove table
	}
	return true;
}

static bool luaVoxel_pushargs(lua_State* s, const core::DynamicArray<core::String>& args, const core::DynamicArray<LUAParameterDescription>& argsInfo) {
	for (size_t i = 0u; i < argsInfo.size(); ++i) {
		const LUAParameterDescription &d = argsInfo[i];
		const core::String &arg = args.size() > i ? args[i] : d.defaultValue;
		switch (d.type) {
		case LUAParameterType::Enum:
		case LUAParameterType::String:
			lua_pushstring(s, arg.c_str());
			break;
		case LUAParameterType::Boolean: {
			const bool val = arg == "1" || arg == "true";
			lua_pushboolean(s, val ? 1 : 0);
			break;
		}
		case LUAParameterType::ColorIndex:
		case LUAParameterType::Integer:
			lua_pushinteger(s, glm::clamp(core::string::toInt(arg), (int)d.minValue, (int)d.maxValue));
			break;
		case LUAParameterType::Float:
			lua_pushnumber(s, glm::clamp(core::string::toFloat(arg), (float)d.minValue, (float)d.maxValue));
			break;
		case LUAParameterType::Max:
			Log::error("Invalid argument type");
			return false;
		}
	}
	return true;
}

core::String LUAGenerator::load(const core::String& scriptName) const {
	core::String filename = scriptName;
	io::normalizePath(filename);
	if (!io::filesystem()->exists(filename)) {
		if (!core::string::endsWith(filename, ".lua")) {
			filename.append(".lua");
		}
		filename = core::string::path("scripts", filename);
	}
#if LUA_VERSION_NUM < 504
	core::String lua = io::filesystem()->load(filename);
	return core::string::replaceAll(lua, "<const>", "");
#else
	return io::filesystem()->load(filename);
#endif
}

core::DynamicArray<LUAScript> LUAGenerator::listScripts() const {
	lua::LUA lua;
	core::DynamicArray<LUAScript> scripts;
	core::DynamicArray<io::FilesystemEntry> entities;
	io::filesystem()->list("scripts", entities, "*.lua");
	scripts.reserve(entities.size());
	for (const auto& e : entities) {
		const core::String path = core::string::path("scripts", e.name);
		lua.load(io::filesystem()->load(path));
		lua_getglobal(lua, "main");
		if (!lua_isfunction(lua, -1)) {
			Log::debug("No main() function found in %s", path.c_str());
			scripts.push_back({e.name, false});
			continue;
		}
		scripts.push_back({e.name, true});
	}
	return scripts;
}

bool LUAGenerator::exec(const core::String &luaScript, voxelformat::SceneGraph &sceneGraph, int nodeId,
						const voxel::Region &region, const voxel::Voxel &voxel, voxel::Region &dirtyRegion,
						const core::DynamicArray<core::String> &args) {
	core::DynamicArray<LUAParameterDescription> argsInfo;
	if (!argumentInfo(luaScript, argsInfo)) {
		Log::error("Failed to get argument details");
		return false;
	}

	if (!args.empty() && args[0] == "help") {
		Log::info("Parameter description");
		for (const auto& e : argsInfo) {
			Log::info(" %s: %s (default: '%s')", e.name.c_str(), e.description.c_str(), e.defaultValue.c_str());
		}
		return true;
	}

	voxelformat::SceneGraphNode &node = sceneGraph.node(nodeId);
	voxel::RawVolume *v = node.volume();
	if (v == nullptr) {
		Log::error("Node %i has no volume", nodeId);
		return false;
	}

	lua::LUA lua;
	lua.newGlobalData<voxelformat::SceneGraph>(luaVoxel_globalscenegraph(), &sceneGraph);
	lua.newGlobalData<voxel::Region>(luaVoxel_globaldirtyregion(), &dirtyRegion);
	lua.newGlobalData<int>(luaVoxel_globalnodeid(), &nodeId);
	lua.newGlobalData<noise::Noise>(luaVoxel_globalnoise(), &_noise);
	prepareState(lua);

	// load and run once to initialize the global variables
	if (luaL_dostring(lua, luaScript.c_str())) {
		Log::error("%s", lua_tostring(lua, -1));
		return false;
	}

	// get main(node, region, color) method
	lua_getglobal(lua, "main");
	if (!lua_isfunction(lua, -1)) {
		Log::error("LUA generator: no main(node, region, color) function found in '%s'", luaScript.c_str());
		return false;
	}

	// first parameter is scene node
	if (luaVoxel_pushscenegraphnode(lua, node) == 0) {
		Log::error("Failed to push scene graph node");
		return false;
	}

	// second parameter is the region to operate on
	if (luaVoxel_pushregion(lua, &region) == 0) {
		Log::error("Failed to push region");
		return false;
	}

	// third parameter is the current color
	lua_pushinteger(lua, voxel.getColor());

#if GENERATOR_LUA_SANTITY > 0
	if (!lua_isfunction(lua, -4)) {
		Log::error("LUA generate: expected to find the main function");
		return false;
	}
	if (!luaL_checkudata(lua, -3, luaVoxel_metascenegraphnode())) {
		Log::error("LUA generate: expected to find scene graph node");
		return false;
	}
	if (!luaL_checkudata(lua, -2, luaVoxel_metaregion())) {
		Log::error("LUA generate: expected to find region");
		return false;
	}
	if (!lua_isnumber(lua, -1)) {
		Log::error("LUA generate: expected to find color");
		return false;
	}
#endif

	if (!luaVoxel_pushargs(lua, args, argsInfo)) {
		Log::error("Failed to execute main() function with the given number of arguments. Try calling with 'help' as parameter");
		return false;
	}

	const int error = lua_pcall(lua, 3 + argsInfo.size(), 0, 0);
	if (error != LUA_OK) {
		Log::error("LUA generate script: %s", lua_isstring(lua, -1) ? lua_tostring(lua, -1) : "Unknown Error");
		return false;
	}

	return true;
}

}

#undef GENERATOR_LUA_SANTITY
