/**
 * @file
 */

#pragma once

#include "voxelformat/SceneGraph.h"

namespace voxel {

int addNodeToSceneGraph(voxel::SceneGraph &sceneGraph, voxel::SceneGraphNode &node, int parent);
int addSceneGraphNode_r(voxel::SceneGraph &sceneGraph, voxel::SceneGraph &newSceneGraph, voxel::SceneGraphNode &node, int parent);
int addSceneGraphNodes(voxel::SceneGraph &sceneGraph, voxel::SceneGraph &newSceneGraph, int parent);

} // namespace voxel
