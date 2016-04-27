#pragma once

#include "LUATreeLoader.h"
#include "LUATree.h"
#include "LUANode.h"
#include "LUACondition.h"
#include <AIRegistry.h>
#include <conditions/ConditionParser.h>
#include <tree/TreeNodeParser.h>

namespace ai {

static LUATreeLoader* luaGetContext(lua_State * l) {
	return LUA::getGlobalData<LUATreeLoader>(l, "Loader");
}

static LUATree* luaGetTreeContext(lua_State * l, int n) {
	return LUA::getUserData<LUATree>(l, n, "Tree");
}

static LUANode* luaGetNodeContext(lua_State * l, int n) {
	return LUA::getUserData<LUANode>(l, n, "Node");
}

#if 0
static LUACondition* luaGetConditionContext(lua_State * l, int n) {
	return LUA::getUserData<LUACondition>(l, n, "Condition");
}
#endif

static int luaMain_CreateTree(lua_State * l) {
	LUATreeLoader *ctx = luaGetContext(l);
	const std::string name = luaL_checkstring(l, 1);
	LUATree** udata = LUA::newUserdata<LUATree>(l, "Tree");
	*udata = new LUATree(name, ctx);
	return 1;
}

static int luaTree_GC(lua_State * l) {
	LUATree *tree = luaGetTreeContext(l, 1);
	delete tree;
	return 0;
}

static int luaTree_ToString(lua_State * l) {
	const LUATree *tree = luaGetTreeContext(l, 1);
	lua_pushfstring(l, "tree: %s [%s]", tree->getName().c_str(), tree->getRoot() ? "root" : "no root");
	return 1;
}

static int luaTree_GetName(lua_State * l) {
	const LUATree *tree = luaGetTreeContext(l, 1);
	lua_pushstring(l, tree->getName().c_str());
	return 1;
}

static int luaNode_GC(lua_State * l) {
	LUANode *node = luaGetNodeContext(l, 1);
	delete node;
	return 0;
}

static int luaNode_ToString(lua_State * l) {
	const LUANode *node = luaGetNodeContext(l, 1);
	const LUACondition* condition = node->getCondition();
	lua_pushfstring(l, "node: %d children [condition: %s]", (int)node->getChildren().size(),
			condition ? condition->getCondition()->getName().c_str() : "no condition");
	return 1;
}

static int luaNode_GetName(lua_State * l) {
	const LUANode *node = luaGetNodeContext(l, 1);
	lua_pushstring(l, node->getTreeNode()->getName().c_str());
	return 1;
}

static int luaTree_CreateRoot(lua_State * l) {
	LUATree *ctx = luaGetTreeContext(l, 1);
	const std::string id = luaL_checkstring(l, 2);
	const std::string name = luaL_checkstring(l, 3);

	TreeNodeParser parser(ctx->getAIFactory(), id);
	const TreeNodePtr& node = parser.getTreeNode(name);
	if (!node) {
		LUA::returnError(l, "Could not create a node for " + id);
		return 0;
	}

	LUANode** udata = LUA::newUserdata<LUANode>(l, "Node");
	*udata = new LUANode(node, ctx);
	if (!ctx->setRoot(*udata)) {
		LUATreeLoader *loader = luaGetContext(l);
		LUA::returnError(l, loader->getError());
		return 0;
	}
	return 1;
}

static int luaNode_AddNode(lua_State * l) {
	LUANode *node = luaGetNodeContext(l, 1);
	const std::string id = luaL_checkstring(l, 2);
	const std::string name = luaL_checkstring(l, 3);
	LUANode** udata = LUA::newUserdata<LUANode>(l, "Node");

	TreeNodeFactoryContext factoryCtx(name, "", True::get());
	*udata = node->addChild(id, factoryCtx);
	if (*udata == nullptr) {
		LUA::returnError(l, "Could not create a node for " + id);
	}
	return 1;
}

static int luaNode_SetCondition(lua_State * l) {
	LUATreeLoader *ctx = luaGetContext(l);
	LUANode *node = luaGetNodeContext(l, 1);
	const std::string conditionExpression = luaL_checkstring(l, 2);
	LUACondition** udata = LUA::newUserdata<LUACondition>(l, "Condition");

	ConditionParser parser(ctx->getAIFactory(), conditionExpression);
	const ConditionPtr& condition = parser.getCondition();
	if (!condition) {
		LUA::returnError(l, "Could not create a condition for " + conditionExpression + ": " + parser.getError());
		return 0;
	}

	*udata = new LUACondition(condition);
	node->setCondition(*udata);
	return 1;
}

}
