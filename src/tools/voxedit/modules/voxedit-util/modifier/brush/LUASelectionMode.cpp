/**
 * @file
 */

#include "LUASelectionMode.h"
#include "Brush.h"
#include "commonlua/LUA.h"
#include "commonlua/LUAFunctions.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "io/Filesystem.h"
#include "scenegraph/SceneGraph.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxelgenerator/LUAApi.h"

#include "LUAIconMapping.h"
#include "ui/IconsLucide.h"

namespace voxedit {

struct SelectionContext {
	const BrushContext *brushCtx = nullptr;
	const glm::ivec3 *aabbFirstPos = nullptr;
	voxel::FaceNames aabbFace = voxel::FaceNames::Max;
};

static const char *luaSelection_metaname() {
	return "__meta_selectioncontext";
}

static SelectionContext *luaSelection_context(lua_State *s) {
	lua_getglobal(s, luaSelection_metaname());
	SelectionContext *ctx = (SelectionContext *)lua_touserdata(s, -1);
	lua_pop(s, 1);
	return ctx;
}

static int luaSelection_cursorpos(lua_State *s) {
	const SelectionContext *ctx = luaSelection_context(s);
	if (ctx == nullptr || ctx->brushCtx == nullptr) {
		return clua_error(s, "No selection context available");
	}
	return clua_push(s, ctx->brushCtx->cursorPosition);
}

static int luaSelection_cursorface(lua_State *s) {
	const SelectionContext *ctx = luaSelection_context(s);
	if (ctx == nullptr || ctx->brushCtx == nullptr) {
		return clua_error(s, "No selection context available");
	}
	lua_pushstring(s, voxel::faceNameString(ctx->brushCtx->cursorFace));
	return 1;
}

static int luaSelection_hitcursorcolor(lua_State *s) {
	const SelectionContext *ctx = luaSelection_context(s);
	if (ctx == nullptr || ctx->brushCtx == nullptr) {
		return clua_error(s, "No selection context available");
	}
	lua_pushinteger(s, ctx->brushCtx->hitCursorVoxel.getColor());
	return 1;
}

static int luaSelection_modifiertype(lua_State *s) {
	const SelectionContext *ctx = luaSelection_context(s);
	if (ctx == nullptr || ctx->brushCtx == nullptr) {
		return clua_error(s, "No selection context available");
	}
	const char *type = "override";
	if ((ctx->brushCtx->modifierType & ModifierType::Erase) != ModifierType::None) {
		type = "erase";
	}
	lua_pushstring(s, type);
	return 1;
}

static int luaSelection_aabbfirstpos(lua_State *s) {
	const SelectionContext *ctx = luaSelection_context(s);
	if (ctx == nullptr || ctx->aabbFirstPos == nullptr) {
		return clua_error(s, "No selection context available");
	}
	return clua_push(s, *ctx->aabbFirstPos);
}

static int luaSelection_aabbface(lua_State *s) {
	const SelectionContext *ctx = luaSelection_context(s);
	if (ctx == nullptr) {
		return clua_error(s, "No selection context available");
	}
	lua_pushstring(s, voxel::faceNameString(ctx->aabbFace));
	return 1;
}

static int luaSelection_referencepos(lua_State *s) {
	const SelectionContext *ctx = luaSelection_context(s);
	if (ctx == nullptr || ctx->brushCtx == nullptr) {
		return clua_error(s, "No selection context available");
	}
	return clua_push(s, ctx->brushCtx->referencePos);
}

static int luaSelection_targetvolumeregion(lua_State *s) {
	const SelectionContext *ctx = luaSelection_context(s);
	if (ctx == nullptr || ctx->brushCtx == nullptr) {
		return clua_error(s, "No selection context available");
	}
	return voxelgenerator::luaVoxel_pushregion(s, ctx->brushCtx->targetVolumeRegion);
}

// JSON help functions for API documentation
static int luaSelection_cursorpos_jsonhelp(lua_State *s) {
	const char *json = R"({
		"name": "cursorPos",
		"summary": "Get the current cursor position.",
		"parameters": [],
		"returns": [
			{"type": "ivec3", "description": "The cursor position."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaSelection_cursorface_jsonhelp(lua_State *s) {
	const char *json = R"({
		"name": "cursorFace",
		"summary": "Get the face direction where the raycast hit.",
		"parameters": [],
		"returns": [
			{"type": "string", "description": "One of 'left', 'right', 'up', 'down', 'front', 'back', or 'max'."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaSelection_hitcursorcolor_jsonhelp(lua_State *s) {
	const char *json = R"({
		"name": "hitCursorColor",
		"summary": "Get the palette color index of the voxel hit by raycast.",
		"parameters": [],
		"returns": [
			{"type": "integer", "description": "The palette color index of the hit voxel."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaSelection_modifiertype_jsonhelp(lua_State *s) {
	const char *json = R"({
		"name": "modifierType",
		"summary": "Get the current modifier type (select or deselect).",
		"parameters": [],
		"returns": [
			{"type": "string", "description": "One of 'override' (select) or 'erase' (deselect)."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaSelection_aabbfirstpos_jsonhelp(lua_State *s) {
	const char *json = R"({
		"name": "aabbFirstPos",
		"summary": "Get the position of the initial click that started the selection.",
		"parameters": [],
		"returns": [
			{"type": "ivec3", "description": "The first click position."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaSelection_aabbface_jsonhelp(lua_State *s) {
	const char *json = R"({
		"name": "aabbFace",
		"summary": "Get the face from the initial click.",
		"parameters": [],
		"returns": [
			{"type": "string", "description": "One of 'left', 'right', 'up', 'down', 'front', 'back', or 'max'."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaSelection_referencepos_jsonhelp(lua_State *s) {
	const char *json = R"({
		"name": "referencePos",
		"summary": "Get the reference position set by the user.",
		"parameters": [],
		"returns": [
			{"type": "ivec3", "description": "The reference position."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaSelection_targetvolumeregion_jsonhelp(lua_State *s) {
	const char *json = R"({
		"name": "targetVolumeRegion",
		"summary": "Get the region of the target volume.",
		"parameters": [],
		"returns": [
			{"type": "Region", "description": "The region of the target volume."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

LUASelectionMode::LUASelectionMode(const io::FilesystemPtr &filesystem) : _filesystem(filesystem) {
}

bool LUASelectionMode::initLuaState() {
	_lua.resetState();
	if (!_noise.init()) {
		Log::warn("Failed to initialize noise for selection mode script");
	}
	voxelgenerator::luaVoxel_setGlobalData(_lua, voxelgenerator::luaVoxel_globalnoise(), &_noise);
	voxelgenerator::luaVoxel_setGlobalData(_lua, voxelgenerator::luaVoxel_globaldirtyregions(), &_dirtyRegions);
	voxelgenerator::luaVoxel_prepareState(_lua);

	static const clua_Reg selectionStateFuncs[] = {
		{"cursorPos", luaSelection_cursorpos, luaSelection_cursorpos_jsonhelp},
		{"cursorFace", luaSelection_cursorface, luaSelection_cursorface_jsonhelp},
		{"hitCursorColor", luaSelection_hitcursorcolor, luaSelection_hitcursorcolor_jsonhelp},
		{"modifierType", luaSelection_modifiertype, luaSelection_modifiertype_jsonhelp},
		{"aabbFirstPos", luaSelection_aabbfirstpos, luaSelection_aabbfirstpos_jsonhelp},
		{"aabbFace", luaSelection_aabbface, luaSelection_aabbface_jsonhelp},
		{"referencePos", luaSelection_referencepos, luaSelection_referencepos_jsonhelp},
		{"targetVolumeRegion", luaSelection_targetvolumeregion, luaSelection_targetvolumeregion_jsonhelp},
		{nullptr, nullptr, nullptr}};
	clua_registerfuncsglobal(_lua, selectionStateFuncs, luaSelection_metaname(), "g_selectioncontext");

	return true;
}

bool LUASelectionMode::init() {
	return true;
}

void LUASelectionMode::shutdown() {
	_lua.resetState();
	_noise.shutdown();
	_scriptLoaded = false;
	_scriptSource.clear();
	_parameterDescription.clear();
	_parameters.clear();
}

bool LUASelectionMode::apiJsonToStream(io::WriteStream &stream) {
	if (!initLuaState()) {
		Log::error("Failed to initialize LUASelectionMode lua state for API documentation");
		return false;
	}
	lua_State *s = _lua.state();

	if (!stream.writeString("{\"g_selectioncontext\":{\"type\":\"global\",\"methods\":[", false)) {
		return false;
	}

	lua_getglobal(s, "g_selectioncontext");
	if (!lua_istable(s, -1)) {
		lua_pop(s, 1);
		Log::error("g_selectioncontext is not a table");
		return false;
	}

	bool firstMethod = true;
	if (!clua_writejsonmethods(s, luaSelection_metaname(), stream, firstMethod)) {
		lua_pop(s, 1);
		return false;
	}
	lua_pop(s, 1);

	if (!stream.writeString("]}}\n", false)) {
		return false;
	}

	return true;
}

bool LUASelectionMode::loadScript(const core::String &filename) {
	_scriptLoaded = false;
	_scriptFilename = filename;
	_description.clear();
	_parameterDescription.clear();
	_parameters.clear();

	core::String path = filename;
	if (!_filesystem->exists(path)) {
		if (core::string::extractExtension(path) != "lua") {
			path.append(".lua");
		}
		path = core::string::path("selectionmodes", path);
	}

	_scriptSource = _filesystem->load(path);
	if (_scriptSource.empty()) {
		Log::error("Failed to load selection mode script: %s", path.c_str());
		return false;
	}

	if (!initLuaState()) {
		return false;
	}

	const int top = lua_gettop(_lua);
	if (luaL_dostring(_lua, _scriptSource.c_str())) {
		Log::error("Failed to run selection mode script %s: %s", filename.c_str(), lua_tostring(_lua, -1));
		lua_pop(_lua, 1);
		return false;
	}
	lua_settop(_lua, top);

	// Verify select() function exists
	lua_getglobal(_lua, "select");
	if (!lua_isfunction(_lua, -1)) {
		Log::error("Selection mode script %s has no select() function", filename.c_str());
		lua_pop(_lua, 1);
		return false;
	}
	lua_pop(_lua, 1);

	// Get description
	lua_getglobal(_lua, "description");
	if (lua_isfunction(_lua, -1)) {
		if (lua_pcall(_lua, 0, 1, 0) == LUA_OK) {
			if (lua_isstring(_lua, -1)) {
				_description = lua_tostring(_lua, -1);
			}
			lua_pop(_lua, 1);
		} else {
			Log::warn("Error calling description() in %s: %s", filename.c_str(),
					  lua_isstring(_lua, -1) ? lua_tostring(_lua, -1) : "Unknown Error");
			lua_pop(_lua, 1);
		}
	} else {
		lua_pop(_lua, 1);
	}

	// Get icon
	_iconName = "";
	lua_getglobal(_lua, "icon");
	if (lua_isfunction(_lua, -1)) {
		if (lua_pcall(_lua, 0, 1, 0) == LUA_OK) {
			if (lua_isstring(_lua, -1)) {
				_iconName = lua_tostring(_lua, -1);
			}
			lua_pop(_lua, 1);
		} else {
			Log::warn("Error calling icon() in %s: %s", filename.c_str(),
					  lua_isstring(_lua, -1) ? lua_tostring(_lua, -1) : "Unknown Error");
			lua_pop(_lua, 1);
		}
	} else {
		lua_pop(_lua, 1);
	}

	// Get argument info
	lua_getglobal(_lua, "arguments");
	if (lua_isfunction(_lua, -1)) {
		lua_pop(_lua, 1);
		lua::LUA tmpLua;
		if (luaL_dostring(tmpLua, _scriptSource.c_str()) == LUA_OK) {
			voxelgenerator::LUAApi api(_filesystem);
			api.argumentInfo(tmpLua, _parameterDescription);
		}
		_parameters.resize(_parameterDescription.size());
		for (size_t i = 0; i < _parameterDescription.size(); ++i) {
			_parameters[i] = _parameterDescription[i].defaultValue;
		}
	} else {
		lua_pop(_lua, 1);
	}

	_scriptLoaded = true;
	Log::debug("Loaded selection mode script: %s", filename.c_str());
	return true;
}

void LUASelectionMode::execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper,
							   const BrushContext &ctx, const voxel::Region &region,
							   const glm::ivec3 &aabbFirstPos, voxel::FaceNames aabbFace) {
	if (!_scriptLoaded) {
		return;
	}

	_dirtyRegions.clear();

	lua_State *s = _lua.state();

	voxelgenerator::luaVoxel_setGlobalData(s, voxelgenerator::luaVoxel_globalscenegraph(), &sceneGraph);

	SelectionContext selCtx;
	selCtx.brushCtx = &ctx;
	selCtx.aabbFirstPos = &aabbFirstPos;
	selCtx.aabbFace = aabbFace;
	voxelgenerator::luaVoxel_setGlobalData(s, luaSelection_metaname(), &selCtx);

	const int top = lua_gettop(s);
	if (luaL_dostring(s, _scriptSource.c_str())) {
		Log::error("Failed to reload selection mode script: %s", lua_tostring(s, -1));
		lua_pop(s, 1);
		return;
	}
	lua_settop(s, top);

	lua_getglobal(s, "select");
	if (!lua_isfunction(s, -1)) {
		Log::error("Selection mode script lost its select() function");
		lua_pop(s, 1);
		return;
	}

	scenegraph::SceneGraphNode &node = wrapper.node();
	if (voxelgenerator::luaVoxel_pushscenegraphnode(s, node) == 0) {
		Log::error("Failed to push scene graph node for selection mode script");
		lua_pop(s, 1);
		return;
	}

	if (voxelgenerator::luaVoxel_pushregion(s, region) == 0) {
		Log::error("Failed to push region for selection mode script");
		lua_pop(s, 2);
		lua_gc(s, LUA_GCCOLLECT, 0);
		return;
	}

	int nargs = 2;
	if (!_parameterDescription.empty()) {
		const palette::Palette &palette = wrapper.node().palette();
		if (!voxelgenerator::luaVoxel_pushargs(s, _parameters, _parameterDescription, palette)) {
			Log::error("Failed to push selection mode script arguments");
			lua_settop(s, top);
			lua_gc(s, LUA_GCCOLLECT, 0);
			return;
		}
		nargs += (int)_parameterDescription.size();
	}

	if (lua_pcall(s, nargs, 0, 0) != LUA_OK) {
		Log::error("Error executing selection mode script: %s", lua_tostring(s, -1));
		lua_pop(s, 1);
		lua_gc(s, LUA_GCCOLLECT, 0);
		return;
	}

	lua_gc(s, LUA_GCCOLLECT, 0);
}

core::String LUASelectionMode::scriptName() const {
	return core::string::extractFilename(_scriptFilename);
}

const char *LUASelectionMode::iconString() const {
	return luaIconString(_iconName, ICON_LC_SQUARE_DASHED);
}

} // namespace voxedit
