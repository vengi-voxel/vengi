/**
 * @file
 */

#pragma once

#include "voxelformat/SceneGraph.h"

namespace voxel {

// this makes a copy of the volumes affected
int addNodeToSceneGraph(voxel::SceneGraph &sceneGraph, const voxel::SceneGraphNode &node, int parent);

// this doesn't copy but transfer the volume ownership
int addNodeToSceneGraph(voxel::SceneGraph &sceneGraph, voxel::SceneGraphNode &node, int parent);

int addSceneGraphNodes(voxel::SceneGraph &sceneGraph, voxel::SceneGraph &newSceneGraph, int parent);

} // namespace voxel
