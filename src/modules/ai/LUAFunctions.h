/***
 * @file LUAFunctions.h
 * @ingroup LUA
 */
#pragma once

#include "commonlua/LUA.h"

namespace ai {

struct luaAI_AI {
	AIPtr ai;
};

struct luaAI_ICharacter {
	ICharacterPtr character;
};

static inline const char *luaAI_metaai() {
	return "__meta_ai";
}

static inline const char *luaAI_metazone() {
	return "__meta_zone";
}

static inline const char* luaAI_metaaggromgr() {
	return "__meta_aggromgr";
}

static inline const char* luaAI_metaregistry() {
	return "__meta_registry";
}

static inline const char* luaAI_metagroupmgr() {
	return "__meta_groupmgr";
}

static inline const char* luaAI_metacharacter() {
	return "__meta_character";
}

static inline const char* luaAI_metavec() {
	return "__meta_vec";
}

static void luaAI_registerfuncs(lua_State* s, const luaL_Reg* funcs, const char *name) {
	luaL_newmetatable(s, name);
	// assign the metatable to __index
	lua_pushvalue(s, -1);
	lua_setfield(s, -2, "__index");
	luaL_setfuncs(s, funcs, 0);
}

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

template<class T>
static inline T luaAI_getudata(lua_State* s, int n, const char *name) {
	void* dataVoid = luaL_checkudata(s, n, name);
	if (dataVoid == nullptr) {
		std::string error(name);
		error.append(" userdata must not be null");
		luaL_argcheck(s, dataVoid != nullptr, n, error.c_str());
	}
	return (T) dataVoid;
}

template<class T>
static inline T* luaAI_newuserdata(lua_State* s, const T& data) {
	T* udata = (T*) lua_newuserdata(s, sizeof(T));
	*udata = data;
	return udata;
}

static void luaAI_globalpointer(lua_State* s, void* pointer, const char *name) {
	lua_pushlightuserdata(s, pointer);
	lua_setglobal(s, name);
}

static int luaAI_assignmetatable(lua_State* s, const char *name) {
	luaL_getmetatable(s, name);
#if AI_LUA_SANTITY
	if (!lua_istable(s, -1)) {
		ai_log_error("LUA: metatable for %s doesn't exist", name);
		return 0;
	}
#endif
	lua_setmetatable(s, -2);
	return 1;
}

template<class T>
static inline int luaAI_pushudata(lua_State* s, const T& data, const char *name) {
	luaAI_newuserdata<T>(s, data);
	return luaAI_assignmetatable(s, name);
}

template<class T>
static T* luaAI_getlightuserdata(lua_State *s, const char *name) {
	lua_getglobal(s, name);
	if (lua_isnil(s, -1)) {
		return nullptr;
	}
	T* data = (T*) lua_touserdata(s, -1);
	lua_pop(s, 1);
	return data;
}

static luaAI_AI* luaAI_toai(lua_State *s, int n) {
	luaAI_AI* ai = luaAI_getudata<luaAI_AI*>(s, n, luaAI_metaai());
	if (!ai->ai) {
		luaL_error(s, "AI is already destroyed");
	}
	return ai;
}

static luaAI_ICharacter* luaAI_tocharacter(lua_State *s, int n) {
	luaAI_ICharacter* chr = luaAI_getudata<luaAI_ICharacter*>(s, n, luaAI_metacharacter());
	if (!chr->character) {
		luaL_error(s, "ICharacter is already destroyed");
	}
	return chr;
}

static Zone* luaAI_tozone(lua_State *s, int n) {
	return *(Zone**)luaAI_getudata<Zone*>(s, n, luaAI_metazone());
}

static AggroMgr* luaAI_toaggromgr(lua_State *s, int n) {
	return *(AggroMgr**)luaAI_getudata<AggroMgr*>(s, n, luaAI_metaaggromgr());
}

static GroupMgr* luaAI_togroupmgr(lua_State *s, int n) {
	return *(GroupMgr**)luaAI_getudata<GroupMgr*>(s, n, luaAI_metagroupmgr());
}

static glm::vec3* luaAI_tovec(lua_State *s, int n) {
	return luaAI_getudata<glm::vec3*>(s, n, luaAI_metavec());
}

static int luaAI_pushzone(lua_State* s, Zone* zone) {
	if (zone == nullptr) {
		lua_pushnil(s);
		return 1;
	}
	return luaAI_pushudata<Zone*>(s, zone, luaAI_metazone());
}

static int luaAI_pushaggromgr(lua_State* s, AggroMgr* aggroMgr) {
	return luaAI_pushudata<AggroMgr*>(s, aggroMgr, luaAI_metaaggromgr());
}

static int luaAI_pushgroupmgr(lua_State* s, GroupMgr* groupMgr) {
	return luaAI_pushudata<GroupMgr*>(s, groupMgr, luaAI_metagroupmgr());
}

static int luaAI_pushcharacter(lua_State* s, const ICharacterPtr& character) {
	luaAI_ICharacter* raw = (luaAI_ICharacter*) lua_newuserdata(s, sizeof(luaAI_ICharacter));
	luaAI_ICharacter* udata = new (raw)luaAI_ICharacter();
	udata->character = character;
	return luaAI_assignmetatable(s, luaAI_metacharacter());
}

static int luaAI_pushai(lua_State* s, const AIPtr& ai) {
	luaAI_AI* raw = (luaAI_AI*) lua_newuserdata(s, sizeof(luaAI_AI));
	luaAI_AI* udata = new (raw)luaAI_AI();
	udata->ai = ai;
	return luaAI_assignmetatable(s, luaAI_metaai());
}

static int luaAI_pushvec(lua_State* s, const glm::vec3& v) {
	return luaAI_pushudata<glm::vec3>(s, v, luaAI_metavec());
}

/***
 * Get the position of the group (average)
 * @tparam integer groupId
 * @treturn vec The average position of the group
 * @function groupMgr:position
 */
static int luaAI_groupmgrposition(lua_State* s) {
	const GroupMgr* groupMgr = luaAI_togroupmgr(s, 1);
	const GroupId groupId = (GroupId)luaL_checkinteger(s, 2);
	return luaAI_pushvec(s, groupMgr->getPosition(groupId));
}

/***
 * Add an entity from the group
 * @tparam integer groupId
 * @tparam ai ai
 * @treturn boolean Boolean to indicate whether the add was successful
 * @function groupMgr:add
 */
static int luaAI_groupmgradd(lua_State* s) {
	GroupMgr* groupMgr = luaAI_togroupmgr(s, 1);
	const GroupId groupId = (GroupId)luaL_checkinteger(s, 2);
	luaAI_AI* ai = luaAI_toai(s, 3);
	const bool state = groupMgr->add(groupId, ai->ai);
	lua_pushboolean(s, state);
	return 1;
}

/***
 * Remove an entity from the group
 * @tparam integer groupId
 * @tparam ai ai
 * @treturn boolean Boolean to indicate whether the removal was successful
 * @function groupMgr:remove
 */
static int luaAI_groupmgrremove(lua_State* s) {
	GroupMgr* groupMgr = luaAI_togroupmgr(s, 1);
	const GroupId groupId = (GroupId)luaL_checkinteger(s, 2);
	luaAI_AI* ai = luaAI_toai(s, 3);
	const bool state = groupMgr->remove(groupId, ai->ai);
	lua_pushboolean(s, state);
	return 1;
}

/***
 * Checks whether a give ai is the group leader of a particular group
 * @tparam integer groupId
 * @tparam ai ai
 * @treturn boolean Boolean to indicate whether the given AI is the group leader of the given group
 * @function groupMgr:isLeader
 */
static int luaAI_groupmgrisleader(lua_State* s) {
	const GroupMgr* groupMgr = luaAI_togroupmgr(s, 1);
	const GroupId groupId = (GroupId)luaL_checkinteger(s, 2);
	luaAI_AI* ai = luaAI_toai(s, 3);
	const bool state = groupMgr->isGroupLeader(groupId, ai->ai);
	lua_pushboolean(s, state);
	return 1;
}

/***
 * Checks whether a give ai is part of the given group
 * @tparam integer groupId
 * @tparam ai ai
 * @treturn boolean Boolean to indicate whether the given AI is part of the given group
 * @function groupMgr:isInGroup
 */
static int luaAI_groupmgrisingroup(lua_State* s) {
	const GroupMgr* groupMgr = luaAI_togroupmgr(s, 1);
	const GroupId groupId = (GroupId)luaL_checkinteger(s, 2);
	luaAI_AI* ai = luaAI_toai(s, 3);
	const bool state = groupMgr->isInGroup(groupId, ai->ai);
	lua_pushboolean(s, state);
	return 1;
}

/***
 * Checks whether a give ai is part of any group
 * @tparam ai ai
 * @treturn boolean Boolean to indicate whether the given AI is part of any group
 * @function groupMgr:isInAnyGroup
 */
static int luaAI_groupmgrisinanygroup(lua_State* s) {
	const GroupMgr* groupMgr = luaAI_togroupmgr(s, 1);
	luaAI_AI* ai = luaAI_toai(s, 2);
	const bool state = groupMgr->isInAnyGroup(ai->ai);
	lua_pushboolean(s, state);
	return 1;
}

/***
 * Get the group size
 * @tparam integer groupId
 * @treturn integer Size of the group (members)
 * @function groupMgr:size
 */
static int luaAI_groupmgrsize(lua_State* s) {
	const GroupMgr* groupMgr = luaAI_togroupmgr(s, 1);
	const GroupId groupId = (GroupId)luaL_checkinteger(s, 2);
	lua_pushinteger(s, groupMgr->getGroupSize(groupId));
	return 1;
}

/***
 * Get the group leader ai of a given group
 * @tparam integer groupId
 * @treturn ai AI of the group leader, or nil, if there is no such group
 * @function groupMgr:leader
 */
static int luaAI_groupmgrleader(lua_State* s) {
	const GroupMgr* groupMgr = luaAI_togroupmgr(s, 1);
	const GroupId groupId = (GroupId)luaL_checkinteger(s, 2);
	const AIPtr& ai = groupMgr->getLeader(groupId);
	if (!ai) {
		lua_pushnil(s);
	} else {
		luaAI_pushai(s, ai);
	}
	return 1;
}

static int luaAI_groupmgrtostring(lua_State* s) {
	const GroupMgr* groupMgr = luaAI_togroupmgr(s, 1);
	lua_pushfstring(s, "groupmgr: %p", groupMgr);
	return 1;
}

/***
 * Execute a function for all entities of the zone
 * @tparam function The function to execute
 * @function zone:execute
 */
static int luaAI_zoneexecute(lua_State* s) {
	Zone* zone = luaAI_tozone(s, 1);
	luaL_checktype(s, 2, LUA_TFUNCTION);
	const int topIndex = lua_gettop(s);
	zone->execute([=] (const AIPtr& ai) {
		if (luaAI_pushai(s, ai) <= 0) {
			return;
		}
		lua_pcall(s, 1, 0, 0);
		const int stackDelta = lua_gettop(s) - topIndex;
		if (stackDelta > 0) {
			lua_pop(s, stackDelta);
		}
	});
	return 0;
}

/***
 * Get the group manager of the zone
 * @treturn groupMgr The groupMgr of the zone
 * @function zone:groupMgr
 */
static int luaAI_zonegroupmgr(lua_State* s) {
	Zone* zone = luaAI_tozone(s, 1);
	return luaAI_pushgroupmgr(s, &zone->getGroupMgr());
}

static int luaAI_zonetostring(lua_State* s) {
	const Zone* zone = luaAI_tozone(s, 1);
	lua_pushfstring(s, "zone: %s", zone->getName().c_str());
	return 1;
}

/***
 * Get the name of the zone
 * @treturn string The name of the zone
 * @function zone:name
 */
static int luaAI_zonename(lua_State* s) {
	const Zone* zone = luaAI_tozone(s, 1);
	lua_pushstring(s, zone->getName().c_str());
	return 1;
}

/***
 * Get the ai instance for some character id in the zone
 * @tparam integer id The character id to get the AI for
 * @treturn ai AI instance for some character id in the zone, or nil if no such character
 * was found in the zone
 * @function zone:ai
 */
static int luaAI_zoneai(lua_State* s) {
	Zone* zone = luaAI_tozone(s, 1);
	const CharacterId id = (CharacterId)luaL_checkinteger(s, 2);
	const AIPtr& ai = zone->getAI(id);
	if (!ai) {
		lua_pushnil(s);
	} else {
		luaAI_pushai(s, ai);
	}
	return 1;
}

/***
 * Get the size of the zone - as in: How many entities are in that particular zone
 * @treturn integer The amount of entities in the zone
 * @function zone:size
 */
static int luaAI_zonesize(lua_State* s) {
	const Zone* zone = luaAI_tozone(s, 1);
	lua_pushinteger(s, zone->size());
	return 1;
}

/***
 * Get the highest aggro entry
 * @treturn integer The current highest aggro entry character id or nil
 * @treturn number The current highest aggro entry value or nil.
 * @function aggroMgr:highestEntry
 */
static int luaAI_aggromgrhighestentry(lua_State* s) {
	AggroMgr* aggroMgr = luaAI_toaggromgr(s, 1);
	EntryPtr entry = aggroMgr->getHighestEntry();
	if (entry == nullptr) {
		lua_pushnil(s);
		lua_pushnil(s);
	} else {
		lua_pushinteger(s, entry->getCharacterId());
		lua_pushnumber(s, entry->getAggro());
	}
	return 2;
}

/***
 * Return the current aggro manager entries
 * @treturn {integer, number, ...} Table with character id and aggro value
 * @function aggroMgr:entries
 */
static int luaAI_aggromgrentries(lua_State* s) {
	AggroMgr* aggroMgr = luaAI_toaggromgr(s, 1);
	const AggroMgr::Entries& entries = aggroMgr->getEntries();
	lua_newtable(s);
	const int top = lua_gettop(s);
	for (auto it = entries.begin(); it != entries.end(); ++it) {
		lua_pushinteger(s, it->getCharacterId());
		lua_pushnumber(s, it->getAggro());
		lua_settable(s, top);
	}
	return 1;
}

/***
 * Set the reduction type to a ratio-per-second style
 * @tparam number reduceRatioSecond
 * @tparam number minAggro
 * @function aggroMgr:reduceByRatio
 */
static int luaAI_aggromgrsetreducebyratio(lua_State* s) {
	AggroMgr* aggroMgr = luaAI_toaggromgr(s, 1);
	const lua_Number reduceRatioSecond = luaL_checknumber(s, 2);
	const lua_Number minAggro = luaL_checknumber(s, 3);
	aggroMgr->setReduceByRatio((float)reduceRatioSecond, (float)minAggro);
	return 0;
}

/***
 * Set the reduction type to a value-by-second style
 * @tparam number reduceValueSecond
 * @function aggroMgr:reduceByValue
 */
static int luaAI_aggromgrsetreducebyvalue(lua_State* s) {
	AggroMgr* aggroMgr = luaAI_toaggromgr(s, 1);
	const lua_Number reduceValueSecond = luaL_checknumber(s, 2);
	aggroMgr->setReduceByValue((float)reduceValueSecond);
	return 0;
}

/***
 * Reset the reduction values of the aggro manager
 * @function aggroMgr:resetReduceValue
 */
static int luaAI_aggromgrresetreducevalue(lua_State* s) {
	AggroMgr* aggroMgr = luaAI_toaggromgr(s, 1);
	aggroMgr->resetReduceValue();
	return 0;
}

/***
 * Apply aggro on some other character
 * @tparam integer id The character id to get aggro on
 * @tparam number amount The amount of aggro to apply
 * @treturn number The amount of aggro you have on the given entity
 * @function aggroMgr:addAggro
 */
static int luaAI_aggromgraddaggro(lua_State* s) {
	AggroMgr* aggroMgr = luaAI_toaggromgr(s, 1);
	const CharacterId chrId = (CharacterId)luaL_checkinteger(s, 2);
	const lua_Number amount = luaL_checknumber(s, 3);
	const EntryPtr& entry = aggroMgr->addAggro(chrId, (float)amount);
	lua_pushnumber(s, entry->getAggro());
	return 1;
}

static int luaAI_aggromgrtostring(lua_State* s) {
	lua_pushliteral(s, "aggroMgr");
	return 1;
}

/***
 * The character id
 * @treturn integer The id of a character
 * @function character:id
 */
static int luaAI_characterid(lua_State* s) {
	const luaAI_ICharacter* chr = luaAI_tocharacter(s, 1);
	lua_pushinteger(s, chr->character->getId());
	return 1;
}

/***
 * Get the position of the character
 * @treturn vec position of the character
 * @function character:position
 */
static int luaAI_characterposition(lua_State* s) {
	const luaAI_ICharacter* chr = luaAI_tocharacter(s, 1);
	return luaAI_pushvec(s, chr->character->getPosition());
}

/***
 * Set the position of the character
 * @tparam vec position
 * @function character:setPosition
 */
static int luaAI_charactersetposition(lua_State* s) {
	luaAI_ICharacter* chr = luaAI_tocharacter(s, 1);
	const glm::vec3* v = luaAI_tovec(s, 2);
	chr->character->setPosition(*v);
	return 0;
}

/***
 * Get the speed of the character
 * @treturn number Speed value of the character
 * @function character:speed
 */
static int luaAI_characterspeed(lua_State* s) {
	const luaAI_ICharacter* chr = luaAI_tocharacter(s, 1);
	lua_pushnumber(s, chr->character->getSpeed());
	return 1;
}

/***
 * Get the orientation of the character
 * @treturn number orientation value of the character
 * @function character:orientation
 */
static int luaAI_characterorientation(lua_State* s) {
	const luaAI_ICharacter* chr = luaAI_tocharacter(s, 1);
	lua_pushnumber(s, chr->character->getOrientation());
	return 1;
}

/***
 * Set the speed for a character
 * @tparam number speed
 * @function character:setSpeed
 */
static int luaAI_charactersetspeed(lua_State* s) {
	luaAI_ICharacter* chr = luaAI_tocharacter(s, 1);
	const lua_Number value = luaL_checknumber(s, 2);
	chr->character->setSpeed((float)value);
	return 0;
}

/***
 * Set the orientation for a character
 * @tparam number orientation
 * @function character:setOrientation
 */
static int luaAI_charactersetorientation(lua_State* s) {
	luaAI_ICharacter* chr = luaAI_tocharacter(s, 1);
	const lua_Number value = luaL_checknumber(s, 2);
	chr->character->setOrientation((float)value);
	return 0;
}

/***
 * Equal check for a character
 * @treturn boolean
 * @function character:__eq
 */
static int luaAI_charactereq(lua_State* s) {
	const luaAI_ICharacter* a = luaAI_tocharacter(s, 1);
	const luaAI_ICharacter* b = luaAI_tocharacter(s, 2);
	const bool e = *a->character == *b->character;
	lua_pushboolean(s, e);
	return 1;
}

/***
 * Garbage collector callback for a character
 * @function character:__gc
 */
static int luaAI_charactergc(lua_State* s) {
	luaAI_ICharacter* chr = luaAI_tocharacter(s, -1);
	chr->character = ICharacterPtr();
	return 0;
}

/***
 * Pushes the table of character (debugger) attributes onto the stack
 * @treturn {string, string, ....} Table with the key/value pair of the character attributes
 * @function character:attributes
 */
static int luaAI_characterattributes(lua_State* s) {
	const luaAI_ICharacter* chr = luaAI_tocharacter(s, 1);
	const CharacterAttributes& attributes = chr->character->getAttributes();
	lua_newtable(s);
	const int top = lua_gettop(s);
	for (auto it = attributes.begin(); it != attributes.end(); ++it) {
		const std::string& key = it->first;
		const std::string& value = it->second;
		lua_pushlstring(s, key.c_str(), key.size());
		lua_pushlstring(s, value.c_str(), value.size());
		lua_settable(s, top);
	}
	return 1;
}

/***
 * Set a debugger attribute to the character
 * @tparam string key The key of the attribute
 * @tparam string value The value of the attribute
 * @function character:setAttribute
 */
static int luaAI_charactersetattribute(lua_State* s) {
	luaAI_ICharacter* chr = luaAI_tocharacter(s, 1);
	const char* key = luaL_checkstring(s, 2);
	const char* value = luaL_checkstring(s, 3);
	chr->character->setAttribute(key, value);
	return 0;
}

static int luaAI_charactertostring(lua_State* s) {
	luaAI_ICharacter* character = luaAI_tocharacter(s, 1);
	lua_pushfstring(s, "Character: %d", (lua_Integer)character->character->getId());
	return 1;
}

/***
 * Get the character id
 * @treturn integer Integer with the character id
 * @function ai:id
 */
static int luaAI_aiid(lua_State* s) {
	const luaAI_AI* ai = luaAI_toai(s, 1);
	lua_pushinteger(s, ai->ai->getId());
	return 1;
}

/***
 * Get the active lifetime of the ai
 * @treturn integer Integer with the active lifetime of the ai
 * @function ai:time
 */
static int luaAI_aitime(lua_State* s) {
	const luaAI_AI* ai = luaAI_toai(s, 1);
	lua_pushinteger(s, ai->ai->getTime());
	return 1;
}

/***
 * Get the filtered entities of the ai
 * @treturn {integer, integer, ...} A table with key/value (index, characterid) pairs of the filtered entities
 * @function ai:filteredEntities
 */
static int luaAI_aifilteredentities(lua_State* s) {
	const luaAI_AI* ai = luaAI_toai(s, 1);
	const FilteredEntities& filteredEntities = ai->ai->getFilteredEntities();
	lua_newtable(s);
	const int top = lua_gettop(s);
	int i = 0;
	for (auto it = filteredEntities.begin(); it != filteredEntities.end(); ++it) {
		lua_pushinteger(s, ++i);
		lua_pushinteger(s, *it);
		lua_settable(s, top);
	}
	return 1;
}

/***
 * Get the zone of the ai
 * @treturn zone The zone where the ai is active in or nil
 * @function ai:zone
 */
static int luaAI_aigetzone(lua_State* s) {
	const luaAI_AI* ai = luaAI_toai(s, 1);
	return luaAI_pushzone(s, ai->ai->getZone());
}

/***
 * Get the aggro mgr of the ai
 * @treturn aggroMgr the aggro manager of the ai
 * @function ai:aggroMgr
 */
static int luaAI_aigetaggromgr(lua_State* s) {
	luaAI_AI* ai = luaAI_toai(s, 1);
	return luaAI_pushaggromgr(s, &ai->ai->getAggroMgr());
}

/***
 * Get the character of the ai
 * @treturn character the character of the ai
 * @function ai:character
 */
static int luaAI_aigetcharacter(lua_State* s) {
	const luaAI_AI* ai = luaAI_toai(s, 1);
	return luaAI_pushcharacter(s, ai->ai->getCharacter());
}

/***
 * Check whether the ai has a zone
 * @treturn boolean true if the ai has a zone, false otherwise
 * @function ai:hasZone
 */
static int luaAI_aihaszone(lua_State* s) {
	const luaAI_AI* ai = luaAI_toai(s, 1);
	lua_pushboolean(s, ai->ai->hasZone() ? 1 : 0);
	return 1;
}

/***
 * Equality for two ai instances
 * @treturn boolean true if the two given ai's are equal
 * @function ai:__eq
 */
static int luaAI_aieq(lua_State* s) {
	const luaAI_AI* a = luaAI_toai(s, 1);
	const luaAI_AI* b = luaAI_toai(s, 2);
	const bool e = a->ai->getId() == b->ai->getId();
	lua_pushboolean(s, e);
	return 1;
}

/***
 * Garbage collector callback for ai instances
 * @function ai:__gc
 */
static int luaAI_aigc(lua_State* s) {
	luaAI_AI* ai = luaAI_toai(s, -1);
	ai->ai = AIPtr();
	return 0;
}

static int luaAI_aitostring(lua_State* s) {
	const luaAI_AI* ai = luaAI_toai(s, 1);
	TreeNodePtr treeNode = ai->ai->getBehaviour();
	if (treeNode) {
		lua_pushfstring(s, "ai: %s", treeNode->getName().c_str());
	} else {
		lua_pushstring(s, "ai: no behaviour tree set");
	}
	return 1;
}

/***
 * Vector addition
 * @tparam vec a
 * @tparam vec b
 * @treturn vec the sum of a + b
 * @function vec:__add
 */
static int luaAI_vecadd(lua_State* s) {
	const glm::vec3* a = luaAI_tovec(s, 1);
	const glm::vec3* b = luaAI_tovec(s, 2);
	const glm::vec3& c = *a + *b;
	return luaAI_pushvec(s, c);
}

/***
 * Vector dot product also as vec:__mul
 * @tparam vec a
 * @tparam vec b
 * @treturn number The dot product of a and b
 * @function vec:dot
 */
static int luaAI_vecdot(lua_State* s) {
	const glm::vec3* a = luaAI_tovec(s, 1);
	const glm::vec3* b = luaAI_tovec(s, 2);
	const float c = glm::dot(*a, *b);
	lua_pushnumber(s, c);
	return 1;
}

/***
 * Vector div function
 * @tparam vec a
 * @tparam vec b
 * @treturn vec a / b
 * @function vec:__div
 */
static int luaAI_vecdiv(lua_State* s) {
	const glm::vec3* a = luaAI_tovec(s, 1);
	const glm::vec3* b = luaAI_tovec(s, 2);
	const glm::vec3& c = *a / *b;
	luaAI_pushvec(s, c);
	return 1;
}

static int luaAI_veclen(lua_State* s) {
	const glm::vec3* a = luaAI_tovec(s, 1);
	const float c = glm::length(*a);
	lua_pushnumber(s, c);
	return 1;
}

static int luaAI_veceq(lua_State* s) {
	const glm::vec3* a = luaAI_tovec(s, 1);
	const glm::vec3* b = luaAI_tovec(s, 2);
	const bool e = glm::all(glm::epsilonEqual(*a, *b, 0.0001f));
	lua_pushboolean(s, e);
	return 1;
}

/***
 * Vector subtraction
 * @tparam vec a
 * @tparam vec b
 * @treturn vec the result of a - b
 * @function vec:__sub
 */
static int luaAI_vecsub(lua_State* s) {
	const glm::vec3* a = luaAI_tovec(s, 1);
	const glm::vec3* b = luaAI_tovec(s, 2);
	const glm::vec3& c = *a - *b;
	luaAI_pushvec(s, c);
	return 1;
}

/***
 * Negates a given vector
 * @tparam vec a
 * @treturn vec The result of -a
 * @function vec:__unm
 */
static int luaAI_vecnegate(lua_State* s) {
	const glm::vec3* a = luaAI_tovec(s, 1);
	luaAI_pushvec(s, -(*a));
	return 1;
}

static int luaAI_vectostring(lua_State* s) {
	const glm::vec3* a = luaAI_tovec(s, 1);
	lua_pushfstring(s, "vec: %f:%f:%f", a->x, a->y, a->z);
	return 1;
}

static int luaAI_vecindex(lua_State *s) {
	const glm::vec3* v = luaAI_tovec(s, 1);
	const char* i = luaL_checkstring(s, 2);

	switch (*i) {
	case '0':
	case 'x':
	case 'r':
		lua_pushnumber(s, v->x);
		break;
	case '1':
	case 'y':
	case 'g':
		lua_pushnumber(s, v->y);
		break;
	case '2':
	case 'z':
	case 'b':
		lua_pushnumber(s, v->z);
		break;
	default:
		lua_pushnil(s);
		break;
	}

	return 1;
}

static int luaAI_vecnewindex(lua_State *s) {
	glm::vec3* v = luaAI_tovec(s, 1);
	const char *i = luaL_checkstring(s, 2);
	const lua_Number t = luaL_checknumber(s, 3);

	switch (*i) {
	case '0':
	case 'x':
	case 'r':
		v->x = (float)t;
		break;
	case '1':
	case 'y':
	case 'g':
		v->y = (float)t;
		break;
	case '2':
	case 'z':
	case 'b':
		v->z = (float)t;
		break;
	default:
		break;
	}

	return 1;
}

}
