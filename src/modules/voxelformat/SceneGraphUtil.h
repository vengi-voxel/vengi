/**
 * @file
 */

#pragma once

#include "voxelformat/SceneGraph.h"

namespace voxelformat {

// this makes a copy of the volumes affected
int addNodeToSceneGraph(SceneGraph &sceneGraph, const SceneGraphNode &node, int parent);

// this doesn't copy but transfer the volume ownership
int addNodeToSceneGraph(SceneGraph &sceneGraph, SceneGraphNode &node, int parent);

int addSceneGraphNodes(SceneGraph &sceneGraph, SceneGraph &newSceneGraph, int parent);

} // namespace voxel
