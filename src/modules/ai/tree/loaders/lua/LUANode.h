#pragma once

#include <string>
#include <vector>
#include <tree/TreeNode.h>
#include <tree/loaders/lua/LUACondition.h>

namespace ai {

class LUACondition;
class LUATree;

class LUANode {
private:
	TreeNodePtr _node;
	LUACondition *_condition;
	std::vector<LUANode*> _children;
	LUATree *_tree;
public:
	LUANode(const TreeNodePtr& node, LUATree *tree) :
			_node(node), _condition(nullptr), _tree(tree) {
	}

	~LUANode() {
		delete _condition;
	}

	inline TreeNodePtr& getTreeNode() {
		return _node;
	}

	inline const TreeNodePtr& getTreeNode() const {
		return _node;
	}

	inline void setCondition(LUACondition *condition) {
		_condition = condition;
		_node->setCondition(condition->getCondition());
	}

	inline const std::vector<LUANode*>& getChildren() const {
		return _children;
	}

	inline LUACondition* getCondition() const {
		return _condition;
	}

	LUANode* addChild (const std::string& nodeType, const TreeNodeFactoryContext& ctx);
};

}
