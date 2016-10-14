/**
 * @file
 */
#pragma once

#include "AIRegistry.h"
#include "tree/LUATreeNode.h"
#include "conditions/LUACondition.h"
#include "filter/LUAFilter.h"
#include "movement/LUASteering.h"

namespace ai {

/**
 * @brief Allows you to register lua @ai{TreeNode}s, @ai{Conditions} and so on.
 *
 * @see @ai{LUATreeNode}
 *
 * @par TreeNode
 * @code
 *
 * @encode
 */
class LUAAIRegistry : public AIRegistry {
protected:
	lua_State* _s = nullptr;

	using LuaNodeFactory = LUATreeNode::LUATreeNodeFactory;
	typedef std::shared_ptr<LuaNodeFactory> LUATreeNodeFactoryPtr;
	typedef std::map<std::string, LUATreeNodeFactoryPtr> TreeNodeFactoryMap;

	using LuaConditionFactory = LUACondition::LUAConditionFactory;
	typedef std::shared_ptr<LuaConditionFactory> LUAConditionFactoryPtr;
	typedef std::map<std::string, LUAConditionFactoryPtr> ConditionFactoryMap;

	using LuaFilterFactory = LUAFilter::LUAFilterFactory;
	typedef std::shared_ptr<LuaFilterFactory> LUAFilterFactoryPtr;
	typedef std::map<std::string, LUAFilterFactoryPtr> FilterFactoryMap;

	using LuaSteeringFactory = movement::LUASteering::LUASteeringFactory;
	typedef std::shared_ptr<LuaSteeringFactory> LUASteeringFactoryPtr;
	typedef std::map<std::string, LUASteeringFactoryPtr> SteeringFactoryMap;

	ReadWriteLock _lock{"luaregistry"};
	TreeNodeFactoryMap _treeNodeFactories;
	ConditionFactoryMap _conditionFactories;
	FilterFactoryMap _filterFactories;
	SteeringFactoryMap _steeringFactories;

	static LUAAIRegistry* luaGetContext(lua_State * s) {
		lua_getglobal(s, "Registry");
		LUAAIRegistry* data = (LUAAIRegistry*) lua_touserdata(s, -1);
		lua_pop(s, 1);
		return data;
	}

	static LuaNodeFactory* luaGetNodeFactoryContext(lua_State * s, int n) {
		return *(LuaNodeFactory **) lua_touserdata(s, n);
	}

	static LuaConditionFactory* luaGetConditionFactoryContext(lua_State * s, int n) {
		return *(LuaConditionFactory **) lua_touserdata(s, n);
	}

	static LuaFilterFactory* luaGetFilterFactoryContext(lua_State * s, int n) {
		return *(LuaFilterFactory **) lua_touserdata(s, n);
	}

	static LuaSteeringFactory* luaGetSteeringFactoryContext(lua_State * s, int n) {
		return *(LuaSteeringFactory **) lua_touserdata(s, n);
	}

	static AI* luaGetAIContext(lua_State * s, int n) {
		return *(AI **) luaL_checkudata(s, n, LUATreeNode::luaAIMetaName());
	}

	static int luaNodeEmptyExecute(lua_State* s) {
		const LuaNodeFactory* factory = luaGetNodeFactoryContext(s, 1);
		return luaL_error(s, "There is no execute function set for node: %s", factory->type().c_str());
	}

	static int luaNodeToString(lua_State* s) {
		const LuaNodeFactory* factory = luaGetNodeFactoryContext(s, 1);
		lua_pushfstring(s, "node: %s", factory->type().c_str());
		return 1;
	}

