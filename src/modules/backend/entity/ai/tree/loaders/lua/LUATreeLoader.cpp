/**
 * @file
 */

#include "LUATreeLoader.h"
#include "backend/entity/ai/tree/TreeNode.h"
#include "backend/entity/ai/AIRegistry.h"
#include "backend/entity/ai/condition/ConditionParser.h"
#include "backend/entity/ai/tree/TreeNodeParser.h"
#include "backend/entity/ai/condition/True.h"
#include "commonlua/LUA.h"
#include "commonlua/LUAFunctions.h"
#include "core/concurrent/Lock.h"

namespace backend {

class LUATreeWrapper;

class LUANodeWrapper {
private:
	TreeNodePtr _node;
	std::vector<LUANodeWrapper*> _children;
	LUATreeWrapper *_tree;
public:
	LUANodeWrapper(const TreeNodePtr& node, LUATreeWrapper *tree) :
			_node(node), _tree(tree) {
	}

	~LUANodeWrapper() {
	}

	inline const TreeNodePtr& getTreeNode() const {
		return _node;
	}

	inline void setCondition(const ConditionPtr& condition) {
		_node->setCondition(condition);
	}

	inline const std::vector<LUANodeWrapper*>& getChildren() const {
		return _children;
	}

	LUANodeWrapper* addChild(const IAIFactory& aiFactory, const core::String& nodeType, const TreeNodeFactoryContext& ctx) {
		TreeNodeParser parser(aiFactory, nodeType);
		const TreeNodePtr& child = parser.getTreeNode(ctx.name);
		if (!child) {
			return nullptr;
		}
		LUANodeWrapper *node = new LUANodeWrapper(child, _tree);
		_children.push_back(node);
		_node->addChild(child);
		return node;
	}
};

class LUATreeWrapper {
private:
	core::String _name;
	LUATreeLoader* _ctx;
public:
	LUATreeWrapper(const core::String& name, LUATreeLoader* ctx) :
			_name(name), _ctx(ctx) {
	}

	inline bool setRoot(const TreeNodePtr& root) {
		return _ctx->addTree(_name, root);
	}

