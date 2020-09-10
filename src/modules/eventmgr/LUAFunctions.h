/**
 * @file
 */

#pragma once

#include "EventConfigurationData.h"
#include "commonlua/LUA.h"

namespace eventmgr {

static EventMgr* luaGetContext(lua_State * l) {
	return lua::LUA::globalData<EventMgr>(l, "EventMgr");
}

static EventConfigurationData* luaGetEventConfigurationDataContext(lua_State * l, int n) {
	return lua::LUA::userData<EventConfigurationData>(l, n, "EventConfigurationData");
}

static int luaCreateEventConfigurationData(lua_State * l) {
	EventMgr *ctx = luaGetContext(l);
	const char* nameId = luaL_checkstring(l, 1);
	const char *typeStr = luaL_checkstring(l, 2);
	const Type type = getType(typeStr);
	const EventConfigurationDataPtr& eventConfig = ctx->createEventConfig(nameId, type);
	if (!eventConfig) {
		return lua::LUA::returnError(l, "Could not create event config for id '%s'", nameId);
	}
	lua::LUA::newUserdata(l, "EventConfigurationData", eventConfig.get());
	return 1;
}

static int luaEventConfigurationDataGC(lua_State * l) {
	return 0;
}

static int luaEventConfigurationDataToString(lua_State * l) {
	const EventConfigurationData *ctx = luaGetEventConfigurationDataContext(l, 1);
	lua_pushfstring(l, "eventconfig: %s (type: %s)", ctx->eventNameId.c_str(), network::EnumNameEventType(ctx->type));
	return 1;
}

static int luaEventConfigurationDataGetType(lua_State * l) {
	const EventConfigurationData *ctx = luaGetEventConfigurationDataContext(l, 1);
	lua_pushinteger(l, core::enumVal(ctx->type));
	return 1;
}

static int luaEventConfigurationDataGetName(lua_State * l) {
	const EventConfigurationData *ctx = luaGetEventConfigurationDataContext(l, 1);
	lua_pushfstring(l, "%s", ctx->eventNameId.c_str());
	return 1;
}

}
