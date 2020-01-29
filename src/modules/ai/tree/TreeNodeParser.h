/**
 * @file
 */
#pragma once

#include "tree/TreeNode.h"
#include "common/IParser.h"
#include "Steer.h"

namespace ai {

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

	void splitTasks(const core::String& string, std::vector<std::string>& tokens) const;
	SteeringPtr getSteering(const core::String& nodeName);
public:
	TreeNodeParser(const IAIFactory& aiFactory, const core::String& taskString) :
			IParser(), _aiFactory(aiFactory) {
		_taskString = ai::Str::eraseAllSpaces(taskString);
	}

	virtual ~TreeNodeParser() {}

	TreeNodePtr getTreeNode(const core::String& name = "");
};

}
