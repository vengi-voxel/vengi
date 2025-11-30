/**
 * @file
 */

#pragma once

#include "color/RGBA.h"
#include "core/collection/Array.h"
#include "core/collection/Buffer.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/Map.h"
#include "core/GLM.h"
#include <glm/mat4x4.hpp>
#include "scenegraph/SceneGraphNode.h"
#include "voxelformat/external/ogt_vox.h"

#define MAGICAVOXEL_USE_REFERENCES 0

namespace palette {
class Palette;
}

namespace voxel {
class RawVolume;
} // namespace voxel

namespace scenegraph {
class SceneGraph;
class SceneGraphNode;
} // namespace scenegraph

namespace voxelformat {

struct MVSceneContext {
	core::Buffer<ogt_vox_group> groups;
	core::Buffer<ogt_vox_model> models;
	core::Buffer<ogt_vox_layer> layers;
	core::Buffer<ogt_vox_instance> instances;
	int transformKeyFrameIdx = 0;
	core::Array<ogt_vox_keyframe_transform, 4096> keyframeTransforms;
	core::Buffer<ogt_vox_cam> cameras;
	bool paletteErrorPrinted = false;
	core::Map<int, uint32_t> nodeToModel;
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
 * @return glm::vec4 The transformed world position
 */
inline glm::ivec3 calcTransform(const glm::mat4x4 &mat, const glm::vec3 &pos) {
	// magicavoxel is doing this in the shader - we have to do it on the cpu as the matrix which is linear
	// can't get expressed by this non-linear floor operation - or at least I don't know how. We could try
	// to floor the translation part of the matrix - as the rotations should be 90 degree aligned. But this
	// would just be an approximation and not the correct way to do it. So we have to do it the hard way and
	// apply the transformation to the local position and floor the result.
	return glm::floor(mat * glm::vec4(pos, 1.0f));
}

void *_ogt_alloc(size_t size);
void _ogt_free(void *mem);

bool loadKeyFrames(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node,
				   const ogt_vox_instance &ogtInstance, const ogt_vox_scene *scene);
glm::mat4 ogtTransformToMat(const ogt_vox_instance &ogtInstance, uint32_t frameIdx, const ogt_vox_scene *scene,
							const ogt_vox_model *ogtModel);
void loadPaletteFromScene(const ogt_vox_scene *scene, palette::Palette &palette);
bool loadPaletteFromBuffer(const uint8_t *buffer, size_t size, palette::Palette &palette);
void printDetails(const ogt_vox_scene *scene);
void checkRotation(const ogt_vox_transform &transform);
void loadCameras(const ogt_vox_scene *scene, scenegraph::SceneGraph &sceneGraph);
bool instanceHidden(const ogt_vox_scene *scene, const ogt_vox_instance &instance);
const char *instanceName(const ogt_vox_scene *scene, const ogt_vox_instance &instance);
color::RGBA instanceColor(const ogt_vox_scene *scene, const ogt_vox_instance &instance);

inline glm::vec3 ogtVolumeSize(const ogt_vox_model *model) {
	return glm::vec3(model->size_x - 1, model->size_y - 1, model->size_z - 1);
}

/**
 * @note The pivot to do the rotation around. This is the @code chunk_size - 1 + 0.5 @endcode. Please
 * note that the @c w component must be @c 0.0
 */
inline glm::vec4 ogtVolumePivot(const ogt_vox_model *model) {
	return glm::vec4((float)(int)(model->size_x / 2), (float)(int)(model->size_y / 2), (float)(int)(model->size_z / 2),
					 0.0f);
}

struct MVModelToNode {
	MVModelToNode();
	~MVModelToNode();
	inline MVModelToNode(voxel::RawVolume *_volume, int _nodeId) : volume(_volume), nodeId(_nodeId) {
	}
	MVModelToNode(MVModelToNode &&other) noexcept;
	MVModelToNode &operator=(MVModelToNode &&other) noexcept;

	voxel::RawVolume *volume;
	int nodeId;
};
core::DynamicArray<MVModelToNode> loadModels(const ogt_vox_scene *scene, const palette::Palette &palette);

} // namespace voxelformat
