/**
 * @file
 * @brief C++ view of shaders/_gpuscene.inc (GpuMaterial, GpuEmitLight).
 */

#pragma once

#include "VoxelpathtracerShaderConstants.h"
#include "core/StandardLib.h"
#include <stddef.h>
#include <stdint.h>

#define VENGICPP
#include "shaders/_gpuscene.inc"
#include "math/Vec4f.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"
#include "math/Vec2u.h"
#undef VENGICPP

namespace palette {
struct Material;
}

namespace voxelpathtracer {

constexpr int BrickSize = shader::VoxelpathtracerShaderConstants::getBrickSize();

/** @brief PCG RXS-M-XS hash matching the shader RNG seed permutation. */
uint32_t sampleSeedHash(uint32_t sampleIndex);

/** @brief First two Sobol coordinates, represented as 32-bit fixed-point fractions. */
math::Vec2u cameraSobolPoint(uint32_t sampleIndex);

/** @brief Encode the largest aligned empty block (up to 8 bricks per axis) in each empty cell.
 * Occupied input cells must contain 255, empty cells 0. Output stores log2(block width), or 255 for occupied.
 */
void buildEmptySpaceHierarchy(uint8_t *coarse, const math::Vec3i &size);

/** @brief Flat xyz index for a volume of the given size. */
inline int volumeOffset(int x, int y, int z, const math::Vec3i &size) {
	return (z * size.y + y) * size.x + x;
}

/** @brief Round up to the next multiple of the pathtracer brick size. */
int padToBrick(int v);

/**
 * @brief Clamp and pad the scene AABB to indirection-grid and texture-size limits.
 * @param[in] maxTextureAxis @c Limit::Max3DTextureSize for the indirection and brick-atlas textures.
 * @param[out] truncated Set when the input was shrunk to fit.
 */
math::Vec3i clampVolumeSize(math::Vec3i size, int maxTextureAxis, bool &truncated);

static_assert(sizeof(GpuMaterial) ==
				  (size_t)shader::VoxelpathtracerShaderConstants::getMaterialRows() * 4u * sizeof(float),
			  "GpuMaterial must match MaterialRows RGBA32F texels");
static_assert(sizeof(GpuEmitLight) == 8u * sizeof(float), "GpuEmitLight must match two RGBA32F texels");

static_assert(offsetof(GpuMaterial, type) == 0);
static_assert(offsetof(GpuMaterial, roughness) == 4);
static_assert(offsetof(GpuMaterial, metallic) == 8);
static_assert(offsetof(GpuMaterial, emission) == 12);
static_assert(offsetof(GpuMaterial, ior) == 16);
static_assert(offsetof(GpuMaterial, transparency) == 20);
static_assert(offsetof(GpuMaterial, specular) == 24);
static_assert(offsetof(GpuMaterial, attenuation) == 28);
static_assert(offsetof(GpuMaterial, flux) == 32);
static_assert(offsetof(GpuMaterial, density) == 36);
static_assert(offsetof(GpuMaterial, phase) == 40);
static_assert(offsetof(GpuMaterial, media) == 44);
static_assert(offsetof(GpuMaterial, ldr) == 48);
static_assert(offsetof(GpuMaterial, sp) == 52);
static_assert(offsetof(GpuMaterial, mediatype) == 56);
static_assert(offsetof(GpuMaterial, transmits) == 60);

static_assert(offsetof(GpuEmitLight, cellx) == 0);
static_assert(offsetof(GpuEmitLight, celly) == 4);
static_assert(offsetof(GpuEmitLight, cellz) == 8);
static_assert(offsetof(GpuEmitLight, aliasThreshold) == 12);
static_assert(offsetof(GpuEmitLight, radianceR) == 16);
static_assert(offsetof(GpuEmitLight, radianceG) == 20);
static_assert(offsetof(GpuEmitLight, radianceB) == 24);
static_assert(offsetof(GpuEmitLight, aliasIndex) == 28);

/** @brief Pack palette material data, including a cached transmission classification. */
GpuMaterial packGpuMaterial(const palette::Material &material, uint8_t alpha, bool voxelTransparent);

/** @brief Pack an emitter with the same linear radiance formula used for surface emission. */
GpuEmitLight packGpuEmitLight(const math::Vec3i &cell, const math::Vec3f &srgb, const GpuMaterial &material);

/** @brief Build a luminance-weighted emitter alias table; return total luminance. */
float buildEmitterAliasTable(GpuEmitLight *lights, int count);

/** @brief Build 16 equal-solid-angle sky bands; return integrated luminance, or zero for a nearly uniform sky. */
float buildSkyDistribution(const math::Vec3f &top, const math::Vec3f &horizon, const math::Vec3f &nadir, math::Vec4f *bands);

/** @brief Pack a brick voxel word: materialId in bits 0-13, transmits/emit flags in 14/15. */
inline uint32_t packVoxelPayload(uint32_t materialId, bool transmits, bool emit) {
	const uint32_t idMask = (uint32_t)shader::VoxelpathtracerShaderConstants::getVoxelMaterialIdMask();
	uint32_t word = materialId & idMask;
	if (transmits) {
		word |= (uint32_t)shader::VoxelpathtracerShaderConstants::getVoxelFlagTransmits();
	}
	if (emit) {
		word |= (uint32_t)shader::VoxelpathtracerShaderConstants::getVoxelFlagEmit();
	}
	return word;
}

inline uint32_t voxelMaterialId(uint32_t payload) {
	return payload & (uint32_t)shader::VoxelpathtracerShaderConstants::getVoxelMaterialIdMask();
}

inline bool voxelPayloadTransmits(uint32_t payload) {
	return (payload & (uint32_t)shader::VoxelpathtracerShaderConstants::getVoxelFlagTransmits()) != 0u;
}

inline bool voxelPayloadEmits(uint32_t payload) {
	return (payload & (uint32_t)shader::VoxelpathtracerShaderConstants::getVoxelFlagEmit()) != 0u;
}

/** @brief Compact brick payloads to R16, or nullptr when materialCount exceeds the 14-bit id field.
 * The caller owns the returned allocation.
 */
inline uint16_t *compactGpuVoxelIndices(const uint32_t *indices, size_t count, size_t materialCount) {
	const size_t idMask = (size_t)shader::VoxelpathtracerShaderConstants::getVoxelMaterialIdMask();
	if (materialCount > idMask) {
		return nullptr;
	}
	uint16_t *compact = (uint16_t *)core_malloc(count * sizeof(uint16_t));
	if (compact != nullptr) {
		for (size_t i = 0; i < count; ++i) {
			compact[i] = (uint16_t)indices[i];
		}
	}
	return compact;
}

} // namespace voxelpathtracer
