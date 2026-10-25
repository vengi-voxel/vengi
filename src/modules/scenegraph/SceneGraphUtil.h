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

/**
 * @brief Sample local TRS from nonempty, frame-sorted keyframes, clamping outside their range.
 * CatmullRom uses the neighboring key values as spline controls for translation and scale.
 * @param step If supplied, receives whether the source key uses Instant interpolation.
 */
SceneGraphTransform sampleLocalTransform(const SceneGraphKeyFrames &keys, FrameIndex frame, bool *step = nullptr);

/**
 * @brief Interpolate a 1D value. @p current / @p start / @p end are the time range.
 * CatmullRom uses all four values; other types ease between @p p1 and @p p2.
 */
double interpolate(InterpolationType interpolationType, double current, double start, double end, double p0, double p1,
				   double p2, double p3);

/**
 * @brief Time remap onto @c [start, end]. CatmullRom without neighbors duplicates the endpoints.
 */
double interpolate(InterpolationType interpolationType, double current, double start, double end);

/**
 * @brief Apply a world-space translation to all keyframe transforms of @c node
 */
void offsetNodeWorldTransforms(SceneGraphNode &node, const glm::vec3 &delta);

} // namespace scenegraph
