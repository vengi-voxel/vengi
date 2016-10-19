/***
 * @file LUAAIRegistry.h
 * @ingroup LUA
 */
#pragma once

#include "AIRegistry.h"
#include "LUAFunctions.h"
#include "tree/LUATreeNode.h"
#include "conditions/LUACondition.h"
#include "filter/LUAFilter.h"
#include "movement/LUASteering.h"

namespace ai {

/**
 * @brief Allows you to register lua @ai{TreeNode}s, @ai{Conditions}, @ai{Filter}s and @ai{ISteering}s.
 *
 * @see @ai{LUATreeNode}
 *
 * @par TreeNode
 * @code
 * local luanode = REGISTRY.createNode("SomeName")
 * function luanode:execute(ai, deltaMillis)
 *   print("Node execute called with parameters: ai=["..tostring(ai).."], deltaMillis=["..tostring(deltaMillis).."]")
 *   return FINISHED
 * end
 * @encode
 * The @ai{TreeNodeStatus} states are put into the global space. They are: @c UNKNOWN, @c CANNOTEXECUTE,
 * @c RUNNING, @c FINISHED, @c FAILED and @c EXCEPTION
 *
 * Use @c SomeName later on in your behaviour trees to use this @ai{ITreeNode}
 *
 * @par Conditions
 * @code
 * local luacondition = REGISTRY.createCondition("SomeName")
 * function luacondition:evaluate(ai)
 *   --print("Condition evaluate called with parameter: ai=["..tostring(ai).."]")
 *   return true
 * end
 * @encode
 *
 * Use @c SomeName later on in your behaviour trees to use this @ai{ICondition}
 *
 * @par IFilter
 * @code
 * local luafilter = REGISTRY.createFilter("SomeName")
 * function luafilter:filter(ai)
 * end
 * @endcode
 *
 * Use @c SomeName later on in your behaviour trees to use this @ai{ICondition}
 *
 * @par ISteering
 * @code
 * local luasteering = REGISTRY.createSteering("SomeName")
 * function luasteering:execute(ai, speed)
 *   -- return MoveVector
 *   return 0.0, 1.0, 0.0, 0.6
 * end
 * @endcode
 *
 * Use @c SomeName later on in your behaviour trees to use this @ai{ICondition}
 *
 * @par AI metatable
 * There is a metatable that you can modify by calling @ai{LUAAIRegistry::pushAIMetatable()}.
 * This metatable is applied to all @ai{AI} pointers that are forwarded to the lua functions.
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
		ScopedWriteLock scopedLock(r->_lock);
		r->_treeNodeFactories.emplace(type, factory);
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
		ScopedWriteLock scopedLock(r->_lock);
		r->_conditionFactories.emplace(type, factory);
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

		ScopedWriteLock scopedLock(r->_lock);
		r->_filterFactories.emplace(type, factory);
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

		ScopedWriteLock scopedLock(r->_lock);
		r->_steeringFactories.emplace(type, factory);
		return 1;
	}

public:
	LUAAIRegistry() {
		init();
	}

	std::vector<luaL_Reg> aiFuncs = {
		{"id", luaAI_aiid},
		{"time", luaAI_aitime},
		{"hasZone", luaAI_aihaszone},
		{"zone", luaAI_aigetzone},
		{"filteredEntities", luaAI_aifilteredentities},
		{"setFilteredEntities", luaAI_aisetfilteredentities},
		{"addFilteredEntity", luaAI_aiaddfilteredentity},
		{"character", luaAI_aigetcharacter},
		{"aggroMgr", luaAI_aigetaggromgr},
		{"__tostring", luaAI_aitostring},
		{"__gc", luaAI_aigc},
		{"__eq", luaAI_aieq},
		{nullptr, nullptr}
	};
	std::vector<luaL_Reg> vecFuncs = {
		{"__add", luaAI_vecadd},
		{"__sub", luaAI_vecsub},
		{"__mul", luaAI_vecdot},
		{"__div", luaAI_vecdiv},
		{"__unm", luaAI_vecnegate},
		{"__len", luaAI_veclen},
		{"__eq", luaAI_veceq},
		{"__tostring", luaAI_vectostring},
		{"__index", luaAI_vecindex},
		{"__newindex", luaAI_vecnewindex},
		{"dot", luaAI_vecdot},
		{nullptr, nullptr}
	};
	std::vector<luaL_Reg> zoneFuncs = {
		{"size", luaAI_zonesize},
		{"name", luaAI_zonename},
		{"ai", luaAI_zoneai},
		{"execute", luaAI_zoneexecute},
		{"groupMgr", luaAI_zonegroupmgr},
		{"__tostring", luaAI_zonetostring},
		{nullptr, nullptr}
	};
	std::vector<luaL_Reg> characterFuncs = {
		{"id", luaAI_characterid},
		{"position", luaAI_characterposition},
		{"setPosition", luaAI_charactersetposition},
		{"speed", luaAI_characterspeed},
		{"setSpeed", luaAI_charactersetspeed},
		{"orientation", luaAI_characterorientation},
		{"setOrientation", luaAI_charactersetorientation},
		{"setAttribute", luaAI_charactersetattribute},
		{"attributes", luaAI_characterattributes},
		{"__eq", luaAI_charactereq},
		{"__gc", luaAI_charactergc},
		{"__tostring", luaAI_charactertostring},
		{nullptr, nullptr}
	};
	std::vector<luaL_Reg> aggroMgrFuncs = {
		{"setReduceByRatio", luaAI_aggromgrsetreducebyratio},
		{"setReduceByValue", luaAI_aggromgrsetreducebyvalue},
		{"resetReduceValue", luaAI_aggromgrresetreducevalue},
		{"addAggro", luaAI_aggromgraddaggro},
		{"highestEntry", luaAI_aggromgrhighestentry},
		{"entries", luaAI_aggromgrentries},
		{"__tostring", luaAI_aggromgrtostring},
		{nullptr, nullptr}
	};
	std::vector<luaL_Reg> groupMgrFuncs = {
		{"add", luaAI_groupmgradd},
		{"remove", luaAI_groupmgrremove},
		{"isLeader", luaAI_groupmgrisleader},
		{"isInGroup", luaAI_groupmgrisingroup},
		{"isInAnyGroup", luaAI_groupmgrisinanygroup},
		{"size", luaAI_groupmgrsize},
		{"position", luaAI_groupmgrposition},
		{"leader", luaAI_groupmgrleader},
		{"__tostring", luaAI_groupmgrtostring},
		{nullptr, nullptr}
	};
	std::vector<luaL_Reg> registryFuncs = {
		{"createNode", luaAI_createnode},
		{"createCondition", luaAI_createcondition},
		{"createFilter", luaAI_createfilter},
		{"createSteering", luaAI_createsteering},
		{nullptr, nullptr}
	};

	static int luaAI_aisetfilteredentities(lua_State* s) {
		luaAI_AI* ai = luaAI_toai(s, 1);
		luaL_checktype(s, 2, LUA_TTABLE);

		const int n = lua_rawlen(s, 2);
		FilteredEntities v(n);
		for (int i = 1; i <= n; ++i) {
			lua_rawgeti(s, 2, i);
			const int top = lua_gettop(s);
			const CharacterId id = (CharacterId)luaL_checknumber(s, top);
			v[i - 1] = id;
			lua_pop(s, 1);
		}
		ai->ai->setFilteredEntities(v);
		return 0;
	}

	static int luaAI_aiaddfilteredentity(lua_State* s) {
		luaAI_AI* ai = luaAI_toai(s, 1);
		const CharacterId id = (CharacterId)luaL_checkinteger(s, 2);
		ai->ai->addFilteredEntity(id);
		return 0;
	}

	/**
	 * @brief Access to the lua state.
	 * @see pushAIMetatable()
	 */
	lua_State* getLuaState() {
		return _s;
	}