	static int luaNewIndex(lua_State* s) {
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

	static int luaAiGC(lua_State* s) {
		return 0;
	}

	static int luaAiToString(lua_State* s) {
		const AI* ai = luaGetAIContext(s, 1);
		TreeNodePtr treeNode = ai->getBehaviour();
		if (treeNode) {
			lua_pushfstring(s, "ai: %s", treeNode->getName().c_str());
		} else {
			lua_pushstring(s, "ai: no behaviour tree set");
		}
		return 1;
	}

	static void setupMetatable(lua_State* s, const std::string& type, const luaL_Reg *funcs, const std::string& name) {
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
		luaL_setfuncs(s, funcs, 0);
		lua_setmetatable(s, -2);
	}

	static const luaL_Reg* nodeFuncs() {
		static const luaL_Reg nodes[] = {
			{"execute", luaNodeEmptyExecute},
			{"__tostring", luaNodeToString},
			{"__newindex", luaNewIndex},
			{nullptr, nullptr}
		};
		return nodes;
	}

	/**
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
	static int luaCreateNode(lua_State* s) {
		LUAAIRegistry* r = luaGetContext(s);
		const std::string type = luaL_checkstring(s, -1);
		const LUATreeNodeFactoryPtr& factory = std::make_shared<LuaNodeFactory>(s, type);
		const bool inserted = r->registerNodeFactory(type, *factory);
		if (!inserted) {
			return luaL_error(s, "tree node %s is already registered", type.c_str());
		}

		LuaNodeFactory ** udata = (LuaNodeFactory**) lua_newuserdata(s, sizeof(LuaNodeFactory*));
		*udata = factory.get();
		setupMetatable(s, type, nodeFuncs(), "node");
		ScopedWriteLock scopedLock(r->_lock);
		r->_treeNodeFactories.emplace(type, factory);

		return 1;
	}

	static int luaConditionEmptyEvaluate(lua_State* s) {
		const LuaConditionFactory* factory = luaGetConditionFactoryContext(s, 1);
		return luaL_error(s, "There is no evaluate function set for condition: %s", factory->type().c_str());
	}

	static int luaConditionToString(lua_State* s) {
		const LuaConditionFactory* factory = luaGetConditionFactoryContext(s, 1);
		lua_pushfstring(s, "condition: %s", factory->type().c_str());
		return 1;
	}

	static const luaL_Reg* conditionFuncs() {
		static const luaL_Reg nodes[] = {
			{"evaluate", luaConditionEmptyEvaluate},
			{"__tostring", luaConditionToString},
			{"__newindex", luaNewIndex},
			{nullptr, nullptr}
		};
		return nodes;
	}

	static int luaCreateCondition(lua_State* s) {
		LUAAIRegistry* r = luaGetContext(s);
		const std::string type = luaL_checkstring(s, -1);
		const LUAConditionFactoryPtr& factory = std::make_shared<LuaConditionFactory>(s, type);
		const bool inserted = r->registerConditionFactory(type, *factory);
		if (!inserted) {
			return luaL_error(s, "condition %s is already registered", type.c_str());
		}

		LuaConditionFactory ** udata = (LuaConditionFactory**) lua_newuserdata(s, sizeof(LuaConditionFactory*));
		*udata = factory.get();
		setupMetatable(s, type, conditionFuncs(), "condition");

		ScopedWriteLock scopedLock(r->_lock);
		r->_conditionFactories.emplace(type, factory);

		return 1;
	}

	static int luaFilterEmptyFilter(lua_State* s) {
		const LuaFilterFactory* factory = luaGetFilterFactoryContext(s, 1);
		return luaL_error(s, "There is no filter function set for filter: %s", factory->type().c_str());
	}

	static int luaFilterToString(lua_State* s) {
		const LuaFilterFactory* factory = luaGetFilterFactoryContext(s, 1);
		lua_pushfstring(s, "filter: %s", factory->type().c_str());
		return 1;
	}

	static const luaL_Reg* filterFuncs() {
		static const luaL_Reg nodes[] = {
			{"filter", luaFilterEmptyFilter},
			{"__tostring", luaFilterToString},
			{"__newindex", luaNewIndex},
			{nullptr, nullptr}
		};
		return nodes;
	}

	static int luaCreateFilter(lua_State* s) {
		LUAAIRegistry* r = luaGetContext(s);
		const std::string type = luaL_checkstring(s, -1);
		const LUAFilterFactoryPtr& factory = std::make_shared<LuaFilterFactory>(s, type);
		const bool inserted = r->registerFilterFactory(type, *factory);
		if (!inserted) {
			return luaL_error(s, "filter %s is already registered", type.c_str());
		}

		LuaFilterFactory ** udata = (LuaFilterFactory**) lua_newuserdata(s, sizeof(LuaFilterFactory*));
		*udata = factory.get();
		setupMetatable(s, type, filterFuncs(), "filter");

		ScopedWriteLock scopedLock(r->_lock);
		r->_filterFactories.emplace(type, factory);

		return 1;
	}

	static int luaSteeringEmptyExecute(lua_State* s) {
		const LuaSteeringFactory* factory = luaGetSteeringFactoryContext(s, 1);
		return luaL_error(s, "There is no execute() function set for steering: %s", factory->type().c_str());
	}

	static int luaSteeringToString(lua_State* s) {
		const LuaSteeringFactory* factory = luaGetSteeringFactoryContext(s, 1);
		lua_pushfstring(s, "steering: %s", factory->type().c_str());
		return 1;
	}

	static const luaL_Reg* steeringFuncs() {
		static const luaL_Reg nodes[] = {
			{"filter", luaSteeringEmptyExecute},
			{"__tostring", luaSteeringToString},
			{"__newindex", luaNewIndex},
			{nullptr, nullptr}
		};
		return nodes;
	}

	static int luaCreateSteering(lua_State* s) {
		LUAAIRegistry* r = luaGetContext(s);
		const std::string type = luaL_checkstring(s, -1);
		const LUASteeringFactoryPtr& factory = std::make_shared<LuaSteeringFactory>(s, type);
		const bool inserted = r->registerSteeringFactory(type, *factory);
		if (!inserted) {
			return luaL_error(s, "steering %s is already registered", type.c_str());
		}

		LuaSteeringFactory ** udata = (LuaSteeringFactory**) lua_newuserdata(s, sizeof(LuaSteeringFactory*));
		*udata = factory.get();
		setupMetatable(s, type, steeringFuncs(), "steering");

		ScopedWriteLock scopedLock(r->_lock);
		r->_steeringFactories.emplace(type, factory);

		return 1;
	}
public:
	LUAAIRegistry() {
		init();
	}

	/**
	 * @brief Access to the lua state.
	 */
	lua_State* getLuaState() {
		return _s;
	}

	/**
	 * @see shutdown()
	 */
	bool init() {
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

		luaL_Reg registryFuncs[] = {
			{"createNode", luaCreateNode},
			{"createCondition", luaCreateCondition},
			{"createFilter", luaCreateFilter},
			{"createSteering", luaCreateSteering},
			{nullptr, nullptr}
		};

		luaL_newmetatable(_s, "META_REGISTRY");
		lua_pushvalue(_s, -1);
		lua_setfield(_s, -2, "__index");
		luaL_setfuncs(_s, registryFuncs, 0);
		lua_setglobal(_s, "REGISTRY");

		lua_pushlightuserdata(_s, this);
		lua_setglobal(_s, "Registry");

		luaL_Reg aiFuncs[] = {
			// TODO: make this extensible from outside
			{"__tostring", luaAiToString},
			{"__gc", luaAiGC},
			{nullptr, nullptr}
		};

		luaL_newmetatable(_s, LUATreeNode::luaAIMetaName());
		// assign the metatable to __index
		lua_pushvalue(_s, -1);
		lua_setfield(_s, -2, "__index");
		luaL_setfuncs(_s, aiFuncs, 0);

		const char* script = ""
			"UNKNOWN, CANNOTEXECUTE, RUNNING, FINISHED, FAILED, EXCEPTION = 0, 1, 2, 3, 4, 5\n";

		if (luaL_loadbufferx(_s, script, strlen(script), "", nullptr) || lua_pcall(_s, 0, 0, 0)) {
			ai_log_error("%s", lua_tostring(_s, -1));
			lua_pop(_s, 1);
			return false;
		}
		return true;
	}

	/**
	 * @see init()
	 */
	void shutdown() {
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

	~LUAAIRegistry() {
		shutdown();
	}

	inline bool evaluate(const std::string& str) {
		return evaluate(str.c_str(), str.length());
	}

	/**
	 * @brief Load your lua scripts into the lua state of the registry.
	 * This can be called multiple times to e.g. load multiple files.
	 * @return @c true if the lua script was loaded, @c false otherwise
	 * @note you have to call init() before
	 */
	bool evaluate(const char* luaBuffer, size_t size) {
		if (_s == nullptr) {
			return false;
		}
		if (luaL_loadbufferx(_s, luaBuffer, size, "", nullptr) || lua_pcall(_s, 0, 0, 0)) {
			ai_log_error("%s", lua_tostring(_s, -1));
			lua_pop(_s, 1);
			return false;
		}
		return true;
	}
};

}
