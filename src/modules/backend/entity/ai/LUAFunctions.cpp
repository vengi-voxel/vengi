/***
 * @file
 * @ingroup LUA
 */

#include "LUAFunctions.h"
#include "backend/entity/ai/group/GroupId.h"
#include "backend/entity/ai/group/GroupMgr.h"
#include "backend/entity/ai/zone/Zone.h"
#include "backend/entity/ai/aggro/AggroMgr.h"
#include "backend/entity/ai/common/Math.h"
#include "backend/entity/ai/tree/TreeNode.h"
#include "commonlua/LUAFunctions.h"
#include "lua.h"

namespace backend {

struct luaAI_AI {
	AIPtr ai;
};

struct luaAI_ICharacter {
	ICharacterPtr character;
};

const char *luaAI_metaai() {
	return "__meta_ai";
}

static inline const char *luaAI_metazone() {
	return "__meta_zone";
}

static inline const char* luaAI_metaaggromgr() {
	return "__meta_aggromgr";
}

const char* luaAI_metaregistry() {
	return "__meta_registry";
}

static inline const char* luaAI_metagroupmgr() {
	return "__meta_groupmgr";
}

const char* luaAI_metacharacter() {
	return "__meta_character";
}

static luaAI_AI* luaAI_toai(lua_State *s, int n) {
	luaAI_AI* ai = clua_getudata<luaAI_AI*>(s, n, luaAI_metaai());
	return ai;
}

static luaAI_ICharacter* luaAI_tocharacter(lua_State *s, int n) {
	luaAI_ICharacter* chr = clua_getudata<luaAI_ICharacter*>(s, n, luaAI_metacharacter());
	return chr;
}

static Zone* luaAI_tozone(lua_State *s, int n) {
	return *(Zone**)clua_getudata<Zone*>(s, n, luaAI_metazone());
}

static AggroMgr* luaAI_toaggromgr(lua_State *s, int n) {
	return *(AggroMgr**)clua_getudata<AggroMgr*>(s, n, luaAI_metaaggromgr());
}

static GroupMgr* luaAI_togroupmgr(lua_State *s, int n) {
	return *(GroupMgr**)clua_getudata<GroupMgr*>(s, n, luaAI_metagroupmgr());
}

static int luaAI_pushzone(lua_State* s, Zone* zone) {
	if (zone == nullptr) {
		lua_pushnil(s);
		return 1;
	}
	return clua_pushudata<Zone*>(s, zone, luaAI_metazone());
}

static int luaAI_pushaggromgr(lua_State* s, AggroMgr* aggroMgr) {
	return clua_pushudata<AggroMgr*>(s, aggroMgr, luaAI_metaaggromgr());
}

static int luaAI_pushgroupmgr(lua_State* s, GroupMgr* groupMgr) {
	return clua_pushudata<GroupMgr*>(s, groupMgr, luaAI_metagroupmgr());
}

static int luaAI_pushcharacter(lua_State* s, const ICharacterPtr& character) {
	luaAI_ICharacter* raw = (luaAI_ICharacter*) lua_newuserdata(s, sizeof(luaAI_ICharacter));
	luaAI_ICharacter* udata = new (raw)luaAI_ICharacter();
	udata->character = character;
	return clua_assignmetatable(s, luaAI_metacharacter());
}

int luaAI_pushai(lua_State* s, const AIPtr& ai) {
	luaAI_AI* raw = (luaAI_AI*) lua_newuserdata(s, sizeof(luaAI_AI));
	luaAI_AI* udata = new (raw)luaAI_AI();
	udata->ai = ai;
	return clua_assignmetatable(s, luaAI_metaai());
}

/***
 * Get the position of the group (average)
 * @par integer groupId
 * @treturn vec The average position of the group
 * @function groupMgr:position
 */
static int luaAI_groupmgrposition(lua_State* s) {
	const GroupMgr* groupMgr = luaAI_togroupmgr(s, 1);
	const GroupId groupId = (GroupId)luaL_checkinteger(s, 2);
	glm::vec3 target;
	if (groupMgr->getPosition(groupId, target)) {
		return clua_push(s, target);
	}
	lua_pushnil(s);
	return 1;
}

/***
 * Add an entity from the group
 * @par integer groupId
 * @par ai ai
 * @treturn boolean Boolean to indicate whether the add was successful
 * @function groupMgr:add
 */
static int luaAI_groupmgradd(lua_State* s) {
	GroupMgr* groupMgr = luaAI_togroupmgr(s, 1);
	const GroupId groupId = (GroupId)luaL_checkinteger(s, 2);
	luaAI_AI* ai = luaAI_toai(s, 3);
	if (!ai->ai) {
		return clua_error(s, "AI is already destroyed");
	}
	const bool state = groupMgr->add(groupId, ai->ai);
	lua_pushboolean(s, state);
	return 1;
}

/***
 * Remove an entity from the group
 * @par integer groupId
 * @par ai ai
 * @treturn boolean Boolean to indicate whether the removal was successful
 * @function groupMgr:remove
 */
static int luaAI_groupmgrremove(lua_State* s) {
	GroupMgr* groupMgr = luaAI_togroupmgr(s, 1);
	const GroupId groupId = (GroupId)luaL_checkinteger(s, 2);
	luaAI_AI* ai = luaAI_toai(s, 3);
	if (!ai->ai) {
		return clua_error(s, "AI is already destroyed");
	}
	const bool state = groupMgr->remove(groupId, ai->ai);
	lua_pushboolean(s, state);
	return 1;
}

/***
 * Checks whether a give ai is the group leader of a particular group
 * @par integer groupId
 * @par ai ai
 * @treturn boolean Boolean to indicate whether the given AI is the group leader of the given group
 * @function groupMgr:isLeader
 */
static int luaAI_groupmgrisleader(lua_State* s) {
	const GroupMgr* groupMgr = luaAI_togroupmgr(s, 1);
	const GroupId groupId = (GroupId)luaL_checkinteger(s, 2);
	luaAI_AI* ai = luaAI_toai(s, 3);
	if (!ai->ai) {
		return clua_error(s, "AI is already destroyed");
	}
	const bool state = groupMgr->isGroupLeader(groupId, ai->ai);
	lua_pushboolean(s, state);
	return 1;
}

/***
 * Checks whether a give ai is part of the given group
 * @par integer groupId
 * @par ai ai
 * @treturn boolean Boolean to indicate whether the given AI is part of the given group
 * @function groupMgr:isInGroup
 */
static int luaAI_groupmgrisingroup(lua_State* s) {
	const GroupMgr* groupMgr = luaAI_togroupmgr(s, 1);
	const GroupId groupId = (GroupId)luaL_checkinteger(s, 2);
	luaAI_AI* ai = luaAI_toai(s, 3);
	if (!ai->ai) {
		return clua_error(s, "AI is already destroyed");
	}
	const bool state = groupMgr->isInGroup(groupId, ai->ai);
	lua_pushboolean(s, state);
	return 1;
}

/***
 * Checks whether a give ai is part of any group
 * @par ai ai
 * @treturn boolean Boolean to indicate whether the given AI is part of any group
 * @function groupMgr:isInAnyGroup
 */
static int luaAI_groupmgrisinanygroup(lua_State* s) {
	const GroupMgr* groupMgr = luaAI_togroupmgr(s, 1);
	luaAI_AI* ai = luaAI_toai(s, 2);
	if (!ai->ai) {
		return clua_error(s, "AI is already destroyed");
	}
	const bool state = groupMgr->isInAnyGroup(ai->ai);
	lua_pushboolean(s, state);
	return 1;
}

/***
 * Get the group size
 * @par integer groupId
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
 * @par integer groupId
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
 * @par function The function to execute
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
 * @par integer id The character id to get the AI for
 * @treturn ai AI instance for some character id in the zone, or nil if no such character
 * was found in the zone
 * @function zone:ai
 */
static int luaAI_zoneai(lua_State* s) {
	Zone* zone = luaAI_tozone(s, 1);
	const ai::CharacterId id = (ai::CharacterId)luaL_checkinteger(s, 2);
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
 * @par number reduceRatioSecond
 * @par number minAggro
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
 * @par number reduceValueSecond
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
 * @par integer id The character id to get aggro on
 * @par number amount The amount of aggro to apply
 * @treturn number The amount of aggro you have on the given entity
 * @function aggroMgr:addAggro
 */
static int luaAI_aggromgraddaggro(lua_State* s) {
	AggroMgr* aggroMgr = luaAI_toaggromgr(s, 1);
	const ai::CharacterId chrId = (ai::CharacterId)luaL_checkinteger(s, 2);
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
	if (!chr->character) {
		return clua_error(s, "ICharacter is already destroyed");
	}
	return clua_push(s, chr->character->getPosition());
}

/***
 * Set the position of the character
 * @par vec position
 * @function character:setPosition
 */
static int luaAI_charactersetposition(lua_State* s) {
	luaAI_ICharacter* chr = luaAI_tocharacter(s, 1);
	if (!chr->character) {
		return clua_error(s, "ICharacter is already destroyed");
	}
	const glm::vec3* v = clua_get<glm::vec3>(s, 2);
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
	if (!chr->character) {
		return clua_error(s, "ICharacter is already destroyed");
	}
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
	if (!chr->character) {
		return clua_error(s, "ICharacter is already destroyed");
	}
	lua_pushnumber(s, chr->character->getOrientation());
	return 1;
}

/***
 * Set the speed for a character
 * @par number speed
 * @function character:setSpeed
 */
static int luaAI_charactersetspeed(lua_State* s) {
	luaAI_ICharacter* chr = luaAI_tocharacter(s, 1);
	if (!chr->character) {
		return clua_error(s, "ICharacter is already destroyed");
	}
	const lua_Number value = luaL_checknumber(s, 2);
	chr->character->setSpeed((float)value);
	return 0;
}

/***
 * Set the orientation for a character
 * @par number orientation
 * @function character:setOrientation
 */
static int luaAI_charactersetorientation(lua_State* s) {
	luaAI_ICharacter* chr = luaAI_tocharacter(s, 1);
	if (!chr->character) {
		return clua_error(s, "ICharacter is already destroyed");
	}
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
	if (!a->character) {
		return clua_error(s, "ICharacter is already destroyed");
	}
	const luaAI_ICharacter* b = luaAI_tocharacter(s, 2);
	if (!b->character) {
		return clua_error(s, "ICharacter is already destroyed");
	}
	const bool e = a->character == b->character;
	lua_pushboolean(s, e);
	return 1;
}

/***
 * Garbage collector callback for a character
 * @function character:__gc
 */
static int luaAI_charactergc(lua_State* s) {
	luaAI_ICharacter* chr = luaAI_tocharacter(s, -1);
	if (!chr->character) {
		return clua_error(s, "ICharacter is already destroyed");
	}
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
	if (!chr->character) {
		return clua_error(s, "ICharacter is already destroyed");
	}
	const ai::CharacterAttributes& attributes = chr->character->getAttributes();
	lua_newtable(s);
	const int top = lua_gettop(s);
	for (auto it = attributes.begin(); it != attributes.end(); ++it) {
		const core::String& key = it->first;
		const core::String& value = it->second;
		lua_pushlstring(s, key.c_str(), key.size());
		lua_pushlstring(s, value.c_str(), value.size());
		lua_settable(s, top);
	}
	return 1;
}

/***
 * Set a debugger attribute to the character
 * @par string key The key of the attribute
 * @par string value The value of the attribute
 * @function character:setAttribute
 */
static int luaAI_charactersetattribute(lua_State* s) {
	luaAI_ICharacter* chr = luaAI_tocharacter(s, 1);
	if (!chr->character) {
		return clua_error(s, "ICharacter is already destroyed");
	}
	const char* key = luaL_checkstring(s, 2);
	const char* value = luaL_checkstring(s, 3);
	chr->character->setAttribute(key, value);
	return 0;
}

static int luaAI_charactertostring(lua_State* s) {
	luaAI_ICharacter* chr = luaAI_tocharacter(s, 1);
	if (!chr->character) {
		return clua_error(s, "ICharacter is already destroyed");
	}
	lua_pushfstring(s, "Character: %d", (lua_Integer)chr->character->getId());
	return 1;
}

/***
 * Get the character id
 * @treturn integer Integer with the character id
 * @function ai:id
 */
static int luaAI_aiid(lua_State* s) {
	const luaAI_AI* ai = luaAI_toai(s, 1);
	if (!ai->ai) {
		return clua_error(s, "AI is already destroyed");
	}
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
	if (!ai->ai) {
		return clua_error(s, "AI is already destroyed");
	}
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
	if (!ai->ai) {
		return clua_error(s, "AI is already destroyed");
	}
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
	if (!ai->ai) {
		return clua_error(s, "AI is already destroyed");
	}
	return luaAI_pushzone(s, ai->ai->getZone());
}

/***
 * Get the aggro mgr of the ai
 * @treturn aggroMgr the aggro manager of the ai
 * @function ai:aggroMgr
 */
static int luaAI_aigetaggromgr(lua_State* s) {
	luaAI_AI* ai = luaAI_toai(s, 1);
	if (!ai->ai) {
		return clua_error(s, "AI is already destroyed");
	}
	return luaAI_pushaggromgr(s, &ai->ai->getAggroMgr());
}

/***
 * Get the character of the ai
 * @treturn character the character of the ai
 * @function ai:character
 */
static int luaAI_aigetcharacter(lua_State* s) {
	const luaAI_AI* ai = luaAI_toai(s, 1);
	if (!ai->ai) {
		return clua_error(s, "AI is already destroyed");
	}
	return luaAI_pushcharacter(s, ai->ai->getCharacter());
}

/***
 * Check whether the ai has a zone
 * @treturn boolean true if the ai has a zone, false otherwise
 * @function ai:hasZone
 */
static int luaAI_aihaszone(lua_State* s) {
	const luaAI_AI* ai = luaAI_toai(s, 1);
	if (!ai->ai) {
		return clua_error(s, "AI is already destroyed");
	}
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
	if (!a->ai) {
		return clua_error(s, "AI is already destroyed");
	}
	const luaAI_AI* b = luaAI_toai(s, 2);
	if (!b->ai) {
		return clua_error(s, "AI is already destroyed");
	}
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
	if (!ai->ai) {
		return clua_error(s, "AI is already destroyed");
	}
	ai->ai = AIPtr();
	return 0;
}

static int luaAI_aitostring(lua_State* s) {
	const luaAI_AI* ai = luaAI_toai(s, 1);
	if (!ai->ai) {
		return clua_error(s, "AI is already destroyed");
	}
	TreeNodePtr treeNode = ai->ai->getBehaviour();
	if (treeNode) {
		lua_pushfstring(s, "ai: %s", treeNode->getName().c_str());
	} else {
		lua_pushstring(s, "ai: no behaviour tree set");
	}
	return 1;
}

static int luaAI_aisetfilteredentities(lua_State* s) {
	luaAI_AI* ai = luaAI_toai(s, 1);
	if (!ai->ai) {
		return clua_error(s, "AI is already destroyed");
	}
	luaL_checktype(s, 2, LUA_TTABLE);

	const int n = lua_rawlen(s, 2);
	FilteredEntities v(n);
	for (int i = 1; i <= n; ++i) {
		lua_rawgeti(s, 2, i);
		const int top = lua_gettop(s);
		const ai::CharacterId id = (ai::CharacterId)luaL_checknumber(s, top);
		v[i - 1] = id;
		lua_pop(s, 1);
	}
	ai->ai->setFilteredEntities(v);
	return 0;
}

static int luaAI_aiaddfilteredentity(lua_State* s) {
	luaAI_AI* ai = luaAI_toai(s, 1);
	if (!ai->ai) {
		return clua_error(s, "AI is already destroyed");
	}
	const ai::CharacterId id = (ai::CharacterId)luaL_checkinteger(s, 2);
	ai->ai->addFilteredEntity(id);
	return 0;
}

void luaAI_registerAll(lua_State* s) {
	static const luaL_Reg aiFuncs[] = {
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
	clua_registerfuncs(s, aiFuncs, luaAI_metaai());

	static const luaL_Reg zoneFuncs[] = {
		{"size", luaAI_zonesize},
		{"name", luaAI_zonename},
		{"ai", luaAI_zoneai},
		{"execute", luaAI_zoneexecute},
		{"groupMgr", luaAI_zonegroupmgr},
		{"__tostring", luaAI_zonetostring},
		{nullptr, nullptr}
	};
	clua_registerfuncs(s, zoneFuncs, luaAI_metazone());

	static const luaL_Reg characterFuncs[] = {
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
	clua_registerfuncs(s, characterFuncs, luaAI_metacharacter());

	static const luaL_Reg aggroMgrFuncs[] = {
		{"setReduceByRatio", luaAI_aggromgrsetreducebyratio},
		{"setReduceByValue", luaAI_aggromgrsetreducebyvalue},
		{"resetReduceValue", luaAI_aggromgrresetreducevalue},
		{"addAggro", luaAI_aggromgraddaggro},
		{"highestEntry", luaAI_aggromgrhighestentry},
		{"entries", luaAI_aggromgrentries},
		{"__tostring", luaAI_aggromgrtostring},
		{nullptr, nullptr}
	};
	clua_registerfuncs(s, aggroMgrFuncs, luaAI_metaaggromgr());

	static const luaL_Reg groupMgrFuncs[] = {
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
	clua_registerfuncs(s, groupMgrFuncs, luaAI_metagroupmgr());

	clua_mathregister(s);
}

}
