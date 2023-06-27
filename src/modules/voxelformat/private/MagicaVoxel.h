/**
 * @file
 */

#pragma once

#include "core/collection/Array.h"
#include "core/collection/Buffer.h"
#include "voxelformat/external/ogt_vox.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>

namespace voxel {
class Palette;
} // namespace voxel

namespace scenegraph {
class SceneGraph;
class SceneGraphNode;
} // namespace scenegraph

namespace voxelformat {

struct ogt_SceneContext {
	core::Buffer<ogt_vox_group> groups;
	core::Buffer<ogt_vox_model> models;
	core::Buffer<ogt_vox_layer> layers;
	core::Buffer<ogt_vox_instance> instances;
	int transformKeyFrameIdx = 0;
	core::Array<ogt_vox_keyframe_transform, 4096> keyframeTransforms;
	core::Buffer<ogt_vox_cam> cameras;
	bool paletteErrorPrinted = false;
};

// clang-format off
static const ogt_vox_transform ogt_identity_transform{
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f
};
// clang-format on

/**
 * @brief Calculate the scene graph object transformation. Used for the voxel and the AABB of the volume.
 *
 * @param mat The world space model matrix (rotation and translation) for the chunk
 * @param pos The position inside the untransformed chunk (local position)
 * @param pivot The pivot to do the rotation around. This is the @code chunk_size - 1 + 0.5 @endcode. Please
 * note that the @c w component must be @c 0.0
 * @return glm::vec4 The transformed world position
 */
inline glm::vec4 calcTransform(const glm::mat4x4 &mat, const glm::ivec3 &pos, const glm::vec4 &pivot) {
	return glm::floor(mat * (glm::vec4((float)pos.x + 0.5f, (float)pos.y + 0.5f, (float)pos.z + 0.5f, 1.0f) - pivot));
}

void *_ogt_alloc(size_t size);
void _ogt_free(void *mem);

bool loadKeyFrames(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node,
				   const ogt_vox_instance &ogtInstance, const ogt_vox_scene *scene);
glm::mat4 ogtTransformToMat(const ogt_vox_transform &t);
void loadPaletteFromScene(const ogt_vox_scene *scene, voxel::Palette &palette);
bool loadPaletteFromBuffer(const uint8_t *buffer, size_t size, voxel::Palette &palette);
void printDetails(const ogt_vox_scene *scene);
void checkRotation(const ogt_vox_transform &transform);
int findClosestPaletteIndex(const voxel::Palette &palette);
void loadCameras(const ogt_vox_scene *scene, scenegraph::SceneGraph &sceneGraph);

} // namespace voxelformat
