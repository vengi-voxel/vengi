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

int addSceneGraphNodes(SceneGraph &target, SceneGraph &source, int parent);

int copySceneGraph(SceneGraph &target, const SceneGraph &source);

int createNodeReference(SceneGraph &target, const SceneGraphNode &node);

/**
 * @param createEmpty if @c true, for empty parts of the source volume empty volumes will be created, too. Otherwise
 * they will be ignored.
 * @param skipHidden if @c true, hidden nodes will be skipped from splitting. They won't appear in the new
 * @c scenegraph::SceneGraph instance.
 */
bool splitVolumes(const scenegraph::SceneGraph &srcSceneGraph, scenegraph::SceneGraph &destSceneGraph,
				  bool crop = false, bool createEmpty = false, bool skipHidden = false,
				  const glm::ivec3 &maxSize = glm::ivec3(128));

double interpolate(InterpolationType interpolationType, double current, double start, double end);

} // namespace scenegraph
