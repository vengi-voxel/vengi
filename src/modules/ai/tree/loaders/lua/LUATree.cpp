#include "LUATree.h"
#include "LUANode.h"
#include "LUATreeLoader.h"

namespace ai {

bool LUATree::setRoot(LUANode* root) {
	if (_ctx->addTree(_name, root->getTreeNode())) {
		_root = root;
		return true;
	}

	return false;
}

const IAIFactory& LUATree::getAIFactory() const {
	return _ctx->getAIFactory();
}

}
