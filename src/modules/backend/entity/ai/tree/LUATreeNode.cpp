/**
 * @file
 */

#include "LUATreeNode.h"
#include "backend/entity/ai/LUAFunctions.h"

namespace backend {

ai::TreeNodeStatus LUATreeNode::runLUA(const AIPtr& entity, int64_t deltaMillis) {
	// get userdata of the behaviour tree node
	const core::String name = "__meta_node_" + _type;
	lua_getfield(_s, LUA_REGISTRYINDEX, name.c_str());
#if AI_LUA_SANTITY > 0
	if (lua_isnil(_s, -1)) {
		Log::error("LUA node: could not find lua userdata for %s", name.c_str());
		return ai::TreeNodeStatus::EXCEPTION;
	}
#endif
	// get metatable
	lua_getmetatable(_s, -1);
#if AI_LUA_SANTITY > 0
	if (!lua_istable(_s, -1)) {
		Log::error("LUA node: userdata for %s doesn't have a metatable assigned", name.c_str());
		return ai::TreeNodeStatus::EXCEPTION;
	}
#endif
	// get execute() method
	lua_getfield(_s, -1, "execute");
	if (!lua_isfunction(_s, -1)) {
		Log::error("LUA node: metatable for %s doesn't have the execute() function assigned", name.c_str());
		return ai::TreeNodeStatus::EXCEPTION;
	}

	// push self onto the stack
	lua_getfield(_s, LUA_REGISTRYINDEX, name.c_str());

	// first parameter is ai
	if (luaAI_pushai(_s, entity) == 0) {
		return ai::TreeNodeStatus::EXCEPTION;
	}

	// second parameter is dt
	lua_pushinteger(_s, deltaMillis);

#if AI_LUA_SANTITY > 0
	if (!lua_isfunction(_s, -4)) {
		Log::error("LUA node: expected to find a function on stack -4");
		return ai::TreeNodeStatus::EXCEPTION;
	}
	if (!lua_isuserdata(_s, -3)) {
		Log::error("LUA node: expected to find the userdata on -3");
		return ai::TreeNodeStatus::EXCEPTION;
	}
	if (!lua_isuserdata(_s, -2)) {
		Log::error("LUA node: second parameter should be the ai");
		return ai::TreeNodeStatus::EXCEPTION;
	}
	if (!lua_isinteger(_s, -1)) {
		Log::error("LUA node: first parameter should be the delta millis");
		return ai::TreeNodeStatus::EXCEPTION;
	}
#endif
	const int error = lua_pcall(_s, 3, 1, 0);
	if (error) {
		Log::error("LUA node script: %s", lua_isstring(_s, -1) ? lua_tostring(_s, -1) : "Unknown Error");
		// reset stack
		lua_pop(_s, lua_gettop(_s));
		return ai::TreeNodeStatus::EXCEPTION;
	}
	const lua_Integer execstate = luaL_checkinteger(_s, -1);
	if (execstate < 0 || execstate >= (lua_Integer)ai::TreeNodeStatus::MAX_TREENODESTATUS) {
		Log::error("LUA node: illegal tree node status returned: " LUA_INTEGER_FMT, execstate);
	}

	// reset stack
	lua_pop(_s, lua_gettop(_s));
	return (ai::TreeNodeStatus)execstate;
}

LUATreeNode::LUATreeNodeFactory::LUATreeNodeFactory(lua_State* s, const core::String& typeStr) :
		_s(s), _type(typeStr) {
}

TreeNodePtr LUATreeNode::LUATreeNodeFactory::create(const TreeNodeFactoryContext* ctx) const {
	return std::make_shared<LUATreeNode>(ctx->name, ctx->parameters, ctx->condition, _s, _type);
}

LUATreeNode::LUATreeNode(const core::String& name, const core::String& parameters, const ConditionPtr& condition, lua_State* s, const core::String& type) :
		TreeNode(name, parameters, condition), _s(s) {
	_type = type;
}

LUATreeNode::~LUATreeNode() {
}

ai::TreeNodeStatus LUATreeNode::execute(const AIPtr& entity, int64_t deltaMillis) {
	if (TreeNode::execute(entity, deltaMillis) == ai::TreeNodeStatus::CANNOTEXECUTE) {
		return ai::TreeNodeStatus::CANNOTEXECUTE;
	}

	return state(entity, runLUA(entity, deltaMillis));
}

}
