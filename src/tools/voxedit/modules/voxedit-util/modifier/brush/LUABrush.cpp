/**
 * @file
 */

#include "LUABrush.h"
#include "commonlua/LUA.h"
#include "commonlua/LUAFunctions.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "io/Filesystem.h"
#include "scenegraph/SceneGraph.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxelgenerator/LUAApi.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>

#include "ui/IconsLucide.h"

namespace voxedit {

static const char *luaBrush_metaname() {
	return "__meta_brushcontext";
}

static BrushContext *luaBrush_context(lua_State *s) {
	lua_getglobal(s, luaBrush_metaname());
	BrushContext *ctx = (BrushContext *)lua_touserdata(s, -1);
	lua_pop(s, 1);
	return ctx;
}

static int luaBrush_referencepos(lua_State *s) {
	const BrushContext *ctx = luaBrush_context(s);
	if (ctx == nullptr) {
		return clua_error(s, "No brush context available");
	}
	return clua_push(s, ctx->referencePos);
}

static int luaBrush_cursorpos(lua_State *s) {
	const BrushContext *ctx = luaBrush_context(s);
	if (ctx == nullptr) {
		return clua_error(s, "No brush context available");
	}
	return clua_push(s, ctx->cursorPosition);
}

static int luaBrush_color(lua_State *s) {
	const BrushContext *ctx = luaBrush_context(s);
	if (ctx == nullptr) {
		return clua_error(s, "No brush context available");
	}
	lua_pushinteger(s, ctx->cursorVoxel.getColor());
	return 1;
}

static int luaBrush_prevcursorpos(lua_State *s) {
	const BrushContext *ctx = luaBrush_context(s);
	if (ctx == nullptr) {
		return clua_error(s, "No brush context available");
	}
	return clua_push(s, ctx->prevCursorPosition);
}

static int luaBrush_hitcursorcolor(lua_State *s) {
	const BrushContext *ctx = luaBrush_context(s);
	if (ctx == nullptr) {
		return clua_error(s, "No brush context available");
	}
	lua_pushinteger(s, ctx->hitCursorVoxel.getColor());
	return 1;
}

static int luaBrush_voxelatcursorcolor(lua_State *s) {
	const BrushContext *ctx = luaBrush_context(s);
	if (ctx == nullptr) {
		return clua_error(s, "No brush context available");
	}
	lua_pushinteger(s, ctx->voxelAtCursor.getColor());
	return 1;
}

static int luaBrush_cursorface(lua_State *s) {
	const BrushContext *ctx = luaBrush_context(s);
	if (ctx == nullptr) {
		return clua_error(s, "No brush context available");
	}
	lua_pushstring(s, voxel::faceNameString(ctx->cursorFace));
	return 1;
}

static int luaBrush_lockedaxis(lua_State *s) {
	const BrushContext *ctx = luaBrush_context(s);
	if (ctx == nullptr) {
		return clua_error(s, "No brush context available");
	}
	const char *axis = "none";
	switch (ctx->lockedAxis) {
	case math::Axis::X:
		axis = "x";
		break;
	case math::Axis::Y:
		axis = "y";
		break;
	case math::Axis::Z:
		axis = "z";
		break;
	default:
		break;
	}
	lua_pushstring(s, axis);
	return 1;
}

static int luaBrush_fixedorthosideview(lua_State *s) {
	const BrushContext *ctx = luaBrush_context(s);
	if (ctx == nullptr) {
		return clua_error(s, "No brush context available");
	}
	lua_pushboolean(s, ctx->fixedOrthoSideView);
	return 1;
}

static int luaBrush_normalpaint(lua_State *s) {
	const BrushContext *ctx = luaBrush_context(s);
	if (ctx == nullptr) {
		return clua_error(s, "No brush context available");
	}
	lua_pushboolean(s, ctx->normalPaint);
	return 1;
}

static int luaBrush_normalindex(lua_State *s) {
	const BrushContext *ctx = luaBrush_context(s);
	if (ctx == nullptr) {
		return clua_error(s, "No brush context available");
	}
	lua_pushinteger(s, ctx->normalIndex);
	return 1;
}

static int luaBrush_gridresolution(lua_State *s) {
	const BrushContext *ctx = luaBrush_context(s);
	if (ctx == nullptr) {
		return clua_error(s, "No brush context available");
	}
	lua_pushinteger(s, ctx->gridResolution);
	return 1;
}

static int luaBrush_targetvolumeregion(lua_State *s) {
	const BrushContext *ctx = luaBrush_context(s);
	if (ctx == nullptr) {
		return clua_error(s, "No brush context available");
	}
	return voxelgenerator::luaVoxel_pushregion(s, ctx->targetVolumeRegion);
}

static int luaBrush_brushgizmoactive(lua_State *s) {
	const BrushContext *ctx = luaBrush_context(s);
	if (ctx == nullptr) {
		return clua_error(s, "No brush context available");
	}
	lua_pushboolean(s, ctx->brushGizmoActive);
	return 1;
}

static int luaBrush_modifiertype(lua_State *s) {
	const BrushContext *ctx = luaBrush_context(s);
	if (ctx == nullptr) {
		return clua_error(s, "No brush context available");
	}
	const char *type = "place";
	if ((ctx->modifierType & ModifierType::Erase) != ModifierType::None) {
		type = "erase";
	} else if ((ctx->modifierType & ModifierType::Override) != ModifierType::None) {
		type = "override";
	} else if ((ctx->modifierType & ModifierType::Paint) != ModifierType::None) {
		type = "paint";
	} else if ((ctx->modifierType & ModifierType::NormalPaint) != ModifierType::None) {
		type = "normalpaint";
	} else if ((ctx->modifierType & ModifierType::ColorPicker) != ModifierType::None) {
		type = "colorpicker";
	}
	lua_pushstring(s, type);
	return 1;
}

static int luaBrush_referencepos_jsonhelp(lua_State *s) {
	const char *json = R"({
		"name": "referencePos",
		"summary": "Get the reference position set by the user before applying the brush.",
		"parameters": [],
		"returns": [
			{"type": "ivec3", "description": "The reference position as a table with x, y, z fields."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaBrush_cursorpos_jsonhelp(lua_State *s) {
	const char *json = R"({
		"name": "cursorPos",
		"summary": "Get the current cursor position.",
		"parameters": [],
		"returns": [
			{"type": "ivec3", "description": "The cursor position as a table with x, y, z fields."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaBrush_color_jsonhelp(lua_State *s) {
	const char *json = R"({
		"name": "color",
		"summary": "Get the current palette color index.",
		"parameters": [],
		"returns": [
			{"type": "integer", "description": "The palette color index."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaBrush_prevcursorpos_jsonhelp(lua_State *s) {
	const char *json = R"({
		"name": "prevCursorPos",
		"summary": "Get the cursor position before any clamping or brush execution was applied.",
		"parameters": [],
		"returns": [
			{"type": "ivec3", "description": "The previous cursor position as a table with x, y, z fields."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaBrush_hitcursorcolor_jsonhelp(lua_State *s) {
	const char *json = R"({
		"name": "hitCursorColor",
		"summary": "Get the palette color index of the voxel hit by raycast (before the hit face).",
		"parameters": [],
		"returns": [
			{"type": "integer", "description": "The palette color index of the hit voxel."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaBrush_voxelatcursorcolor_jsonhelp(lua_State *s) {
	const char *json = R"({
		"name": "voxelAtCursorColor",
		"summary": "Get the palette color index of the voxel at the cursor position (may be air = -1).",
		"parameters": [],
		"returns": [
			{"type": "integer", "description": "The palette color index of the voxel at cursor, or -1 for air."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaBrush_cursorface_jsonhelp(lua_State *s) {
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

static int luaBrush_lockedaxis_jsonhelp(lua_State *s) {
	const char *json = R"({
		"name": "lockedAxis",
		"summary": "Get the axis lock constraint for 2D operations.",
		"parameters": [],
		"returns": [
			{"type": "string", "description": "One of 'none', 'x', 'y', or 'z'."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaBrush_fixedorthosideview_jsonhelp(lua_State *s) {
	const char *json = R"({
		"name": "fixedOrthoSideView",
		"summary": "Check whether the view is an orthographic side view.",
		"parameters": [],
		"returns": [
			{"type": "boolean", "description": "True if in orthographic side view mode."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaBrush_normalpaint_jsonhelp(lua_State *s) {
	const char *json = R"({
		"name": "normalPaint",
		"summary": "Check whether normal painting mode is active.",
		"parameters": [],
		"returns": [
			{"type": "boolean", "description": "True if normal painting mode is active."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaBrush_normalindex_jsonhelp(lua_State *s) {
	const char *json = R"({
		"name": "normalIndex",
		"summary": "Get the normal index used for normal painting.",
		"parameters": [],
		"returns": [
			{"type": "integer", "description": "The normal index value."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaBrush_gridresolution_jsonhelp(lua_State *s) {
	const char *json = R"({
		"name": "gridResolution",
		"summary": "Get the grid resolution for snapping.",
		"parameters": [],
		"returns": [
			{"type": "integer", "description": "Grid resolution - voxels are placed at multiples of this value."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaBrush_targetvolumeregion_jsonhelp(lua_State *s) {
	const char *json = R"({
		"name": "targetVolumeRegion",
		"summary": "Get the region of the target volume for clamping the brush.",
		"parameters": [],
		"returns": [
			{"type": "Region", "description": "The region of the target volume."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaBrush_brushgizmoactive_jsonhelp(lua_State *s) {
	const char *json = R"({
		"name": "brushGizmoActive",
		"summary": "Check whether the brush gizmo is actively being manipulated.",
		"parameters": [],
		"returns": [
			{"type": "boolean", "description": "True if the gizmo is being manipulated."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaBrush_modifiertype_jsonhelp(lua_State *s) {
	const char *json = R"({
		"name": "modifierType",
		"summary": "Get the current modifier operation type.",
		"parameters": [],
		"returns": [
			{"type": "string", "description": "One of 'place', 'erase', 'override', 'paint', 'normalpaint', or 'colorpicker'."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

LuaBrush::LuaBrush(const io::FilesystemPtr &filesystem)
	: Brush(BrushType::Script, ModifierType::Place,
			(ModifierType::Place | ModifierType::Erase | ModifierType::Override)),
	  _filesystem(filesystem) {
}

bool LuaBrush::initLuaState() {
	_lua.resetState();
	if (!_noise.init()) {
		Log::warn("Failed to initialize noise for brush script");
	}
	voxelgenerator::luaVoxel_setGlobalData(_lua, voxelgenerator::luaVoxel_globalnoise(), &_noise);
	voxelgenerator::luaVoxel_setGlobalData(_lua, voxelgenerator::luaVoxel_globaldirtyregions(), &_dirtyRegions);
	voxelgenerator::luaVoxel_prepareState(_lua);

	static const clua_Reg brushStateFuncs[] = {
		{"referencePos", luaBrush_referencepos, luaBrush_referencepos_jsonhelp},
		{"cursorPos", luaBrush_cursorpos, luaBrush_cursorpos_jsonhelp},
		{"prevCursorPos", luaBrush_prevcursorpos, luaBrush_prevcursorpos_jsonhelp},
		{"color", luaBrush_color, luaBrush_color_jsonhelp},
		{"hitCursorColor", luaBrush_hitcursorcolor, luaBrush_hitcursorcolor_jsonhelp},
		{"voxelAtCursorColor", luaBrush_voxelatcursorcolor, luaBrush_voxelatcursorcolor_jsonhelp},
		{"cursorFace", luaBrush_cursorface, luaBrush_cursorface_jsonhelp},
		{"lockedAxis", luaBrush_lockedaxis, luaBrush_lockedaxis_jsonhelp},
		{"fixedOrthoSideView", luaBrush_fixedorthosideview, luaBrush_fixedorthosideview_jsonhelp},
		{"normalPaint", luaBrush_normalpaint, luaBrush_normalpaint_jsonhelp},
		{"normalIndex", luaBrush_normalindex, luaBrush_normalindex_jsonhelp},
		{"gridResolution", luaBrush_gridresolution, luaBrush_gridresolution_jsonhelp},
		{"targetVolumeRegion", luaBrush_targetvolumeregion, luaBrush_targetvolumeregion_jsonhelp},
		{"brushGizmoActive", luaBrush_brushgizmoactive, luaBrush_brushgizmoactive_jsonhelp},
		{"modifierType", luaBrush_modifiertype, luaBrush_modifiertype_jsonhelp},
		{nullptr, nullptr, nullptr}};
	clua_registerfuncsglobal(_lua, brushStateFuncs, luaBrush_metaname(), "g_brushcontext");

	return true;
}

bool LuaBrush::init() {
	if (!Super::init()) {
		return false;
	}
	return true;
}

void LuaBrush::shutdown() {
	_lua.resetState();
	_noise.shutdown();
	_scriptLoaded = false;
	_hasCalcRegion = false;
	_hasGizmo = false;
	_hasApplyGizmo = false;
	_useSimplePreview = false;
	_scriptSource.clear();
	_parameterDescription.clear();
	_parameters.clear();
	Super::shutdown();
}

void LuaBrush::reset() {
	Super::reset();
}

bool LuaBrush::loadScript(const core::String &filename) {
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
		path = core::string::path("brushes", path);
	}

	_scriptSource = _filesystem->load(path);
	if (_scriptSource.empty()) {
		Log::error("Failed to load brush script: %s", path.c_str());
		return false;
	}

	if (!initLuaState()) {
		return false;
	}

	// Run the script once to register global functions
	const int top = lua_gettop(_lua);
	if (luaL_dostring(_lua, _scriptSource.c_str())) {
		Log::error("Failed to run brush script %s: %s", filename.c_str(), lua_tostring(_lua, -1));
		lua_pop(_lua, 1);
		return false;
	}
	lua_settop(_lua, top);

	// Verify that generate() function exists
	lua_getglobal(_lua, "generate");
	if (!lua_isfunction(_lua, -1)) {
		Log::error("Brush script %s has no generate() function", filename.c_str());
		lua_pop(_lua, 1);
		return false;
	}
	lua_pop(_lua, 1);

	// Get description if available
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

	// Check for calcregion() callback
	lua_getglobal(_lua, "calcregion");
	_hasCalcRegion = lua_isfunction(_lua, -1);
	lua_pop(_lua, 1);

	// Check for gizmo callbacks
	lua_getglobal(_lua, "gizmo");
	_hasGizmo = lua_isfunction(_lua, -1);
	lua_pop(_lua, 1);

	lua_getglobal(_lua, "applygizmo");
	_hasApplyGizmo = lua_isfunction(_lua, -1);
	lua_pop(_lua, 1);

	// Get argument info if available
	lua_getglobal(_lua, "arguments");
	if (lua_isfunction(_lua, -1)) {
		lua_pop(_lua, 1); // pop the function, let argumentInfo handle it
		// Use a temporary lua state for parameter parsing since argumentInfo calls the function
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

	// Check for settings() callback to configure preview mode
	_useSimplePreview = false;
	lua_getglobal(_lua, "settings");
	if (lua_isfunction(_lua, -1)) {
		if (lua_pcall(_lua, 0, 1, 0) == LUA_OK) {
			if (lua_istable(_lua, -1)) {
				lua_getfield(_lua, -1, "preview");
				if (lua_isstring(_lua, -1)) {
					const char *previewMode = lua_tostring(_lua, -1);
					_useSimplePreview = SDL_strcmp(previewMode, "simple") == 0;
				}
				lua_pop(_lua, 1); // pop preview field
			}
			lua_pop(_lua, 1); // pop settings table
		} else {
			Log::warn("Error calling settings() in %s: %s", filename.c_str(),
					  lua_isstring(_lua, -1) ? lua_tostring(_lua, -1) : "Unknown Error");
			lua_pop(_lua, 1);
		}
	} else {
		lua_pop(_lua, 1);
	}

	// Get icon name if available
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

	_scriptLoaded = true;
	markDirty();
	Log::debug("Loaded brush script: %s", filename.c_str());
	return true;
}

struct IconMapping {
	const char *name;
	const char *icon;
};

static const IconMapping iconMappings[] = {
	{"blend", ICON_LC_BLEND},
	{"box", ICON_LC_BOX},
	{"boxes", ICON_LC_BOXES},
	{"brush", ICON_LC_BRUSH},
	{"circle", ICON_LC_CIRCLE},
	{"cloud", ICON_LC_CLOUD},
	{"diamond", ICON_LC_DIAMOND},
	{"eraser", ICON_LC_ERASER},
	{"expand", ICON_LC_EXPAND},
	{"flame", ICON_LC_FLAME},
	{"footprints", ICON_LC_FOOTPRINTS},
	{"grid2x2", ICON_LC_GRID_2X2},
	{"grid3x3", ICON_LC_GRID_3X3},
	{"group", ICON_LC_GROUP},
	{"hammer", ICON_LC_HAMMER},
	{"hexagon", ICON_LC_HEXAGON},
	{"image", ICON_LC_IMAGE},
	{"layers", ICON_LC_LAYERS},
	{"mountain", ICON_LC_MOUNTAIN},
	{"move", ICON_LC_MOVE},
	{"paintbrush", ICON_LC_PAINTBRUSH},
	{"palette", ICON_LC_PALETTE},
	{"penline", ICON_LC_PEN_LINE},
	{"pencil", ICON_LC_PENCIL},
	{"pipette", ICON_LC_PIPETTE},
	{"ruler", ICON_LC_RULER},
	{"scan", ICON_LC_SCAN},
	{"scroll", ICON_LC_SCROLL},
	{"snowflake", ICON_LC_SNOWFLAKE},
	{"sparkles", ICON_LC_SPARKLES},
	{"spray", ICON_LC_SPRAY_CAN},
	{"square", ICON_LC_SQUARE},
	{"stamp", ICON_LC_STAMP},
	{"star", ICON_LC_STAR},
	{"sun", ICON_LC_SUN},
	{"swords", ICON_LC_SWORDS},
	{"target", ICON_LC_TARGET},
	{"trees", ICON_LC_TREES},
	{"triangle", ICON_LC_TRIANGLE},
	{"wand", ICON_LC_WAND},
	{"waves", ICON_LC_WAVES},
	{"zap", ICON_LC_ZAP},
};

const char *LuaBrush::iconString() const {
	if (_iconName.empty()) {
		return ICON_LC_SCROLL;
	}
	for (int i = 0; i < (int)lengthof(iconMappings); ++i) {
		if (SDL_strcasecmp(_iconName.c_str(), iconMappings[i].name) == 0) {
			return iconMappings[i].icon;
		}
	}
	return ICON_LC_SCROLL;
}

core::String LuaBrush::scriptName() const {
	return core::string::extractFilename(_scriptFilename);
}

void LuaBrush::update(const BrushContext &ctx, double nowSeconds) {
	Super::update(ctx, nowSeconds);
	if (_lastCursorPosition != ctx.cursorPosition) {
		_lastCursorPosition = ctx.cursorPosition;
		markDirty();
	}
}

bool LuaBrush::active() const {
	return _scriptLoaded;
}

voxel::Region LuaBrush::calcRegion(const BrushContext &ctx) const {
	const glm::ivec3 &pos = ctx.cursorPosition;
	if (!_hasCalcRegion || !_scriptLoaded) {
		return voxel::Region(pos, pos);
	}

	lua_State *s = _lua.state();
	const int top = lua_gettop(s);

	BrushContext ctxCopy = ctx;
	voxelgenerator::luaVoxel_setGlobalData(s, luaBrush_metaname(), &ctxCopy);

	lua_getglobal(s, "calcregion");
	if (!lua_isfunction(s, -1)) {
		lua_settop(s, top);
		return voxel::Region(pos, pos);
	}

	// Push cursor position
	lua_pushinteger(s, pos.x);
	lua_pushinteger(s, pos.y);
	lua_pushinteger(s, pos.z);

	// Push user-defined arguments as typed values
	int nargs = 3;
	for (size_t i = 0; i < _parameterDescription.size(); ++i) {
		const voxelgenerator::LUAParameterDescription &p = _parameterDescription[i];
		const core::String &arg = i < _parameters.size() ? _parameters[i] : p.defaultValue;
		switch (p.type) {
		case voxelgenerator::LUAParameterType::Integer:
		case voxelgenerator::LUAParameterType::ColorIndex:
			lua_pushinteger(s, core::string::toInt(arg));
			break;
		case voxelgenerator::LUAParameterType::Float:
			lua_pushnumber(s, core::string::toFloat(arg));
			break;
		case voxelgenerator::LUAParameterType::Boolean:
			lua_pushboolean(s, core::string::toBool(arg) ? 1 : 0);
			break;
		default:
			lua_pushstring(s, arg.c_str());
			break;
		}
		++nargs;
	}

	if (lua_pcall(s, nargs, 6, 0) != LUA_OK) {
		Log::warn("Error calling calcregion(): %s", lua_tostring(s, -1));
		lua_settop(s, top);
		return voxel::Region(pos, pos);
	}

	const int minX = (int)lua_tointeger(s, -6);
	const int minY = (int)lua_tointeger(s, -5);
	const int minZ = (int)lua_tointeger(s, -4);
	const int maxX = (int)lua_tointeger(s, -3);
	const int maxY = (int)lua_tointeger(s, -2);
	const int maxZ = (int)lua_tointeger(s, -1);
	lua_settop(s, top);

	return voxel::Region(glm::ivec3(minX, minY, minZ), glm::ivec3(maxX, maxY, maxZ));
}

void LuaBrush::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
						const voxel::Region &region) {
	if (!_scriptLoaded) {
		return;
	}

	_dirtyRegions.clear();

	lua_State *s = _lua.state();

	// Set the scenegraph as global data so Lua bindings can access it
	voxelgenerator::luaVoxel_setGlobalData(s, voxelgenerator::luaVoxel_globalscenegraph(), &sceneGraph);

	BrushContext ctxCopy = ctx;
	voxelgenerator::luaVoxel_setGlobalData(s, luaBrush_metaname(), &ctxCopy);

	// Re-run the script source to ensure global functions are defined
	// (the lua state is persistent, but we need fresh function references)
	const int top = lua_gettop(s);
	if (luaL_dostring(s, _scriptSource.c_str())) {
		Log::error("Failed to reload brush script: %s", lua_tostring(s, -1));
		lua_pop(s, 1);
		return;
	}
	lua_settop(s, top);

	// Call generate(node, region, color, ...)
	lua_getglobal(s, "generate");
	if (!lua_isfunction(s, -1)) {
		Log::error("Brush script lost its generate() function");
		lua_pop(s, 1);
		return;
	}

	// Push the scene graph node
	scenegraph::SceneGraphNode &node = wrapper.node();
	if (voxelgenerator::luaVoxel_pushscenegraphnode(s, node) == 0) {
		Log::error("Failed to push scene graph node for brush script");
		lua_pop(s, 1);
		return;
	}

	// Push the region
	if (voxelgenerator::luaVoxel_pushregion(s, region) == 0) {
		Log::error("Failed to push region for brush script");
		lua_pop(s, 2);
		lua_gc(s, LUA_GCCOLLECT, 0);
		return;
	}

	// Push the cursor voxel color
	lua_pushinteger(s, ctx.cursorVoxel.getColor());

	// Push user-defined arguments
	int nargs = 3;
	if (!_parameterDescription.empty()) {
		const palette::Palette &palette = wrapper.node().palette();
		if (!voxelgenerator::luaVoxel_pushargs(s, _parameters, _parameterDescription, palette)) {
			Log::error("Failed to push brush script arguments");
			lua_settop(s, top);
			lua_gc(s, LUA_GCCOLLECT, 0);
			return;
		}
		nargs += (int)_parameterDescription.size();
	}

	// Call the function
	if (lua_pcall(s, nargs, 0, 0) != LUA_OK) {
		Log::error("Error executing brush script: %s", lua_tostring(s, -1));
		lua_pop(s, 1);
		lua_gc(s, LUA_GCCOLLECT, 0);
		return;
	}

	// Mark the region as dirty on the wrapper so the undo system tracks it
	if (region.isValid()) {
		wrapper.addToDirtyRegion(region.getLowerCorner());
		wrapper.addToDirtyRegion(region.getUpperCorner());
	}

	// Force GC to collect the LuaRawVolumeWrapper now, while the scene graph node
	// is still alive. Otherwise the wrapper persists until lua_close() during shutdown,
	// at which point the node may already be destroyed (causing a dangling pointer assert).
	lua_gc(s, LUA_GCCOLLECT, 0);
}

uint32_t LuaBrush::mapGizmoOperation(const char *name) {
	if (SDL_strcmp(name, "translate") == 0) {
		return BrushGizmo_Translate;
	}
	if (SDL_strcmp(name, "translatex") == 0) {
		return BrushGizmo_TranslateX;
	}
	if (SDL_strcmp(name, "translatey") == 0) {
		return BrushGizmo_TranslateY;
	}
	if (SDL_strcmp(name, "translatez") == 0) {
		return BrushGizmo_TranslateZ;
	}
	if (SDL_strcmp(name, "rotate") == 0) {
		return BrushGizmo_Rotate;
	}
	if (SDL_strcmp(name, "scale") == 0) {
		return BrushGizmo_Scale;
	}
	if (SDL_strcmp(name, "bounds") == 0) {
		return BrushGizmo_Bounds;
	}
	if (SDL_strcmp(name, "line") == 0) {
		return BrushGizmo_Line;
	}
	Log::warn("Unknown gizmo operation: %s", name);
	return BrushGizmo_None;
}

bool LuaBrush::wantBrushGizmo(const BrushContext &ctx) const {
	if (!_hasGizmo || !_scriptLoaded) {
		return false;
	}

	lua_State *s = _lua.state();
	const int top = lua_gettop(s);

	lua_getglobal(s, "gizmo");
	if (!lua_isfunction(s, -1)) {
		lua_settop(s, top);
		return false;
	}

	if (lua_pcall(s, 0, 1, 0) != LUA_OK) {
		Log::warn("Error calling gizmo(): %s", lua_tostring(s, -1));
		lua_settop(s, top);
		return false;
	}

	const bool hasGizmo = lua_istable(s, -1);
	lua_settop(s, top);
	return hasGizmo;
}

void LuaBrush::brushGizmoState(const BrushContext &ctx, BrushGizmoState &state) const {
	if (!_hasGizmo || !_scriptLoaded) {
		state.operations = BrushGizmo_None;
		return;
	}

	lua_State *s = _lua.state();
	const int top = lua_gettop(s);

	lua_getglobal(s, "gizmo");
	if (!lua_isfunction(s, -1)) {
		lua_settop(s, top);
		state.operations = BrushGizmo_None;
		return;
	}

	if (lua_pcall(s, 0, 1, 0) != LUA_OK) {
		Log::warn("Error calling gizmo(): %s", lua_tostring(s, -1));
		lua_settop(s, top);
		state.operations = BrushGizmo_None;
		return;
	}

	if (!lua_istable(s, -1)) {
		lua_settop(s, top);
		state.operations = BrushGizmo_None;
		return;
	}

	// Parse position → matrix
	lua_getfield(s, -1, "position");
	if (lua_istable(s, -1)) {
		lua_rawgeti(s, -1, 1);
		lua_rawgeti(s, -2, 2);
		lua_rawgeti(s, -3, 3);
		const float px = (float)lua_tonumber(s, -3);
		const float py = (float)lua_tonumber(s, -2);
		const float pz = (float)lua_tonumber(s, -1);
		lua_pop(s, 3);
		state.matrix = glm::translate(glm::mat4(1.0f), glm::vec3(px, py, pz));
	}
	lua_pop(s, 1); // pop position

	// Parse operations array
	state.operations = BrushGizmo_None;
	lua_getfield(s, -1, "operations");
	if (lua_istable(s, -1)) {
		const int len = (int)lua_rawlen(s, -1);
		for (int i = 1; i <= len; ++i) {
			lua_rawgeti(s, -1, i);
			if (lua_isstring(s, -1)) {
				state.operations |= mapGizmoOperation(lua_tostring(s, -1));
			}
			lua_pop(s, 1);
		}
	}
	lua_pop(s, 1); // pop operations

	// Parse snap
	lua_getfield(s, -1, "snap");
	if (lua_isnumber(s, -1)) {
		state.snap = (float)lua_tonumber(s, -1);
	}
	lua_pop(s, 1);

	// Parse localMode
	lua_getfield(s, -1, "localMode");
	if (lua_isboolean(s, -1)) {
		state.localMode = lua_toboolean(s, -1) != 0;
	}
	lua_pop(s, 1);

	// Parse positions array for line visualization
	lua_getfield(s, -1, "positions");
	if (lua_istable(s, -1)) {
		state.numPositions = 0;
		const int len = (int)lua_rawlen(s, -1);
		const int maxPositions = (int)(sizeof(state.positions) / sizeof(state.positions[0]));
		for (int i = 1; i <= len && state.numPositions < maxPositions; ++i) {
			lua_rawgeti(s, -1, i);
			if (lua_istable(s, -1)) {
				lua_rawgeti(s, -1, 1);
				lua_rawgeti(s, -2, 2);
				lua_rawgeti(s, -3, 3);
				state.positions[state.numPositions].x = (float)lua_tonumber(s, -3);
				state.positions[state.numPositions].y = (float)lua_tonumber(s, -2);
				state.positions[state.numPositions].z = (float)lua_tonumber(s, -1);
				lua_pop(s, 3);
				++state.numPositions;
			}
			lua_pop(s, 1);
		}
	}
	lua_pop(s, 1); // pop positions

	lua_settop(s, top);
}

bool LuaBrush::applyBrushGizmo(BrushContext &ctx, const glm::mat4 &matrix, const glm::mat4 &deltaMatrix,
							   uint32_t operation) {
	if (!_hasApplyGizmo || !_scriptLoaded) {
		return false;
	}

	lua_State *s = _lua.state();
	const int top = lua_gettop(s);

	lua_getglobal(s, "applygizmo");
	if (!lua_isfunction(s, -1)) {
		lua_settop(s, top);
		return false;
	}

	// Extract translation from delta matrix
	const glm::vec3 translation(deltaMatrix[3]);

	// Extract rotation (euler angles in degrees) from delta matrix
	// Use the rotation part of the delta matrix (upper-left 3x3)
	const glm::mat3 rotMat(deltaMatrix);
	const glm::vec3 rotation = glm::degrees(glm::eulerAngles(glm::quat_cast(rotMat)));

	// Extract scale from delta matrix columns
	const glm::vec3 scale(glm::length(glm::vec3(deltaMatrix[0])), glm::length(glm::vec3(deltaMatrix[1])),
						  glm::length(glm::vec3(deltaMatrix[2])));

	// Push translation table
	lua_createtable(s, 0, 3);
	lua_pushnumber(s, translation.x);
	lua_setfield(s, -2, "x");
	lua_pushnumber(s, translation.y);
	lua_setfield(s, -2, "y");
	lua_pushnumber(s, translation.z);
	lua_setfield(s, -2, "z");

	// Push rotation table
	lua_createtable(s, 0, 3);
	lua_pushnumber(s, rotation.x);
	lua_setfield(s, -2, "x");
	lua_pushnumber(s, rotation.y);
	lua_setfield(s, -2, "y");
	lua_pushnumber(s, rotation.z);
	lua_setfield(s, -2, "z");

	// Push scale table
	lua_createtable(s, 0, 3);
	lua_pushnumber(s, scale.x);
	lua_setfield(s, -2, "x");
	lua_pushnumber(s, scale.y);
	lua_setfield(s, -2, "y");
	lua_pushnumber(s, scale.z);
	lua_setfield(s, -2, "z");

	// Push operation name string
	const char *opName = "unknown";
	if (operation & BrushGizmo_Translate)
		opName = "translate";
	else if (operation & BrushGizmo_TranslateX)
		opName = "translatex";
	else if (operation & BrushGizmo_TranslateY)
		opName = "translatey";
	else if (operation & BrushGizmo_TranslateZ)
		opName = "translatez";
	else if (operation & BrushGizmo_Rotate)
		opName = "rotate";
	else if (operation & BrushGizmo_Scale)
		opName = "scale";
	else if (operation & BrushGizmo_Bounds)
		opName = "bounds";
	else if (operation & BrushGizmo_Line)
		opName = "line";
	lua_pushstring(s, opName);

	if (lua_pcall(s, 4, 1, 0) != LUA_OK) {
		Log::warn("Error calling applygizmo(): %s", lua_tostring(s, -1));
		lua_settop(s, top);
		return false;
	}

	const bool changed = lua_toboolean(s, -1) != 0;
	lua_settop(s, top);

	if (changed) {
		markDirty();
	}
	return changed;
}

bool LuaBrush::apiJsonToStream(io::WriteStream &stream) {
	if (!initLuaState()) {
		Log::error("Failed to initialize LuaBrush lua state for API documentation");
		return false;
	}
	lua_State *s = _lua.state();

	if (!stream.writeString("{\"g_brushcontext\":{\"type\":\"global\",\"methods\":[", false)) {
		return false;
	}

	// Get methods from the g_brushcontext global table
	lua_getglobal(s, "g_brushcontext");
	if (!lua_istable(s, -1)) {
		lua_pop(s, 1);
		Log::error("g_brushcontext is not a table");
		return false;
	}

	bool firstMethod = true;
	lua_pushnil(s);
	while (lua_next(s, -2) != 0) {
		if (lua_type(s, -2) == LUA_TSTRING && lua_isfunction(s, -1)) {
			const char *methodName = lua_tostring(s, -2);
			if (methodName && methodName[0] != '_') {
				if (!firstMethod) {
					if (!stream.writeString(",", false)) {
						lua_pop(s, 3);
						return false;
					}
				}
				firstMethod = false;

				lua_CFunction jsonHelpFunc = clua_getjsonhelp(s, luaBrush_metaname(), methodName);
				if (jsonHelpFunc) {
					lua_pushcfunction(s, jsonHelpFunc);
					if (lua_pcall(s, 0, 1, 0) == LUA_OK && lua_isstring(s, -1)) {
						const char *helpJson = lua_tostring(s, -1);
						if (!stream.writeString(helpJson, false)) {
							lua_pop(s, 4);
							return false;
						}
					}
					lua_pop(s, 1);
				} else {
					core::String methodJson = core::String::format("{\"name\":\"%s\"}", methodName);
					if (!stream.writeString(methodJson.c_str(), false)) {
						lua_pop(s, 3);
						return false;
					}
				}
			}
		}
		lua_pop(s, 1);
	}
	lua_pop(s, 1); // pop g_brushcontext table

	if (!stream.writeString("]}}\n", false)) {
		return false;
	}

	return true;
}

} // namespace voxedit
