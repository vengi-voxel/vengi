/**
 * @file
 */

#include "EditorLUAGenerator.h"
#include "commonlua/LUAFunctions.h"
#include "SceneManager.h"
#include "lauxlib.h"
#include "lua.h"
#include "voxedit-util/layer/LayerManager.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxelgenerator/LUAGenerator.h"

namespace voxedit {

static const char *luaVoxel_metalayer() {
	return "__meta_layer";
}

static const char *luaVoxel_metavolume() {
	return "__meta_volume";
}

static const char *luaVoxel_metalayermgr() {
	return "__meta_layermgr";
}

struct LUALayer {
	Layer * layer;
	int layerId;
};

struct LUAVolume {
	int layerId;
	voxel::RawVolume* volume;
	voxel::Region dirtyRegion = voxel::Region::InvalidRegion;
};

static LUALayer* luaVoxel_toLayer(lua_State* s, int n) {
	return clua_getudata<LUALayer*>(s, n, luaVoxel_metalayer());
}

static int luaVoxel_pushvolume(lua_State* s, const LUAVolume& luaVolume) {
	return clua_pushudata(s, luaVolume, luaVoxel_metavolume());
}

static LUAVolume* luaVoxel_tovolume(lua_State* s, int n) {
	return clua_getudata<LUAVolume*>(s, n, luaVoxel_metavolume());
}

static int luaVoxel_layermgr_new(lua_State* s) {
	const char *name = lua_tostring(s, 1);
	const bool visible = lua_toboolean(s, 2);
	const voxel::Region* region = voxelgenerator::LUAGenerator::luaVoxel_toRegion(s, 3);
	voxel::RawVolume *volume = new voxel::RawVolume(*region);
	LayerManager& layerMgr = sceneMgr().layerMgr();
	int layerId = layerMgr.addLayer(name, visible, volume);
	if (layerId == -1) {
		delete volume;
	}
	Layer& layer = layerMgr.layer(layerId);
	LUALayer luaLayer{&layer, layerId};
	return clua_pushudata(s, luaLayer, luaVoxel_metalayer());
}

static int luaVoxel_layermgr_get(lua_State* s) {
	int layerId = luaL_optinteger(s, 1, -1);
	LayerManager& layerMgr = sceneMgr().layerMgr();
	if (layerId == -1) {
		layerId = layerMgr.activeLayer();
	}
	if (layerId < 0 || layerId >= layerMgr.maxLayers()) {
		return lua::LUA::returnError(s, "Could not find layer for id %d", layerId);
	}
	Layer& layer = layerMgr.layer(layerId);
	if (!layer.valid) {
		return lua::LUA::returnError(s, "Invalid layer for id %d", layerId);
	}
	LUALayer luaLayer{&layer, layerId};
	return clua_pushudata(s, luaLayer, luaVoxel_metalayer());
}

static int luaVoxel_layer_name(lua_State* s) {
	LUALayer* luaLayer = luaVoxel_toLayer(s, 1);
	lua_pushstring(s, luaLayer->layer->name.c_str());
	return 1;
}

static int luaVoxel_layer_tostring(lua_State *s) {
	LUALayer* luaLayer = luaVoxel_toLayer(s, 1);
	lua_pushfstring(s, "layer: [%d, %s]", luaLayer->layerId, luaLayer->layer->name.c_str());
	return 1;
}

static int luaVoxel_layer_volume(lua_State* s) {
	LUALayer* luaLayer = luaVoxel_toLayer(s, 1);
	voxel::RawVolume* volume = sceneMgr().volume(luaLayer->layerId);
	if (volume == nullptr) {
		return lua::LUA::returnError(s, "Invalid layer id %d given - no volume found", luaLayer->layerId);
	}
	const LUAVolume luaVolume{luaLayer->layerId, volume, voxel::Region::InvalidRegion};
	return luaVoxel_pushvolume(s, luaVolume);
}

static int luaVoxel_volume_voxel(lua_State* s) {
	const LUAVolume* volume = luaVoxel_tovolume(s, 1);
	const int x = luaL_checkinteger(s, 2);
	const int y = luaL_checkinteger(s, 3);
	const int z = luaL_checkinteger(s, 4);
	const voxel::Voxel& voxel = volume->volume->voxel(x, y, z);
	if (voxel::isAir(voxel.getMaterial())) {
		lua_pushinteger(s, -1);
	} else {
		lua_pushinteger(s, voxel.getColor());
	}
	return 1;
}

static int luaVoxel_volume_region(lua_State* s) {
	const LUAVolume* volume = luaVoxel_tovolume(s, 1);
	return voxelgenerator::LUAGenerator::luaVoxel_pushregion(s, &volume->volume->region());
}

static int luaVoxel_volume_setvoxel(lua_State* s) {
	LUAVolume* volume = luaVoxel_tovolume(s, 1);
	voxel::RawVolumeWrapper wrapper(volume->volume);
	const int x = luaL_checkinteger(s, 2);
	const int y = luaL_checkinteger(s, 3);
	const int z = luaL_checkinteger(s, 4);
	const int color = luaL_checkinteger(s, 5);
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, color);
	const bool insideRegion = wrapper.setVoxel(x, y, z, voxel);
	lua_pushboolean(s, insideRegion ? 1 : 0);
	if (wrapper.dirtyRegion().isValid()) {
		if (volume->dirtyRegion.isValid()) {
			volume->dirtyRegion.accumulate(wrapper.dirtyRegion());
		} else {
			volume->dirtyRegion = wrapper.dirtyRegion();
		}
	}
	return 1;
}

static int luaVoxel_volume_gc(lua_State *s) {
	LUAVolume* volume = luaVoxel_tovolume(s, 1);
	if (volume->dirtyRegion.isValid()) {
		sceneMgr().modified(volume->layerId, volume->dirtyRegion);
	}
	return 0;
}

void EditorLUAGenerator::initializeCustomState(lua_State *s) {
	static const luaL_Reg layerMgrFuncs[] = {
		{"new", luaVoxel_layermgr_new},
		{"get", luaVoxel_layermgr_get},
		{nullptr, nullptr}
	};
	clua_registerfuncs(s, layerMgrFuncs, luaVoxel_metalayermgr());
	lua_setglobal(s, "layermgr");

	static const luaL_Reg layerFuncs[] = {
		{"volume", luaVoxel_layer_volume},
		{"name", luaVoxel_layer_name},
		{"__tostring", luaVoxel_layer_tostring},
		{nullptr, nullptr}
	};
	clua_registerfuncs(s, layerFuncs, luaVoxel_metalayer());

	static const luaL_Reg volumeFuncs[] = {
		{"voxel", luaVoxel_volume_voxel},
		{"region", luaVoxel_volume_region},
		{"setVoxel", luaVoxel_volume_setvoxel},
		{"__gc", luaVoxel_volume_gc},
		{nullptr, nullptr}
	};
	clua_registerfuncs(s, volumeFuncs, luaVoxel_metavolume());
}

}
