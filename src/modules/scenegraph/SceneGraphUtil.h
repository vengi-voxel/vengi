/**
 * @file
 * @ingroup SceneGraph
 */

#pragma once

#include "scenegraph/SceneGraph.h"

namespace scenegraph {

void copyNode(const SceneGraphNode &src, SceneGraphNode &target, bool copyVolume, bool copyKeyFrames = true);

/**
 * @brief this makes a copy of the volumes affected
 */
int addNodeToSceneGraph(SceneGraph &sceneGraph, const SceneGraphNode &node, int parent, bool recursive = false);

/**
 * @brief this doesn't copy but transfer the volume ownership
 */
int addNodeToSceneGraph(SceneGraph &sceneGraph, SceneGraphNode &node, int parent, bool recursive = false);

int addSceneGraphNodes(SceneGraph& target, SceneGraph& source, int parent);

int copySceneGraph(SceneGraph &target, const SceneGraph &source);

int createNodeReference(SceneGraph &target, const SceneGraphNode &node);

void splitVolumes(const scenegraph::SceneGraph &srcSceneGraph, scenegraph::SceneGraph &destSceneGraph,
				  const glm::ivec3 &maxSize = glm::ivec3(128), bool crop = false);

} // namespace voxel
