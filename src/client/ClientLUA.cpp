/**
 * @file
 */

#include "ClientLUA.h"
#include "Client.h"
#include "commonlua/LUA.h"
#include "commonlua/LUAFunctions.h"

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

static int clientlua_signup(lua_State* s) {
	Client* client = clientlua_ctx(s);

	clua_assert_argc(s, lua_gettop(s) == 2);
	const char* email = luaL_checkstring(s, 1);
	const char* password = luaL_checkstring(s, 2);
	if (client->signup(email, password)) {
		lua_pushboolean(s, 1);
	} else {
		lua_pushboolean(s, 0);
	}
	return 1;
}

static int clientlua_auth(lua_State* s) {
	Client* client = clientlua_ctx(s);

	clua_assert_argc(s, lua_gettop(s) == 2);
	const char* email = luaL_checkstring(s, 1);
	const char* password = luaL_checkstring(s, 2);
	if (client->auth(email, password)) {
		lua_pushboolean(s, 1);
	} else {
		lua_pushboolean(s, 0);
	}
	return 1;
}

static int clientlua_isconnected(lua_State* s) {
	Client* client = clientlua_ctx(s);
	lua_pushboolean(s, client->isConnected());
	return 1;
}

static int clientlua_isconnecting(lua_State* s) {
	Client* client = clientlua_ctx(s);
	lua_pushboolean(s, client->isConnecting());
	return 1;
}

void clientlua_init(lua_State* s) {
	const luaL_Reg funcs[] = {
		{"isConnected", clientlua_isconnected},
		{"isConnecting", clientlua_isconnecting},
		{"signup", clientlua_signup},
		{"auth", clientlua_auth},
		{"connect", clientlua_connect},
		{"disconnect", clientlua_disconnect},
		{nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, funcs, "_metaclient", "client");
}
