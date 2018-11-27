/**
 * @file
 */

#include "MeshLUAFunctions.h"
#include "commonlua/LUAFunctions.h"
#include "commonlua/LUA.h"
#include "Mesh.h"
#include "core/Log.h"

namespace video {

namespace {
static const char* MeshId = "MESH";
}

static int meshlua_addskin(lua_State* l) {
	Mesh* mesh = lua::LUA::globalData<Mesh>(l, MeshId);
	const char* skin = luaL_checkstring(l, 1);
	Log::debug("Add skin %s to model '%s'", skin, mesh->filename().c_str());
	//mesh->addSkin(skin);
	return 0;
}

static int meshlua_addanimation(lua_State* l) {
	Mesh* mesh = lua::LUA::globalData<Mesh>(l, MeshId);
	const char* name = luaL_checkstring(l, 1);
	const float startFrame = luaL_checknumber(l, 2);
	const float endFrame = luaL_checknumber(l, 3);
	const float fps = luaL_checknumber(l, 4);
	//mesh->addAnimation(name, startFrame, endFrame - startFrame, fps, false);
	return 0;
}

static int meshlua_setscale(lua_State* l) {
	Mesh* mesh = lua::LUA::globalData<Mesh>(l, MeshId);
	const glm::vec3* scale = clua_get<glm::vec3>(l, 1);
	mesh->setScale(*scale);
	return 0;
}

void meshlua_register(lua::LUA& lua, video::Mesh* mesh) {
	clua_vecregister<glm::vec3>(lua.state());
	lua.newGlobalData<Mesh>(MeshId, mesh);
	const luaL_Reg funcs[] = {
		{"addSkin", meshlua_addskin},
		{"addAnimation", meshlua_addanimation},
		{"setScale", meshlua_setscale},
		{nullptr, nullptr}
	};
	lua.reg("mesh", funcs);
}

}
