/**
 * @file
 */

#include "LUATreeLoader.h"
#include "tree/TreeNode.h"
#include "AIRegistry.h"
#include "conditions/ConditionParser.h"
#include "tree/TreeNodeParser.h"
#include "commonlua/LUA.h"

namespace ai {

class LUATreeWrapper;

class LUAConditionWrapper {
private:
	const ConditionPtr& _condition;
public:
	LUAConditionWrapper(const ConditionPtr& condition) :
			_condition(condition) {
	}

	inline const ConditionPtr& getCondition() const {
		return _condition;
	}
};

class LUANodeWrapper {
private:
	TreeNodePtr _node;
	LUAConditionWrapper *_condition;
	std::vector<LUANodeWrapper*> _children;
	LUATreeWrapper *_tree;
	const IAIFactory& _aiFactory;
public:
	LUANodeWrapper(const TreeNodePtr& node, LUATreeWrapper *tree, const IAIFactory& aiFactory) :
			_node(node), _condition(nullptr), _tree(tree), _aiFactory(aiFactory) {
	}

	~LUANodeWrapper() {
		delete _condition;
	}

	inline const IAIFactory& getAIFactory() const{
		return _aiFactory;
	}

	inline TreeNodePtr& getTreeNode() {
		return _node;
	}

	inline const TreeNodePtr& getTreeNode() const {
		return _node;
	}

	inline void setCondition(LUAConditionWrapper *condition) {
		_condition = condition;
		_node->setCondition(condition->getCondition());
	}

	inline const std::vector<LUANodeWrapper*>& getChildren() const {
		return _children;
	}

	inline LUAConditionWrapper* getCondition() const {
		return _condition;
	}

	LUANodeWrapper* addChild (const std::string& nodeType, const TreeNodeFactoryContext& ctx) {
		TreeNodeParser parser(_aiFactory, nodeType);
		const TreeNodePtr& child = parser.getTreeNode(ctx.name);
		if (!child) {
			return nullptr;
		}
		LUANodeWrapper *node = new LUANodeWrapper(child, _tree, _aiFactory);
		_children.push_back(node);
		_node->addChild(child);
		return node;
	}
};

class LUATreeWrapper {
private:
	std::string _name;
	LUATreeLoader* _ctx;
	LUANodeWrapper* _root;
public:
	LUATreeWrapper(const std::string& name, LUATreeLoader* ctx) :
			_name(name), _ctx(ctx), _root(nullptr) {
	}

	inline const IAIFactory& getAIFactory() const{
		return _ctx->getAIFactory();
	}

	inline bool setRoot(LUANodeWrapper* root) {
		if (_ctx->addTree(_name, root->getTreeNode())) {
			_root = root;
			return true;
		}

		return false;
	}

	inline const std::string& getName() const {
		return _name;
	}

