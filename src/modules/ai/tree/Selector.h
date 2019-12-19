/**
 * @file
 */
#pragma once

#include "TreeNode.h"

namespace ai {

#define SELECTOR_CLASS(NodeName) \
	NodeName(const std::string& name, const std::string& parameters, const ConditionPtr& condition) : \
		Selector(name, parameters, condition) { \
		_type = AI_STRINGIFY(NodeName); \
	} \
	virtual ~NodeName() { \
	} \
	\
	NODE_FACTORY(NodeName)

/**
 * @brief Base class for all type of @c TreeNode selectors.
 *
 * [AiGameDev](http://aigamedev.com/open/article/selector/)
 */
class Selector: public TreeNode {
public:
	NODE_CLASS(Selector)

	/**
	 * @brief Will only deliver valid results if the debugging for the given entity is active
	 */
	virtual void getRunningChildren(const AIPtr& entity, std::vector<bool>& active) const override {
		int n = 0;
		int selectorState = getSelectorState(entity);
		for (TreeNodes::const_iterator i = _children.begin(); i != _children.end(); ++i, ++n) {
			active.push_back(selectorState == n);
		}
	}
};

}
