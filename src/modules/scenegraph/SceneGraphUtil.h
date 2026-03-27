/**
 * @file
 * @ingroup SceneGraph
 */

#pragma once

#include "core/collection/Buffer.h"
#include "scenegraph/SceneGraphNode.h"
#include "core/Function.h"

namespace scenegraph {

void copyNode(const SceneGraphNode &src, SceneGraphNode &target, bool copyVolume, bool copyKeyFrames = true);

/**
 * @brief this makes a copy of the volumes affected
 */
int copyNodeToSceneGraph(SceneGraph &sceneGraph, const SceneGraphNode &node, int parent, bool recursive = false);

/**
 * @brief this doesn't copy but transfer the volume ownership
 */
int moveNodeToSceneGraph(SceneGraph &sceneGraph, SceneGraphNode &node, int parent, const core::Function<void(int)> &onNodeAdded = {});

int addSceneGraphNodes(SceneGraph &target, SceneGraph &source, int parent, const core::Function<void(int)> &onNodeAdded = {});

/**
 * @param copyVolumes if @c true, volumes are deep-copied and owned by the target. If @c false, volumes are
 * referenced without copying - the caller must ensure the source outlives the target.
 */
core::Buffer<int> copySceneGraph(SceneGraph &target, const SceneGraph &source, int parent = 0, bool copyVolumes = true);

/**
 * @brief Copy the scene graph but resolve model references into actual model nodes
 *
 * Model reference nodes are converted to model nodes with a copy of the referenced volume.
 * This is useful for saving to formats that don't support model references.
 * @param copyVolumes if @c true, volumes are deep-copied and owned by the target. If @c false, volumes are
 * referenced without copying - the caller must ensure the source outlives the target.
 */
void copySceneGraphResolveReferences(SceneGraph &target, const SceneGraph &source, int parent = 0, bool copyVolumes = true);

/**
 * @param[in] parent The parent node id - if @c -1 it will use the node parent id
 */
int createNodeReference(SceneGraph &sceneGraph, const SceneGraphNode &node, int parent = -1);

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
