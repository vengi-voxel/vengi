/**
 * @file
 */

#pragma once

#include "math/Math.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "math/tests/TestMathHelper.h"

namespace palette {

inline ::std::ostream &operator<<(::std::ostream &os, const palette::Palette &palette) {
	return os << palette::Palette::print(palette).c_str();
}

inline ::std::ostream &operator<<(::std::ostream &os, const palette::Material &material) {
	os << "Material: " << (int)material.type << " ";
	for (uint32_t i = 0; i < palette::MaterialProperty::MaterialMax - 1; ++i) {
		if (!material.has((palette::MaterialProperty)i)) {
			continue;
		}
		os << palette::MaterialPropertyNames[i] << ": " << material.value((palette::MaterialProperty)i) << ", ";
	}
	return os;
}

} // namespace palette

namespace scenegraph {

inline ::std::ostream &operator<<(::std::ostream &os, const scenegraph::SceneGraph &sceneGraph) {
	os << "SceneGraph: " << sceneGraph.size() << " nodes\n";
	for (const auto &entry : sceneGraph.nodes()) {
		const SceneGraphNode &node = entry->second;
		os << " - " << SceneGraphNodeTypeStr[(int)node.type()] << ": " << node.name().c_str() << " (" << node.uuid().c_str() << ")\n";
	}
	return os;
}

} // namespace scenegraph
