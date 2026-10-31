/**
 * @file
 */

#pragma once

#include "color/RGBA.h"
#include "core/String.h"
#include "core/collection/Buffer.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/Map.h"
#include "core/GLM.h"
#include <glm/mat4x4.hpp>
#include "scenegraph/SceneGraphNode.h"
#include "voxelformat/external/ogt_vox.h"

namespace io {
class WriteStream;
}

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
	core::Buffer<ogt_vox_keyframe_transform> keyframeTransforms;
	core::Buffer<ogt_vox_keyframe_model> keyframeModels;
	core::Buffer<ogt_vox_cam> cameras;
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

void *_ogt_alloc(size_t size);
void _ogt_free(void *mem);

glm::mat4 ogtToMat(const ogt_vox_transform &t);
ogt_vox_transform matToOgt(const glm::mat4 &mat);

/**
 * @brief MagicaVoxel model-space pivot floor(size/2) mapped into the vengi volume layout used by
 * loadModels (MagicaVoxel (x,y,z) -> vengi (sx-1-x, z, y)), as a normalized [0,1] pivot.
 */
glm::vec3 ogtNormalizedPivot(const ogt_vox_model *model);
glm::vec3 ogtNormalizedPivot(uint32_t sizeX, uint32_t sizeY, uint32_t sizeZ);

/**
 * @brief Convert a MagicaVoxel-space transform matrix into vengi coordinate space for node TRS.
 */
glm::mat4 ogtMatToVengi(const glm::mat4 &ogtMat);

/**
 * @brief Convert a vengi-space transform matrix back into MagicaVoxel coordinate space for saving.
 */
glm::mat4 vengiMatToOgt(const glm::mat4 &vengiMat);

bool loadInstanceKeyFrames(scenegraph::SceneGraphNode &node, const ogt_vox_instance &ogtInstance,
						   const ogt_vox_scene *scene);
bool loadGroupKeyFrames(scenegraph::SceneGraphNode &node, const ogt_vox_anim_transform &transformAnim);

/**
 * @brief Write MagicaVoxel keyframes from a node's local transforms into @p ctx.keyframeTransforms.
 * @return Pointer to the first written keyframe (owned by ctx) and keyframe count via @p numKeyframes
 */
const ogt_vox_keyframe_transform *saveKeyFrames(const scenegraph::SceneGraph &sceneGraph,
												const scenegraph::SceneGraphNode &node, MVSceneContext &ctx,
												uint32_t &numKeyframes);

void loadPaletteFromScene(const ogt_vox_scene *scene, palette::Palette &palette);
bool loadPaletteFromBuffer(const uint8_t *buffer, size_t size, palette::Palette &palette);
void printDetails(const ogt_vox_scene *scene);
void checkRotation(const ogt_vox_transform &transform);
void loadCameras(const ogt_vox_scene *scene, scenegraph::SceneGraph &sceneGraph);
void loadSun(const ogt_vox_scene *scene, scenegraph::SceneGraph &sceneGraph);

static const uint32_t FroxelChunkMVRG = OGT_VOX_MAKE_CHUNK_ID('M', 'V', 'R', 'G');
static const uint32_t FroxelChunkMVAN = OGT_VOX_MAKE_CHUNK_ID('M', 'V', 'A', 'N');
static const uint32_t FroxelChunkMVSC = OGT_VOX_MAKE_CHUNK_ID('M', 'V', 'S', 'C');
static const uint32_t TeardownChunkTDCZ = OGT_VOX_MAKE_CHUNK_ID('T', 'D', 'C', 'Z');

enum FroxelParamKind {
	FroxelParamFloat = 0,
	FroxelParamInt = 1,
	FroxelParamBool = 2,
	FroxelParamColor = 3,
	FroxelParamFloats = 4
};

struct FroxelBone {
	core::String name;
	core::String parent;
	float pivot[3] = {0.0f, 0.0f, 0.0f};
	float rotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};
};

struct FroxelBind {
	int32_t trn = -1;
	uint32_t index = 0;
	core::String name;
	core::String bone;
};

struct FroxelClipKey {
	float time = 0.0f;
	bool hasRotation = false;
	bool hasTranslation = false;
	float rotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	float translation[3] = {0.0f, 0.0f, 0.0f};
	float scale[3] = {1.0f, 1.0f, 1.0f};
};

struct FroxelClipTrack {
	core::String bone;
	core::DynamicArray<FroxelClipKey> keys;
};

struct FroxelClip {
	core::String name;
	core::DynamicArray<FroxelClipTrack> tracks;
};

