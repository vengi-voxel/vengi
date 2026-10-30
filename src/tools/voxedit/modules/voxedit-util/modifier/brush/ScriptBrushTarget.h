/**
 * @file
 */
#pragma once

#include "palette/NormalPalette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphUtil.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxedit {

/** Scripts evaluate against a private target. Scene management remains a host service. */
template<class Evaluate>
bool evaluateScriptTarget(ModifierVolumeWrapper &destination, Evaluate &&evaluate) {
	scenegraph::SceneGraph scene;
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model, destination.node().uuid());
	scenegraph::copyNode(destination.node(), node, false);
	node.setVolume(new voxel::RawVolume(*destination.volume()));
	const int id = scene.emplace(core::move(node));
	scene.setActiveNode(id);
	ModifierVolumeWrapper scratch(scene.node(id), ModifierType::Override);
	evaluate(scene, scratch);
	const auto *result = scene.findNodeByUUID(destination.node().uuid());
	if (!result || !result->volume() || result->region() != destination.region()) {
		return false;
	}
	destination.node().setPalette(result->palette());
	destination.node().setNormalPalette(result->normalPalette());
	voxelutil::visitVolume(
		*result->volume(), result->region(),
		[&](int x, int y, int z, const voxel::Voxel &value) {
			if (!(value == destination.voxel(x, y, z))) {
				destination.setVoxel(x, y, z, value);
			}
		},
		voxelutil::VisitAll());
	return true;
}

} // namespace voxedit
