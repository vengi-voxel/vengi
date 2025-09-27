/**
 * @file
 */

#pragma once

#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include <gtest/gtest.h>

namespace scenegraph {

inline ::std::ostream &operator<<(::std::ostream &os, const scenegraph::SceneGraph &sceneGraph) {
	os << "SceneGraph: " << sceneGraph.size() << " nodes\n";
	for (const auto &entry : sceneGraph.nodes()) {
		const SceneGraphNode &node = entry->second;
		const core::String &uuidStr = node.uuid().str();
		os << " - " << SceneGraphNodeTypeStr[(int)node.type()] << ": " << node.name().c_str() << " (" << uuidStr.c_str() << ")\n";
	}
	return os;
}

} // namespace scenegraph