struct FroxelScriptParam {
	core::String name;
	uint8_t kind = FroxelParamFloat;
	float fvalue = 0.0f;
	float fmin = 0.0f;
	float fmax = 0.0f;
	int32_t ivalue = 0;
	int32_t imin = 0;
	int32_t imax = 0;
	uint8_t bvalue = 0;
	uint8_t rgb[3] = {0, 0, 0};
	uint8_t width = 1;
	uint32_t maxLen = 0;
	core::DynamicArray<float> values;
};

struct FroxelScript {
	core::String name;
	core::String source;
	bool autoRegen = false;
	bool hasLocalMin = false;
	int32_t localMin[3] = {0, 0, 0};
	core::DynamicArray<FroxelScriptParam> params;
};

struct FroxelVoxData {
	core::DynamicArray<FroxelBone> bones;
	core::DynamicArray<FroxelBind> binds;
	core::DynamicArray<FroxelClip> clips;
	core::DynamicArray<FroxelScript> scripts;
};

struct VoxCustomChunkContext {
	FroxelVoxData froxel;
	core::Buffer<uint8_t> modelVoxels;
};

void readFroxelCustomChunk(const ogt_vox_custom_chunk *chunk, void *userData);
void readVoxCustomChunk(const ogt_vox_custom_chunk *chunk, void *userData, ogt_vox_custom_model *outModel);
bool parseFroxelMVRG(const uint8_t *content, uint32_t size, FroxelVoxData &out);
bool parseFroxelMVAN(const uint8_t *content, uint32_t size, FroxelVoxData &out);
bool parseFroxelMVSC(const uint8_t *content, uint32_t size, FroxelVoxData &out);
bool parseTeardownTDCZ(const uint8_t *content, uint32_t size, core::Buffer<uint8_t> &voxels, uint32_t &sizeX,
					   uint32_t &sizeY, uint32_t &sizeZ);
bool encodeTeardownTDCZ(uint32_t sizeX, uint32_t sizeY, uint32_t sizeZ, const uint8_t *voxels, io::WriteStream &out);
bool encodeFroxelMVRG(const FroxelVoxData &data, io::WriteStream &out);
bool encodeFroxelMVAN(const FroxelVoxData &data, io::WriteStream &out);
bool encodeFroxelMVSC(const FroxelVoxData &data, io::WriteStream &out);
/**
 * @brief Import Froxel MVRG / MVAN / MVSC data parsed via @c readFroxelCustomChunk.
 * Bones become Point nodes, clips become scenegraph animations, scripts and
 * bone binds are stored as node properties. @p instanceNodeIds maps each
 * ogt instance index to the scenegraph node created for that instance.
 */
void loadFroxelExtensions(const FroxelVoxData &froxel, const ogt_vox_scene *scene, scenegraph::SceneGraph &sceneGraph,
						  const core::DynamicArray<int> &instanceNodeIds);
bool instanceHidden(const ogt_vox_scene *scene, const ogt_vox_instance &instance);
const char *instanceName(const ogt_vox_scene *scene, const ogt_vox_instance &instance);
color::RGBA instanceColor(const ogt_vox_scene *scene, const ogt_vox_instance &instance);

inline glm::vec3 ogtVolumeSize(const ogt_vox_model *model) {
	return glm::vec3(model->size_x - 1, model->size_y - 1, model->size_z - 1);
}

inline glm::vec4 ogtVolumePivot(const ogt_vox_model *model) {
	return glm::vec4((float)(int)(model->size_x / 2), (float)(int)(model->size_y / 2), (float)(int)(model->size_z / 2),
					 0.0f);
}

/**
 * @brief MagicaVoxel applies floor(M * pos) per voxel. Useful for tests comparing world placement.
 */
inline glm::ivec3 calcTransform(const glm::mat4x4 &mat, const glm::vec3 &pos) {
	return glm::floor(mat * glm::vec4(pos, 1.0f));
}

/**
 * @brief MagicaVoxel bake matrix: global nTRN * translate(0.5) * translate(-floor(size/2)).
 * Used with @c calcTransform for per-voxel placement when @c cfg::VoxformatMVApplyTransform is on.
 */
glm::mat4 ogtInstanceBakeMatrix(const ogt_vox_instance &instance, uint32_t frameIdx, const ogt_vox_scene *scene,
								const ogt_vox_model *model);

/**
 * @brief Bake an ogt model through @p bakeMat into a vengi volume (region mins at 0).
 * @p outShift is the world translation that places the volume (former region lower corner).
 */
voxel::RawVolume *bakeOgtModel(const ogt_vox_model *model, const glm::mat4 &bakeMat, const palette::Palette &palette,
							   glm::ivec3 &outShift);

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
