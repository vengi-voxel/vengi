/**
 * @file
 */

#include "commonlua/LUA.h"

static inline Client* clientlua_ctx(lua_State* s) {
	Client* client = lua::LUA::globalData<Client>(s, "clientpointer");
	core_assert(client != nullptr);
	return client;
}

static int clientlua_disconnect(lua_State* s) {
	Client* client = clientlua_ctx(s);
	client->disconnect();
	return 0;
}

static int clientlua_connect(lua_State* s) {
	Client* client = clientlua_ctx(s);

	clua_assert_argc(s, lua_gettop(s) == 2);
	const int port = luaL_checkinteger(s, 1);
	const char* host = luaL_checkstring(s, 2);
	if (client->connect(port, host)) {
		lua_pushboolean(s, 1);
	} else {
		lua_pushboolean(s, 0);
	}
	return 1;
}
