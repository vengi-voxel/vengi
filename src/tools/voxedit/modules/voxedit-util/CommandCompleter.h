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
			const core::String uuidStr = node.uuid().str();
			const core::String idStr = core::string::toString(node.id());
			if (str.empty() || core::string::startsWith(uuidStr, str) || core::string::startsWith(idStr, str)) {
				matches.push_back(uuidStr);
				++i;
			}
		}
		return i;
	};
}

} // namespace voxedit