	/**
	 * @brief Pushes the AI metatable onto the stack. This allows anyone to modify it
	 * to provide own functions and data that is applied to the @c ai parameters of the
	 * lua functions.
	 * @note lua_ctxai() can be used in your lua c callbacks to get access to the
	 * @ai{AI} pointer: @code const AI* ai = lua_ctxai(s, 1); @endcode
	 */
	int pushAIMetatable() {
		ai_assert(_s != nullptr, "LUA state is not yet initialized");
		return luaL_getmetatable(_s, luaAI_metaai());
	}

	/**
	 * @brief Pushes the character metatable onto the stack. This allows anyone to modify it
	 * to provide own functions and data that is applied to the @c ai:character() value
	 */
	int pushCharacterMetatable() {
		ai_assert(_s != nullptr, "LUA state is not yet initialized");
		return luaL_getmetatable(_s, luaAI_metacharacter());
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

		luaAI_registerfuncs(_s, &registryFuncs.front(), "META_REGISTRY");
		lua_setglobal(_s, "REGISTRY");

		// TODO: random

		luaAI_globalpointer(_s, this, luaAI_metaregistry());

		luaAI_registerfuncs(_s, &aiFuncs.front(), luaAI_metaai());
		luaAI_registerfuncs(_s, &vecFuncs.front(), luaAI_metavec());
		luaAI_registerfuncs(_s, &zoneFuncs.front(), luaAI_metazone());
		luaAI_registerfuncs(_s, &characterFuncs.front(), luaAI_metacharacter());
		luaAI_registerfuncs(_s, &aggroMgrFuncs.front(), luaAI_metaaggromgr());
		luaAI_registerfuncs(_s, &groupMgrFuncs.front(), luaAI_metagroupmgr());

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
};

}
