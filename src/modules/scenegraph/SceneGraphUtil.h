/**
 * @file
 */

#pragma once

#include "scenegraph/SceneGraph.h"

namespace scenegraph {

void copyNode(const SceneGraphNode &src, SceneGraphNode &target, bool copyVolume, bool copyKeyFrames = true);

// this makes a copy of the volumes affected
int addNodeToSceneGraph(SceneGraph &sceneGraph, const SceneGraphNode &node, int parent, bool recursive = false);

// this doesn't copy but transfer the volume ownership
int addNodeToSceneGraph(SceneGraph &sceneGraph, SceneGraphNode &node, int parent, bool recursive = false);

int addSceneGraphNodes(SceneGraph& target, SceneGraph& source, int parent);

int copySceneGraph(SceneGraph &target, const SceneGraph &source, int parent);

int createNodeReference(SceneGraph &target, const SceneGraphNode &node);

} // namespace voxel
