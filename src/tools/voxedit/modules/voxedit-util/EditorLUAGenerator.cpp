/**
 * @file
 */

#include "EditorLUAGenerator.h"
#include "commonlua/LUAFunctions.h"
#include "SceneManager.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxelformat/SceneGraphNode.h"
#include "voxelgenerator/LUAGenerator.h"

namespace voxedit {

static const char *luaVoxel_metascenegraphnode() {
	return "__meta_layer";
}

static const char *luaVoxel_metavolume() {
	return "__meta_volume";
}

static const char *luaVoxel_metascenegraph() {
	return "__meta_scenegraph";
}

struct LUASceneGraphNode {
	int nodeId;
};

struct LUAVolume {
	int nodeId;
	voxel::RawVolume* volume;
	voxel::Region dirtyRegion = voxel::Region::InvalidRegion;
};

static LUASceneGraphNode* luaVoxel_toSceneGraphNode(lua_State* s, int n) {
	return clua_getudata<LUASceneGraphNode*>(s, n, luaVoxel_metascenegraphnode());
}

static int luaVoxel_pushscenegraphnode(lua_State* s, const LUASceneGraphNode& luaNode) {
	return clua_pushudata(s, luaNode, luaVoxel_metascenegraphnode());
}

static int luaVoxel_pushvolume(lua_State* s, const LUAVolume& luaVolume) {
	return clua_pushudata(s, luaVolume, luaVoxel_metavolume());
}

static LUAVolume* luaVoxel_tovolume(lua_State* s, int n) {
	return clua_getudata<LUAVolume*>(s, n, luaVoxel_metavolume());
}

static int luaVoxel_scenegraph_new_node(lua_State* s) {
	const char *name = lua_tostring(s, 1);
	const bool visible = lua_toboolean(s, 2);
	const voxel::Region* region = voxelgenerator::LUAGenerator::luaVoxel_toRegion(s, 3);
	voxel::RawVolume *v = new voxel::RawVolume(*region);
	voxelformat::SceneGraphNode node;
	node.setVolume(v, true);
	node.setName(name);
	node.setVisible(visible);
	const glm::vec3 rp = v->region().getPivot();
	const glm::vec3 size = v->region().getDimensionsInVoxels();
	node.setPivot(0, rp, size);
	const int nodeId = sceneMgr().addNodeToSceneGraph(node);
	if (nodeId == -1) {
		return clua_error(s, "Failed to add new node");
	}

	LUASceneGraphNode luaNode{nodeId};
	return luaVoxel_pushscenegraphnode(s, luaNode);
}

static int luaVoxel_scenegraph_get_node(lua_State* s) {
	int nodeId = (int)luaL_optinteger(s, 1, -1);
	const voxelformat::SceneGraph &sceneGraph = sceneMgr().sceneGraph();
	if (nodeId == -1) {
		nodeId = sceneGraph.activeNode();
	}
	if (!sceneGraph.hasNode(nodeId)) {
		return clua_error(s, "Could not find node for id %d", nodeId);
	}
	const voxelformat::SceneGraphNode& node = sceneGraph.node(nodeId);
	if (node.type() != voxelformat::SceneGraphNodeType::Model) {
		return clua_error(s, "Invalid node for id %d", nodeId);
	}
	LUASceneGraphNode luaNode{nodeId};
	return luaVoxel_pushscenegraphnode(s, luaNode);
}

static int luaVoxel_scenegraphnode_name(lua_State* s) {
	LUASceneGraphNode* luaNode = luaVoxel_toSceneGraphNode(s, 1);
	const voxelformat::SceneGraph &sceneGraph = sceneMgr().sceneGraph();
	if (!sceneGraph.hasNode(luaNode->nodeId)) {
		return clua_error(s, "Node with id %i not found", luaNode->nodeId);
	}
	const voxelformat::SceneGraphNode& node = sceneGraph.node(luaNode->nodeId);
	lua_pushstring(s, node.name().c_str());
	return 1;
}

static int luaVoxel_scenegraphnode_setname(lua_State* s) {
	LUASceneGraphNode* luaNode = luaVoxel_toSceneGraphNode(s, 1);
	const char *newName = lua_tostring(s, 2);
	const voxelformat::SceneGraph &sceneGraph = sceneMgr().sceneGraph();
	if (!sceneGraph.hasNode(luaNode->nodeId)) {
		return clua_error(s, "Node with id %i not found", luaNode->nodeId);
	}
	sceneMgr().nodeRename(luaNode->nodeId, newName);
	return 0;
}

static int luaVoxel_scenegraphnode_tostring(lua_State *s) {
	LUASceneGraphNode* luaNode = luaVoxel_toSceneGraphNode(s, 1);
	const voxelformat::SceneGraph &sceneGraph = sceneMgr().sceneGraph();
	if (!sceneGraph.hasNode(luaNode->nodeId)) {
		return clua_error(s, "Node with id %i not found", luaNode->nodeId);
	}
	voxelformat::SceneGraphNode& node = sceneGraph.node(luaNode->nodeId);
	lua_pushfstring(s, "layer: [%d, %s]", luaNode->nodeId, node.name().c_str());
	return 1;
}

static int luaVoxel_scenegraphnode_volume(lua_State* s) {
	LUASceneGraphNode* luaNode = luaVoxel_toSceneGraphNode(s, 1);
	const voxelformat::SceneGraph &sceneGraph = sceneMgr().sceneGraph();
	if (!sceneGraph.hasNode(luaNode->nodeId)) {
		return clua_error(s, "Node with id %i not found", luaNode->nodeId);
	}
	voxelformat::SceneGraphNode& node = sceneGraph.node(luaNode->nodeId);
	voxel::RawVolume* volume = node.volume();
	if (volume == nullptr) {
		return clua_error(s, "Invalid node id %d given - no volume found", luaNode->nodeId);
	}
	const LUAVolume luaVolume{luaNode->nodeId, volume, voxel::Region::InvalidRegion};
	return luaVoxel_pushvolume(s, luaVolume);
}

static int luaVoxel_volume_voxel(lua_State* s) {
	const LUAVolume* volume = luaVoxel_tovolume(s, 1);
	const int x = (int)luaL_checkinteger(s, 2);
	const int y = (int)luaL_checkinteger(s, 3);
	const int z = (int)luaL_checkinteger(s, 4);
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
	const int x = (int)luaL_checkinteger(s, 2);
	const int y = (int)luaL_checkinteger(s, 3);
	const int z = (int)luaL_checkinteger(s, 4);
	const int color = (int)luaL_checkinteger(s, 5);
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
		sceneMgr().modified(volume->nodeId, volume->dirtyRegion);
	}
	return 0;
}

void EditorLUAGenerator::initializeCustomState(lua_State *s) {
	static const luaL_Reg sceneGraphFuncs[] = {
		{"new", luaVoxel_scenegraph_new_node},
		{"get", luaVoxel_scenegraph_get_node},
		{nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, sceneGraphFuncs, luaVoxel_metascenegraph(), "scenegraph");

	static const luaL_Reg sceneGraphNodeFuncs[] = {
		{"volume", luaVoxel_scenegraphnode_volume},
		{"name", luaVoxel_scenegraphnode_name},
		{"setName", luaVoxel_scenegraphnode_setname},
		{"__tostring", luaVoxel_scenegraphnode_tostring},
		{nullptr, nullptr}
	};
	clua_registerfuncs(s, sceneGraphNodeFuncs, luaVoxel_metascenegraphnode());

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
