/**
 * @file
 */
#pragma once

#include "TreeNode.h"
#include "backend/entity/ai/common/IParser.h"
#include "Steer.h"

namespace backend {

class IAIFactory;

/**
 * @brief Transforms the string representation of a @c TreeNode with all its
 * parameters into a @c TreeNode instance.
 *
 * @c #NodeName{Parameters}
 * Parameters are by default optional - but that really depends on the
 * @c TreeNode implementation.
 */
class TreeNodeParser: public IParser {
private:
	const IAIFactory& _aiFactory;
	core::String _taskString;

	void splitTasks(const core::String& string, std::vector<core::String>& tokens) const;
	SteeringPtr getSteering(const core::String& nodeName);
public:
	TreeNodeParser(const IAIFactory& aiFactory, const core::String& taskString) :
			IParser(), _aiFactory(aiFactory) {
		_taskString = Str::eraseAllSpaces(taskString);
	}

	virtual ~TreeNodeParser() {}

	TreeNodePtr getTreeNode(const core::String& name = "");
};

}
