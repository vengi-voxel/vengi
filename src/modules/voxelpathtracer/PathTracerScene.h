/**
 * @file
 * @brief CPU-side scene packing for the GPU pathtracer (sparse bricks, materials, emitters).
 */

#pragma once

#include "GpuSceneData.h"
#include "core/collection/Buffer.h"
#include <glm/vec3.hpp>
#include <stdint.h>

namespace palette {
class Palette;
}

namespace scenegraph {
class SceneGraph;
}

namespace voxelpathtracer {

static constexpr int MaxEmitLights = 4096;
/** @brief Indirection entries below this are empty-skip levels; brick ids are stored as id + BrickIdBias. */
static constexpr uint32_t BrickIdBias = 4u;

int pathTracerBrickSize();
int pathTracerVoxelsPerBrick();

/**
 * @brief Sparse GPU volume: coarse indirection grid + packed brick atlas.
 * Caller owns indirection, bricks, and brickCoords and must core_free them.
 */
struct SparseGpuVolume {
	uint32_t *indirection = nullptr;
	glm::ivec3 coarseSize{0};
	uint32_t *bricks = nullptr;
	glm::ivec3 *brickCoords = nullptr;
	int brickCount = 0;
	int bricksPerRow = 1;
	glm::ivec3 atlasSize{0};
	bool bricksTruncated = false;
};

void freeSparseGpuVolume(SparseGpuVolume &volume);

/**
 * @brief Dense material table for used palette colors (1..N).
 * Caller owns albedoRgba and materials and must core_free them.
 */
struct GpuMaterialAtlas {
	uint8_t *albedoRgba = nullptr;
	GpuMaterial *materials = nullptr;
	int materialCount = 0;
};

void freeGpuMaterialAtlas(GpuMaterialAtlas &atlas);

void collectScenePalettes(const scenegraph::SceneGraph &sceneGraph, core::Buffer<const palette::Palette *> &palettes);

/**
 * @brief Rasterize visible models into a sparse brick pool.
 * @param[in] maxTextureAxis GL_MAX_3D_TEXTURE_SIZE style limit for the brick atlas.
 * @param[in,out] transmissive Temporary palette*color flags for alpha/glass classification.
 * Bricks still store temporary (slot,color) indices until buildDenseMaterialAtlas().
 */
bool buildSparseGpuVolume(const scenegraph::SceneGraph &sceneGraph, core::Buffer<const palette::Palette *> &palettes,
						  const glm::ivec3 &worldMin, const glm::ivec3 &volumeSize, int maxTextureAxis,
						  SparseGpuVolume &out, uint8_t *transmissive);

/**
 * @brief Remap brick voxels to dense materialId+flags and build flat albedo/material tables.
 * @param[in,out] volume Bricks are rewritten from temporary palette indices to packed payloads.
 */
bool buildDenseMaterialAtlas(SparseGpuVolume &volume, const core::Buffer<const palette::Palette *> &palettes,
							 const uint8_t *transmissive, GpuMaterialAtlas &out);

void gatherSceneEmitLights(const SparseGpuVolume &volume, int materialCount, const GpuMaterial *materials,
						   const uint8_t *albedoRgba, GpuEmitLight *emitLights, int &emitCount,
						   float &emitterLuminance);

} // namespace voxelpathtracer
