/**
 * @file
 */

#pragma once

#include "core/StringUtil.h"
#include "scenegraph/SceneGraph.h"

namespace voxedit {

inline auto nodeCompleter(const scenegraph::SceneGraph &sceneGraph) {
	return [&](const core::String &str, core::DynamicArray<core::String> &matches) -> int {
		int i = 0;
		for (const auto &entry : sceneGraph.nodes()) {
			const scenegraph::SceneGraphNode &node = entry->value;
			if (!node.isAnyModelNode()) {
				continue;
			}
			matches.push_back(core::string::toString(node.id()));
		}
		return i;
	};
}

} // namespace voxedit