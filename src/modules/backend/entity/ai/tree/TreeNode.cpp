/**
 * @file
 */

#include "TreeNode.h"
#include "backend/entity/ai/AI.h"
#include "backend/entity/ai/condition/ICondition.h"
#include "core/Assert.h"
#include <algorithm>

namespace backend {

int TreeNode::getId() const {
	return _id;
}

void TreeNode::setName(const core::String& name) {
	if (name.empty()) {
		return;
	}
	_name = name;
}

const core::String& TreeNode::getType() const {
	return _type;
}

const core::String& TreeNode::getName() const {
	return _name;
}

const ConditionPtr& TreeNode::getCondition() const {
	return _condition;
}

void TreeNode::setCondition(const ConditionPtr& condition) {
	_condition = condition;
}

const core::String& TreeNode::getParameters() const {
	return _parameters;
}

const TreeNodes& TreeNode::getChildren() const {
	return _children;
}

TreeNodes& TreeNode::getChildren() {
	return _children;
}

bool TreeNode::addChild(const TreeNodePtr& child) {
	_children.push_back(child);
	return true;
}

void TreeNode::resetState(const AIPtr& entity) {
	for (auto& c : _children) {
		c->resetState(entity);
	}
}

void TreeNode::getRunningChildren(const AIPtr& /*entity*/, std::vector<bool>& active) const {
	const int size = (int)_children.size();
	for (int i = 0; i < size; ++i) {
		active.push_back(false);
	}
}

void TreeNode::setLastExecMillis(const AIPtr& entity) {
	if (!entity->_debuggingActive) {
		return;
	}
	entity->_lastExecMillis[getId()] = entity->_time;
}

int TreeNode::getSelectorState(const AIPtr& entity) const {
	AI::SelectorStates::const_iterator i = entity->_selectorStates.find(getId());
	if (i == entity->_selectorStates.end()) {
		return AI_NOTHING_SELECTED;
	}
	return i->second;
}

void TreeNode::setSelectorState(const AIPtr& entity, int selected) {
	entity->_selectorStates[getId()] = selected;
}

int TreeNode::getLimitState(const AIPtr& entity) const {
	AI::LimitStates::const_iterator i = entity->_limitStates.find(getId());
	if (i == entity->_limitStates.end()) {
		return 0;
	}
	return i->second;
}

void TreeNode::setLimitState(const AIPtr& entity, int amount) {
	entity->_limitStates[getId()] = amount;
}

ai::TreeNodeStatus TreeNode::state(const AIPtr& entity, ai::TreeNodeStatus treeNodeState) {
	if (!entity->_debuggingActive) {
		return treeNodeState;
	}
	entity->_lastStatus[getId()] = treeNodeState;
	return treeNodeState;
}

int64_t TreeNode::getLastExecMillis(const AIPtr& entity) const {
	if (!entity->_debuggingActive) {
		return -1L;
	}
	AI::LastExecMap::const_iterator i = entity->_lastExecMillis.find(getId());
	if (i == entity->_lastExecMillis.end()) {
		return -1L;
	}
	return i->second;
}

ai::TreeNodeStatus TreeNode::getLastStatus(const AIPtr& entity) const {
	if (!entity->_debuggingActive) {
		return ai::UNKNOWN;
	}
	AI::NodeStates::const_iterator i = entity->_lastStatus.find(getId());
	if (i == entity->_lastStatus.end()) {
		return ai::UNKNOWN;
	}
	return i->second;
}

TreeNodePtr TreeNode::getChild(int id) const {
	for (auto& child : _children) {
		if (child->getId() == id) {
			return child;
		}
		const TreeNodePtr& node = child->getChild(id);
		if (node) {
			return node;
		}
	}
	return TreeNodePtr();
}

bool TreeNode::replaceChild(int id, const TreeNodePtr& newNode) {
	auto i = std::find_if(_children.begin(), _children.end(), [id] (const TreeNodePtr& other) { return other->getId() == id; });
	if (i == _children.end()) {
		return false;
	}

	if (newNode) {
		*i = newNode;
		return true;
	}

	_children.erase(i);
	return true;
}

TreeNodePtr TreeNode::getParent_r(const TreeNodePtr& parent, int id) const {
	for (auto& child : _children) {
		if (child->getId() == id) {
			return parent;
		}
		const TreeNodePtr& parentPtr = child->getParent_r(child, id);
		if (parentPtr) {
			return parentPtr;
		}
	}
	return TreeNodePtr();
}

TreeNodePtr TreeNode::getParent(const TreeNodePtr& self, int id) const {
	core_assert_msg(getId() != id, "Root nodes don't have a parent");
	for (auto& child : _children) {
		if (child->getId() == id) {
			return self;
		}
		const TreeNodePtr& parent = child->getParent_r(child, id);
		if (parent) {
			return parent;
		}
	}
	return TreeNodePtr();
}

ai::TreeNodeStatus TreeNode::execute(const AIPtr& entity, int64_t /*deltaMillis*/) {
	if (!_condition->evaluate(entity)) {
		return state(entity, ai::CANNOTEXECUTE);
	}

	setLastExecMillis(entity);
	return state(entity, ai::FINISHED);
}

}
