/**
 * @file
 */

#include "LUAGenerator.h"
#include "commonlua/LUAFunctions.h"
#include "lauxlib.h"
#include "lua.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Region.h"
#include "commonlua/LUA.h"
#include "voxel/Voxel.h"
#include "core/Color.h"

#define GENERATOR_LUA_SANTITY 1

namespace voxelgenerator {

static const char *luaVoxel_metaregion() {
	return "__meta_region";
}

static const char *luaVoxel_metavolume() {
	return "__meta_volume";
}

static const char *luaVoxel_metapalette() {
	return "__meta_palette";
}

static voxel::Region* luaVoxel_toRegion(lua_State* s, int n) {
	return *(voxel::Region**)clua_getudata<voxel::Region*>(s, n, luaVoxel_metaregion());
}

static voxel::RawVolumeWrapper* luaVoxel_toVolume(lua_State* s, int n) {
	return *(voxel::RawVolumeWrapper**)clua_getudata<voxel::RawVolume*>(s, n, luaVoxel_metavolume());
}

static int luaVoxel_pushregion(lua_State* s, const voxel::Region* region) {
	if (region == nullptr) {
		return luaL_error(s, "No region given - can't push");
	}
	return clua_pushudata(s, region, luaVoxel_metaregion());
}

static int luaVoxel_pushvolume(lua_State* s, voxel::RawVolumeWrapper* volume) {
	if (volume == nullptr) {
		return luaL_error(s, "No volume given - can't push");
	}
	return clua_pushudata(s, volume, luaVoxel_metavolume());
}

static int luaVoxel_volume_voxel(lua_State* s) {
	const voxel::RawVolumeWrapper* volume = luaVoxel_toVolume(s, 1);
	const int x = luaL_checkinteger(s, 2);
	const int y = luaL_checkinteger(s, 3);
	const int z = luaL_checkinteger(s, 4);
	lua_pushinteger(s, volume->voxel(x, y, z).getColor());
	return 1;
}

static int luaVoxel_volume_region(lua_State* s) {
	const voxel::RawVolumeWrapper* volume = luaVoxel_toVolume(s, 1);
	return luaVoxel_pushregion(s, &volume->region());
}

static int luaVoxel_volume_setvoxel(lua_State* s) {
	voxel::RawVolumeWrapper* volume = luaVoxel_toVolume(s, 1);
	const int x = luaL_checkinteger(s, 2);
	const int y = luaL_checkinteger(s, 3);
	const int z = luaL_checkinteger(s, 4);
	const int color = luaL_checkinteger(s, 5);
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, color);
	const bool insideRegion = volume->setVoxel(x, y, z, voxel);
	lua_pushboolean(s, insideRegion ? 1 : 0);
	return 1;
}

static int luaVoxel_palette_color(lua_State* s) {
	const uint8_t color = luaL_checkinteger(s, 1);
	const glm::vec4& rgba = voxel::getMaterialColor(voxel::createVoxel(voxel::VoxelType::Generic, color));
	return clua_push(s, rgba);
}

static int luaVoxel_palette_closestmatch(lua_State* s) {
	const voxel::MaterialColorArray& materialColors = voxel::getMaterialColors();
	const float r = luaL_checkinteger(s, 1) / 255.0f;
	const float g = luaL_checkinteger(s, 2) / 255.0f;
	const float b = luaL_checkinteger(s, 3) / 255.0f;
	const int match = core::Color::getClosestMatch(glm::vec4(r, b, g, 1.0f), materialColors);
	if (match < 0 || match > 255) {
		return luaL_error(s, "Given color index is not valid or palette is not loaded");
	}
	lua_pushinteger(s, match);
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

static int luaVoxel_region_tostring(lua_State *s) {
	const voxel::Region* region = luaVoxel_toRegion(s, 1);
	const glm::ivec3& mins = region->getLowerCorner();
	const glm::ivec3& maxs = region->getUpperCorner();
	lua_pushfstring(s, "region: [%i:%i:%i]/[%i:%i:%i]", mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);
	return 1;
}

static void prepareState(lua_State* s) {
	static const luaL_Reg volumeFuncs[] = {
		{"voxel", luaVoxel_volume_voxel},
		{"region", luaVoxel_volume_region},
		{"setVoxel", luaVoxel_volume_setvoxel},
		{nullptr, nullptr}
	};
	clua_registerfuncs(s, volumeFuncs, luaVoxel_metavolume());

	static const luaL_Reg regionFuncs[] = {
		{"width", luaVoxel_region_width},
		{"height", luaVoxel_region_height},
		{"depth", luaVoxel_region_depth},
		{"x", luaVoxel_region_x},
		{"y", luaVoxel_region_y},
		{"z", luaVoxel_region_z},
		{"mins", luaVoxel_region_mins},
		{"maxs", luaVoxel_region_maxs},
		{"__tostring", luaVoxel_region_tostring},
		{nullptr, nullptr}
	};
	clua_registerfuncs(s, regionFuncs, luaVoxel_metaregion());

	static const luaL_Reg paletteFuncs[] = {
		{"color", luaVoxel_palette_color},
		{"match", luaVoxel_palette_closestmatch},
		{nullptr, nullptr}
	};
	clua_registerfuncs(s, paletteFuncs, luaVoxel_metapalette());
	lua_setglobal(s, "palette");

	clua_cmdregister(s);
	clua_varregister(s);
	clua_logregister(s);

	clua_vecregister<glm::vec2>(s);
	clua_vecregister<glm::vec3>(s);
	clua_vecregister<glm::vec4>(s);
	clua_vecregister<glm::ivec2>(s);
	clua_vecregister<glm::ivec3>(s);
	clua_vecregister<glm::ivec4>(s);
}

bool LUAGenerator::init() {
	return true;
}

void LUAGenerator::shutdown() {
}

bool LUAGenerator::exec(const core::String& luaScript, voxel::RawVolumeWrapper* volume, const voxel::Region& region, const voxel::Voxel& voxel) {
	lua::LUA lua;
	prepareState(lua);

	// load and run once to initialize the global variables
	if (luaL_dostring(lua, luaScript.c_str())) {
		Log::error("%s", lua_tostring(lua, -1));
		return false;
	}

	// get main(volume, region) method
	lua_getglobal(lua, "main");
	if (!lua_isfunction(lua, -1)) {
		Log::error("%s", lua.stackDump().c_str());
		Log::error("LUA generator: no main(volume, region) function found in '%s'", luaScript.c_str());
		return false;
	}

	// first parameter is volume
	if (luaVoxel_pushvolume(lua, volume) == 0) {
		Log::error("Failed to push volume");
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
	if (!lua_isuserdata(lua, -3)) {
		Log::error("LUA generate: expected to find volume");
		return false;
	}
	if (!lua_isuserdata(lua, -2)) {
		Log::error("LUA generate: expected to find region");
		return false;
	}
	if (!lua_isnumber(lua, -1)) {
		Log::error("LUA generate: expected to find color");
		return false;
	}
#endif
	const int error = lua_pcall(lua, 3, 0, 0);
	if (error) {
		Log::error("LUA generate script: %s", lua_isstring(lua, -1) ? lua_tostring(lua, -1) : "Unknown Error");
		return false;
	}

	return true;
}

}

#undef GENERATOR_LUA_SANTITY
