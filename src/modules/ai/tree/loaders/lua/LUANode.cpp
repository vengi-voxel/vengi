#include "LUATree.h"
#include "LUANode.h"
#include <tree/TreeNodeParser.h>

namespace ai {

LUANode* LUANode::addChild(const std::string& nodeType, const TreeNodeFactoryContext& ctx) {
	TreeNodeParser parser(_tree->getAIFactory(), nodeType);
	const TreeNodePtr& child = parser.getTreeNode(ctx.name);
	if (!child) {
		return nullptr;
	}
	LUANode *node = new LUANode(child, _tree);
	_children.push_back(node);
	_node->addChild(child);
	return node;
}

}
