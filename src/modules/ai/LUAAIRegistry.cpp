/***
 * @file LUAAIRegistry.h
 * @ingroup LUA
 */

#include "LUAAIRegistry.h"
#include "LUAFunctions.h"
#include "common/Common.h"
#include "common/Assert.h"
#include "AI.h"

namespace ai {

static void luaAI_setupmetatable(lua_State* s, const std::string& type, const luaL_Reg *funcs, const std::string& name) {
	const std::string& metaFull = "__meta_" + name + "_" + type;
	// make global
	lua_setfield(s, LUA_REGISTRYINDEX, metaFull.c_str());
	// put back onto stack
	lua_getfield(s, LUA_REGISTRYINDEX, metaFull.c_str());

	// setup meta table - create a new one manually, otherwise we aren't
	// able to override the particular function on a per instance base. Also
	// this 'metatable' must not be in the global registry.
	lua_createtable(s, 0, 2);
	lua_pushvalue(s, -1);
	lua_setfield(s, -2, "__index");
	lua_pushstring(s, name.c_str());
	lua_setfield(s, -2, "__name");
	lua_pushstring(s, type.c_str());
	lua_setfield(s, -2, "type");
	luaL_setfuncs(s, funcs, 0);
	lua_setmetatable(s, -2);
}

static int luaAI_newindex(lua_State* s) {
	// -3 is userdata
	lua_getmetatable(s, -3);
	// -3 is now the field string
	const char *field = luaL_checkstring(s, -3);
	// push -2 to -1 (the value)
	lua_pushvalue(s, -2);
	// set the value into the field
	lua_setfield(s, -2, field);
	lua_pop(s, 1);
	return 0;
}

static void luaAI_globalpointer(lua_State* s, void* pointer, const char *name) {
	lua_pushlightuserdata(s, pointer);
	lua_setglobal(s, name);
}

/***
 * Gives you access the the light userdata for the LUAAIRegistry.
 * @return the registry userdata
 */
static LUAAIRegistry* luaAI_toregistry(lua_State * s) {
	return luaAI_getlightuserdata<LUAAIRegistry>(s, luaAI_metaregistry());
}

/***
 * Gives you access the the userdata for the LuaNodeFactory instance you are operating on.
 * @return the node factory userdata
 */
static LuaNodeFactory* luaAI_tonodefactory(lua_State * s, int n) {
	return *(LuaNodeFactory **) lua_touserdata(s, n);
}

/***
 * Gives you access the the userdata for the LuaConditionFactory instance you are operating on.
 * @return the condition factory userdata
 */
static LuaConditionFactory* luaAI_toconditionfactory(lua_State * s, int n) {
	return *(LuaConditionFactory **) lua_touserdata(s, n);
}

/***
 * Gives you access the the userdata for the LuaFilterFactory instance you are operating on.
 * @return the filter factory userdata
 */
static LuaFilterFactory* luaGetFilterFactoryContext(lua_State * s, int n) {
	return *(LuaFilterFactory **) lua_touserdata(s, n);
}

/***
 * Gives you access the the userdata for the LuaSteeringFactory instance you are operating on.
 * @return the steering factory userdata
 */
static LuaSteeringFactory* luaAI_tosteeringfactory(lua_State * s, int n) {
	return *(LuaSteeringFactory **) lua_touserdata(s, n);
}

/***
 * Empty (default) execute() function that just throws an error.
 * @param ai The ai to execute the tree node on
 * @param deltaMillis Milliseconds since last execution
 * @return throws error because execute() function wasn't overridden
 */
static int luaAI_nodeemptyexecute(lua_State* s) {
	const LuaNodeFactory* factory = luaAI_tonodefactory(s, 1);
	return luaL_error(s, "There is no execute function set for node: %s", factory->type().c_str());
}

static int luaAI_nodetostring(lua_State* s) {
	const LuaNodeFactory* factory = luaAI_tonodefactory(s, 1);
	lua_pushfstring(s, "node: %s", factory->type().c_str());
	return 1;
}

/***
 * @brief Create a new lua @ai{TreeNode}
 *
 * @par lua parameters: #1 name of the node
 * @note you have to specify an @c execute method that accepts two parameters in your lua code. E.g. do it like this:
 * @code
 * local luatest = REGISTRY.createNode(\"LuaTest\")
 " function luatest:execute(ai, deltaMillis)
 "    return FAILED\n"
 " end
 * @endcode
 */
static int luaAI_createnode(lua_State* s) {
	LUAAIRegistry* r = luaAI_toregistry(s);
	const std::string type = luaL_checkstring(s, -1);
	const LUATreeNodeFactoryPtr& factory = std::make_shared<LuaNodeFactory>(s, type);
	const bool inserted = r->registerNodeFactory(type, *factory);
	if (!inserted) {
		return luaL_error(s, "tree node %s is already registered", type.c_str());
	}

	luaAI_newuserdata<LuaNodeFactory*>(s, factory.get());
	const luaL_Reg nodes[] = {
		{"execute", luaAI_nodeemptyexecute},
		{"__tostring", luaAI_nodetostring},
		{"__newindex", luaAI_newindex},
		{nullptr, nullptr}
	};
	luaAI_setupmetatable(s, type, nodes, "node");
	r->addTreeNodeFactory(type, factory);
	return 1;
}

/***
 * Empty (default) evaluate() function that just throws an error.
 * @param ai The ai to execute the condition node on
 * @return throws error because evaluate() function wasn't overridden
 */
static int luaAI_conditionemptyevaluate(lua_State* s) {
	const LuaConditionFactory* factory = luaAI_toconditionfactory(s, 1);
	return luaL_error(s, "There is no evaluate function set for condition: %s", factory->type().c_str());
}

static int luaAI_conditiontostring(lua_State* s) {
	const LuaConditionFactory* factory = luaAI_toconditionfactory(s, 1);
	lua_pushfstring(s, "condition: %s", factory->type().c_str());
	return 1;
}

/***
 * @param type The string that identifies the name that is used to register the condition under
 * @return userdata with a metatable for conditions
 */
static int luaAI_createcondition(lua_State* s) {
	LUAAIRegistry* r = luaAI_toregistry(s);
	const std::string type = luaL_checkstring(s, -1);
	const LUAConditionFactoryPtr& factory = std::make_shared<LuaConditionFactory>(s, type);
	const bool inserted = r->registerConditionFactory(type, *factory);
	if (!inserted) {
		return luaL_error(s, "condition %s is already registered", type.c_str());
	}

	luaAI_newuserdata<LuaConditionFactory*>(s, factory.get());
	const luaL_Reg nodes[] = {
		{"evaluate", luaAI_conditionemptyevaluate},
		{"__tostring", luaAI_conditiontostring},
		{"__newindex", luaAI_newindex},
		{nullptr, nullptr}
	};
	luaAI_setupmetatable(s, type, nodes, "condition");
	r->addConditionFactory(type, factory);
	return 1;
}

/***
 * Empty (default) filter() function that just throws an error.
 * @param ai The ai to execute the filter for
 * @return throws error because filter() function wasn't overridden
 */
static int luaAI_filteremptyfilter(lua_State* s) {
	const LuaFilterFactory* factory = luaGetFilterFactoryContext(s, 1);
	return luaL_error(s, "There is no filter function set for filter: %s", factory->type().c_str());
}

static int luaAI_filtertostring(lua_State* s) {
	const LuaFilterFactory* factory = luaGetFilterFactoryContext(s, 1);
	lua_pushfstring(s, "filter: %s", factory->type().c_str());
	return 1;
}

static int luaAI_createfilter(lua_State* s) {
	LUAAIRegistry* r = luaAI_toregistry(s);
	const std::string type = luaL_checkstring(s, -1);
	const LUAFilterFactoryPtr& factory = std::make_shared<LuaFilterFactory>(s, type);
	const bool inserted = r->registerFilterFactory(type, *factory);
	if (!inserted) {
		return luaL_error(s, "filter %s is already registered", type.c_str());
	}

	luaAI_newuserdata<LuaFilterFactory*>(s, factory.get());
	const luaL_Reg nodes[] = {
		{"filter", luaAI_filteremptyfilter},
		{"__tostring", luaAI_filtertostring},
		{"__newindex", luaAI_newindex},
		{nullptr, nullptr}
	};
	luaAI_setupmetatable(s, type, nodes, "filter");
	r->addFilterFactory(type, factory);
	return 1;
}

static int luaAI_steeringemptyexecute(lua_State* s) {
	const LuaSteeringFactory* factory = luaAI_tosteeringfactory(s, 1);
	return luaL_error(s, "There is no execute() function set for steering: %s", factory->type().c_str());
}

static int luaAI_steeringtostring(lua_State* s) {
	const LuaSteeringFactory* factory = luaAI_tosteeringfactory(s, 1);
	lua_pushfstring(s, "steering: %s", factory->type().c_str());
	return 1;
}

static int luaAI_createsteering(lua_State* s) {
	LUAAIRegistry* r = luaAI_toregistry(s);
	const std::string type = luaL_checkstring(s, -1);
	const LUASteeringFactoryPtr& factory = std::make_shared<LuaSteeringFactory>(s, type);
	const bool inserted = r->registerSteeringFactory(type, *factory);
	if (!inserted) {
		return luaL_error(s, "steering %s is already registered", type.c_str());
	}

	luaAI_newuserdata<LuaSteeringFactory*>(s, factory.get());
	const luaL_Reg nodes[] = {
		{"filter", luaAI_steeringemptyexecute},
		{"__tostring", luaAI_steeringtostring},
		{"__newindex", luaAI_newindex},
		{nullptr, nullptr}
	};
	luaAI_setupmetatable(s, type, nodes, "steering");
	r->addSteeringFactory(type, factory);
	return 1;
}

LUAAIRegistry::LUAAIRegistry() {
	init();
}

lua_State* LUAAIRegistry::getLuaState() {
	return _s;
}

int LUAAIRegistry::pushAIMetatable() {
	ai_assert(_s != nullptr, "LUA state is not yet initialized");
	return luaL_getmetatable(_s, luaAI_metaai());
}

int LUAAIRegistry::pushCharacterMetatable() {
	ai_assert(_s != nullptr, "LUA state is not yet initialized");
	return luaL_getmetatable(_s, luaAI_metacharacter());
}

static const luaL_Reg registryFuncs[] = {
	{"createNode", luaAI_createnode},
	{"createCondition", luaAI_createcondition},
	{"createFilter", luaAI_createfilter},
	{"createSteering", luaAI_createsteering},
	{nullptr, nullptr}
};

bool LUAAIRegistry::init() {
	if (_s != nullptr) {
		return true;
	}
	_s = luaL_newstate();

	lua_atpanic(_s, [] (lua_State* L) {
		ai_log_error("Lua panic. Error message: %s", (lua_isnil(L, -1) ? "" : lua_tostring(L, -1)));
		return 0;
	});
	lua_gc(_s, LUA_GCSTOP, 0);
	luaL_openlibs(_s);

	luaAI_registerfuncs(_s, registryFuncs, "META_REGISTRY");
	lua_setglobal(_s, "REGISTRY");

	// TODO: random

	luaAI_globalpointer(_s, this, luaAI_metaregistry());
	luaAI_registerAll(_s);

	const char* script = ""
		"UNKNOWN, CANNOTEXECUTE, RUNNING, FINISHED, FAILED, EXCEPTION = 0, 1, 2, 3, 4, 5\n";

	if (luaL_loadbufferx(_s, script, strlen(script), "", nullptr) || lua_pcall(_s, 0, 0, 0)) {
		ai_log_error("%s", lua_tostring(_s, -1));
		lua_pop(_s, 1);
		return false;
	}
	return true;
}

void LUAAIRegistry::shutdown() {
	{
		ScopedWriteLock scopedLock(_lock);
		_treeNodeFactories.clear();
		_conditionFactories.clear();
		_filterFactories.clear();
		_steeringFactories.clear();
	}
	if (_s != nullptr) {
		lua_close(_s);
		_s = nullptr;
	}
}

LUAAIRegistry::~LUAAIRegistry() {
	shutdown();
}

bool LUAAIRegistry::evaluate(const char* luaBuffer, size_t size) {
	if (_s == nullptr) {
		ai_log_debug("LUA state is not yet initialized");
		return false;
	}
	if (luaL_loadbufferx(_s, luaBuffer, size, "", nullptr) || lua_pcall(_s, 0, 0, 0)) {
		ai_log_error("%s", lua_tostring(_s, -1));
		lua_pop(_s, 1);
		return false;
	}
	return true;
}

void LUAAIRegistry::addTreeNodeFactory(const std::string& type, const LUATreeNodeFactoryPtr& factory) {
	ScopedWriteLock scopedLock(_lock);
	_treeNodeFactories.emplace(type, factory);
}

void LUAAIRegistry::addConditionFactory(const std::string& type, const LUAConditionFactoryPtr& factory) {
	ScopedWriteLock scopedLock(_lock);
	_conditionFactories.emplace(type, factory);
}

void LUAAIRegistry::addFilterFactory(const std::string& type, const LUAFilterFactoryPtr& factory) {
	ScopedWriteLock scopedLock(_lock);
	_filterFactories.emplace(type, factory);
}

void LUAAIRegistry::addSteeringFactory(const std::string& type, const LUASteeringFactoryPtr& factory) {
	ScopedWriteLock scopedLock(_lock);
	_steeringFactories.emplace(type, factory);
}

}
