/**
 * @file
 */

#include "LUAEventMgr.h"
#include "EventConfigurationData.h"
#include "EventMgr.h"
#include "commonlua/LUA.h"
#include "commonlua/LUAFunctions.h"

namespace eventmgr {

static const char* luaeventmgr_eventmgr() {
	return "__global_eventmgr";
}

static const char* luaeventmgr_metaevent() {
	return "__meta_event";
}

static EventMgr* luaeventmgr_geteventmgr(lua_State *s) {
	lua_getglobal(s, luaeventmgr_eventmgr());
	EventMgr *mgr = (EventMgr *)lua_touserdata(s, -1);
	lua_pop(s, 1);
	return mgr;
}

static EventConfigurationData* luaeventmgr_toevent(lua_State* s, int n) {
	return *(EventConfigurationData**)clua_getudata<EventConfigurationData*>(s, n, luaeventmgr_metaevent());
}

static int luaeventmgr_create_event(lua_State * l) {
	EventMgr *ctx = luaeventmgr_geteventmgr(l);
	const char* nameId = luaL_checkstring(l, 1);
	const char *typeStr = luaL_checkstring(l, 2);
	const Type type = getType(typeStr);
	const EventConfigurationDataPtr& eventConfig = ctx->createEventConfig(nameId, type);
	if (!eventConfig) {
		return clua_error(l, "Could not create event config for id '%s'", nameId);
	}
	return clua_pushudata(l, eventConfig.get(), luaeventmgr_metaevent());
}

static int luaeventmgr_event_gc(lua_State * l) {
	return 0;
}

static int luaeventmgr_event_tostring(lua_State * l) {
	const EventConfigurationData *ctx = luaeventmgr_toevent(l, 1);
	lua_pushfstring(l, "eventconfig: %s (type: %s)", ctx->eventNameId.c_str(), network::EnumNameEventType(ctx->type));
	return 1;
}

static int luaeventmgr_event_gettype(lua_State * l) {
	const EventConfigurationData *ctx = luaeventmgr_toevent(l, 1);
	lua_pushinteger(l, core::enumVal(ctx->type));
	return 1;
}

static int luaeventmgr_event_getname(lua_State * l) {
	const EventConfigurationData *ctx = luaeventmgr_toevent(l, 1);
	lua_pushfstring(l, "%s", ctx->eventNameId.c_str());
	return 1;
}

static void luaeventmgr_pusheventmgr(lua_State* s, EventMgr* mgr) {
	lua_pushlightuserdata(s, mgr);
	lua_setglobal(s, luaeventmgr_eventmgr());
}

void luaeventmgr_setup(lua_State* s, EventMgr* mgr) {
	static const luaL_Reg eventFuncs[] = {
		{"type",       luaeventmgr_event_gettype},
		{"name",       luaeventmgr_event_getname},
		{"__gc",       luaeventmgr_event_gc},
		{"__tostring", luaeventmgr_event_tostring},
		{nullptr, nullptr}
	};
	clua_registerfuncs(s, eventFuncs, luaeventmgr_metaevent());

	static const luaL_Reg attribFuncs[] = {
		{"create", luaeventmgr_create_event},
		{nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, attribFuncs, luaeventmgr_eventmgr(), "eventmgr");

	luaeventmgr_pusheventmgr(s, mgr);
}

}