	inline LUANodeWrapper* getRoot() const {
		return _root;
	}
};

static LUATreeLoader* luaGetContext(lua_State * l) {
	return lua::LUA::globalData<LUATreeLoader>(l, "Loader");
}

static LUATreeWrapper* luaGetTreeContext(lua_State * l, int n) {
	return lua::LUA::userData<LUATreeWrapper>(l, n, "Tree");
}

static LUANodeWrapper* luaGetNodeContext(lua_State * l, int n) {
	return lua::LUA::userData<LUANodeWrapper>(l, n, "Node");
}

#if 0
static LUAConditionWrapper* luaGetConditionContext(lua_State * l, int n) {
	return lua::LUA::userData<LUAConditionWrapper>(l, n, "Condition");
}
#endif

static int luaMain_CreateTree(lua_State * l) {
	LUATreeLoader *ctx = luaGetContext(l);
	const std::string name = luaL_checkstring(l, 1);
	lua::LUA::newUserdata(l, "Tree", new LUATreeWrapper(name, ctx));
	return 1;
}

static int luaTree_GC(lua_State * l) {
	LUATreeWrapper *tree = luaGetTreeContext(l, 1);
	delete tree;
	return 0;
}

static int luaTree_ToString(lua_State * l) {
	const LUATreeWrapper *tree = luaGetTreeContext(l, 1);
	lua_pushfstring(l, "tree: %s [%s]", tree->getName().c_str(), tree->getRoot() ? "root" : "no root");
	return 1;
}

static int luaTree_GetName(lua_State * l) {
	const LUATreeWrapper *tree = luaGetTreeContext(l, 1);
	lua_pushstring(l, tree->getName().c_str());
	return 1;
}

static int luaNode_GC(lua_State * l) {
	LUANodeWrapper *node = luaGetNodeContext(l, 1);
	delete node;
	return 0;
}

static int luaNode_ToString(lua_State * l) {
	const LUANodeWrapper *node = luaGetNodeContext(l, 1);
	const LUAConditionWrapper* condition = node->getCondition();
	lua_pushfstring(l, "node: %d children [condition: %s]", (int)node->getChildren().size(),
			condition ? condition->getCondition()->getName().c_str() : "no condition");
	return 1;
}

static int luaNode_GetName(lua_State * l) {
	const LUANodeWrapper *node = luaGetNodeContext(l, 1);
	lua_pushstring(l, node->getTreeNode()->getName().c_str());
	return 1;
}

static int luaTree_CreateRoot(lua_State * l) {
	LUATreeWrapper *ctx = luaGetTreeContext(l, 1);
	const std::string id = luaL_checkstring(l, 2);
	const std::string name = luaL_checkstring(l, 3);

	TreeNodeParser parser(ctx->getAIFactory(), id);
	const TreeNodePtr& node = parser.getTreeNode(name);
	if (!node) {
		return lua::LUA::returnError(l, "Could not create a node for " + id);
	}

	LUANodeWrapper* udata = lua::LUA::newUserdata(l, "Node", new LUANodeWrapper(node, ctx, ctx->getAIFactory()));
	if (!ctx->setRoot(udata)) {
		LUATreeLoader *loader = luaGetContext(l);
		return lua::LUA::returnError(l, loader->getError());
	}
	return 1;
}

static int luaNode_AddNode(lua_State * l) {
	LUANodeWrapper *node = luaGetNodeContext(l, 1);
	const std::string id = luaL_checkstring(l, 2);
	const std::string name = luaL_checkstring(l, 3);

	TreeNodeFactoryContext factoryCtx(name, "", True::get());
	LUANodeWrapper* udata = lua::LUA::newUserdata(l, "Node", node->addChild(id, factoryCtx));
	if (udata == nullptr) {
		return lua::LUA::returnError(l, "Could not create a node for " + id);
	}
	return 1;
}

static int luaNode_SetCondition(lua_State * l) {
	LUATreeLoader *ctx = luaGetContext(l);
	LUANodeWrapper *node = luaGetNodeContext(l, 1);
	const std::string conditionExpression = luaL_checkstring(l, 2);

	ConditionParser parser(ctx->getAIFactory(), conditionExpression);
	const ConditionPtr& condition = parser.getCondition();
	if (!condition) {
		return lua::LUA::returnError(l, "Could not create a condition for " + conditionExpression + ": " + parser.getError());
	}

	LUAConditionWrapper* udata = lua::LUA::newUserdata(l, "Condition", new LUAConditionWrapper(condition));
	node->setCondition(udata);
	return 1;
}

LUATreeLoader::LUATreeLoader(const IAIFactory& aiFactory) :
		ITreeLoader(aiFactory) {
}

bool LUATreeLoader::init(const std::string& luaString) {
	shutdown();

	lua::LUA lua;
	luaL_Reg createTree = { "createTree", luaMain_CreateTree };
	luaL_Reg eof = { nullptr, nullptr };
	luaL_Reg funcs[] = { createTree, eof };

	lua::LUAType tree = lua.registerType("Tree");
	tree.addFunction("createRoot", luaTree_CreateRoot);
	tree.addFunction("getName", luaTree_GetName);
	tree.addFunction("__gc", luaTree_GC);
	tree.addFunction("__tostring", luaTree_ToString);

	lua::LUAType node = lua.registerType("Node");
	node.addFunction("addNode", luaNode_AddNode);
	node.addFunction("getName", luaNode_GetName);
	node.addFunction("setCondition", luaNode_SetCondition);
	node.addFunction("__gc", luaNode_GC);
	node.addFunction("__tostring", luaNode_ToString);

	lua.reg("AI", funcs);

	if (!lua.load(luaString)) {
		setError("%s", lua.error().c_str());
		return false;
	}

	// loads all the trees
	lua.newGlobalData<LUATreeLoader>("Loader", this);
	if (!lua.execute("init")) {
		setError("%s", lua.error().c_str());
		return false;
	}

	bool empty;
	{
		ScopedReadLock scopedLock(_lock);
		empty = _treeMap.empty();
	}
	if (empty) {
		setError("No behaviour trees specified");
		return false;
	}
	return true;
}

}
