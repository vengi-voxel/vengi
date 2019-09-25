/**
 * @file
 */

#pragma once

#include "core/GLM.h"

namespace physic {

class VoxTreeNode {
public:
	enum NodeValue {
		nodeUnknown, nodeEmpty, nodeFull, nodeMixed
	};
public:
	VoxTreeNode(const glm::ivec3& mins, const glm::ivec3& maxs);
	~VoxTreeNode();
public:
	NodeValue _value;
	const glm::ivec3 _mins;
	const glm::ivec3 _maxs;
	VoxTreeNode *_children[2];
};

inline VoxTreeNode::VoxTreeNode(const glm::ivec3& mins, const glm::ivec3& maxs) :
		_value(nodeUnknown), _mins(mins), _maxs(maxs) {
	_children[0] = _children[1] = nullptr;
}

inline VoxTreeNode::~VoxTreeNode() {
	delete _children[0];
	delete _children[1];
}

}