	inline const core::String& getName() const {
		return _name;
	}
};

static const char *luaai_metatree() {
	return "__meta_tree";
}

static const char *luaai_metanode() {
	return "__meta_node";
}

static const char *luaai_metaai() {
	return "__global_ai";
}

static const char *luaai_metatreeloader() {
	return "__meta_loader";
}

static LUATreeLoader* luaai_gettreeloader(lua_State *s) {
	lua_getglobal(s, luaai_metatreeloader());
	LUATreeLoader *loader = (LUATreeLoader *)lua_touserdata(s, -1);
	lua_pop(s, 1);
	return loader;
}

static LUATreeWrapper* luaai_totree(lua_State* s, int n) {
	return *(LUATreeWrapper**)clua_getudata<LUATreeWrapper*>(s, n, luaai_metatree());
}

static LUANodeWrapper* luaai_tonode(lua_State* s, int n) {
	return *(LUANodeWrapper**)clua_getudata<LUANodeWrapper*>(s, n, luaai_metanode());
}

static int luaai_pushtree(lua_State* s, LUATreeWrapper *tree) {
	if (tree == nullptr) {
		lua_pushnil(s);
		return 1;
	}
	return clua_pushudata(s, tree, luaai_metatree());
}

static int luaai_pushnode(lua_State* s, LUANodeWrapper *node) {
	if (node == nullptr) {
		lua_pushnil(s);
		return 1;
	}

	return clua_pushudata(s, node, luaai_metanode());
}

static int luaai_createtree(lua_State* s) {
	LUATreeLoader *ctx = luaai_gettreeloader(s);
	const core::String name = luaL_checkstring(s, 1);
	return luaai_pushtree(s, new LUATreeWrapper(name, ctx));
}

static int luaai_tree_gc(lua_State * l) {
	LUATreeWrapper *tree = luaai_totree(l, 1);
	delete tree;
	return 0;
}

static int luaai_tree_tostring(lua_State * l) {
	const LUATreeWrapper *tree = luaai_totree(l, 1);
	lua_pushfstring(l, "tree: %s", tree->getName().c_str());
	return 1;
}

static int luaai_tree_getname(lua_State * l) {
	const LUATreeWrapper *tree = luaai_totree(l, 1);
	lua_pushstring(l, tree->getName().c_str());
	return 1;
}

static int luaai_node_gc(lua_State * l) {
	LUANodeWrapper *node = luaai_tonode(l, 1);
	delete node;
	return 0;
}

static int luaai_node_tostring(lua_State * l) {
	const LUANodeWrapper *node = luaai_tonode(l, 1);
	lua_pushfstring(l, "node: %d children", (int)node->getChildren().size());
	return 1;
}

static int luaai_node_getname(lua_State * l) {
	const LUANodeWrapper *node = luaai_tonode(l, 1);
	lua_pushstring(l, node->getTreeNode()->getName().c_str());
	return 1;
}

static int luaai_tree_createroot(lua_State * l) {
	LUATreeLoader *ctx = luaai_gettreeloader(l);
	LUATreeWrapper *tree = luaai_totree(l, 1);
	const core::String id = luaL_checkstring(l, 2);
	const core::String name = luaL_checkstring(l, 3);

	TreeNodeParser parser(ctx->getAIFactory(), id);
	const TreeNodePtr& node = parser.getTreeNode(name);
	if (!node) {
		return clua_error(l, "Could not create a node for %s", id.c_str());
	}

	LUANodeWrapper* luaNode = new LUANodeWrapper(node, tree);
	if (!tree->setRoot(node)) {
		delete luaNode;
		return clua_error(l, "%s", ctx->getError().c_str());
	}

	return luaai_pushnode(l, luaNode);
}

static int luaai_node_addnode(lua_State * l) {
	LUATreeLoader *ctx = luaai_gettreeloader(l);
	LUANodeWrapper *node = luaai_tonode(l, 1);
	const core::String id = luaL_checkstring(l, 2);
	const core::String name = luaL_checkstring(l, 3);
	const TreeNodeFactoryContext factoryCtx(name, "", True::get());
	return luaai_pushnode(l, node->addChild(ctx->getAIFactory(), id, factoryCtx));
}

static int luaai_node_setcondition(lua_State * l) {
	LUATreeLoader *ctx = luaai_gettreeloader(l);
	LUANodeWrapper *node = luaai_tonode(l, 1);
	const core::String conditionExpression = luaL_checkstring(l, 2);

	ConditionParser parser(ctx->getAIFactory(), conditionExpression);
	const ConditionPtr& condition = parser.getCondition();
	if (!condition) {
		return clua_error(l, "Could not create a condition for %s: %s", conditionExpression.c_str(), parser.getError().c_str());
	}

	node->setCondition(condition);
	return 0;
}

static void luaai_pushloader(lua_State* s, LUATreeLoader* loader) {
	lua_pushlightuserdata(s, loader);
	lua_setglobal(s, luaai_metatreeloader());
}

LUATreeLoader::LUATreeLoader(const IAIFactory& aiFactory) :
		ITreeLoader(aiFactory) {
}

bool LUATreeLoader::init(const core::String& luaString) {
	shutdown();

	lua::LUA lua;

	static const luaL_Reg treeFuncs[] = {
		{"createRoot", luaai_tree_createroot},
		{"getName",    luaai_tree_getname},
		{"__gc",       luaai_tree_gc},
		{"__tostring", luaai_tree_tostring},
		{nullptr, nullptr}
	};
	clua_registerfuncs(lua, treeFuncs, luaai_metatree());

	static const luaL_Reg nodeFuncs[] = {
		{"addNode",      luaai_node_addnode},
		{"getName",      luaai_node_getname},
		{"setCondition", luaai_node_setcondition},
		{"__gc",         luaai_node_gc},
		{"__tostring",   luaai_node_tostring},
		{nullptr, nullptr}
	};
	clua_registerfuncs(lua, nodeFuncs, luaai_metanode());

	static const luaL_Reg aiFuncs[] = {
		{"createTree", luaai_createtree},
		{nullptr, nullptr}
	};
	clua_registerfuncsglobal(lua, aiFuncs, luaai_metaai(), "AI");
	luaai_pushloader(lua, this);

	clua_mathregister(lua);

	if (!lua.load(luaString)) {
		setError("%s", lua.error().c_str());
		return false;
	}

	// loads all the trees
	if (!lua.execute("init")) {
		setError("%s", lua.error().c_str());
		return false;
	}

	bool empty;
	{
		core::ScopedLock scopedLock(_lock);
		empty = _treeMap.empty();
	}
	if (empty) {
		setError("No behaviour trees specified");
		return false;
	}
	return true;
}

}
