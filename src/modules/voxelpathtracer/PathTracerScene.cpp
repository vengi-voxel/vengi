/**
 * @file
 */

#include "PathTracerScene.h"
#include "VoxelpathtracerShaderConstants.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

namespace voxelpathtracer {

namespace {

constexpr int BrickSize = shader::VoxelpathtracerShaderConstants::getBrickSize();
constexpr int VoxelsPerBrick = BrickSize * BrickSize * BrickSize;
// Knuth multiplicative hash: 2^32 / phi. Mixes the emit-voxel counter so overflow
// replacements are spread instead of clustering at the start of the light list.
constexpr uint32_t KnuthHashMul = 2654435761u;

int findOrAddPalette(core::Buffer<const palette::Palette *> &palettes, const palette::Palette &palette) {
	const uint64_t hash = palette.hash();
	for (size_t i = 0; i < palettes.size(); ++i) {
		if (palettes[i]->hash() == hash) {
			return (int)i;
		}
	}
	palettes.push_back(&palette);
	return (int)palettes.size() - 1;
}

uint32_t packVoxelIndex(int slot, uint8_t colorIndex) {
	return (uint32_t)(slot * palette::PaletteMaxColors + (int)colorIndex + 1);
}

int brickLocalOffset(int lx, int ly, int lz) {
	return (lz * BrickSize + ly) * BrickSize + lx;
}

} // namespace

int pathTracerBrickSize() {
	return BrickSize;
}

int pathTracerVoxelsPerBrick() {
	return VoxelsPerBrick;
}

void freeSparseGpuVolume(SparseGpuVolume &volume) {
	core_free(volume.indirection);
	core_free(volume.bricks);
	core_free(volume.brickCoords);
	volume = SparseGpuVolume{};
}

void freeGpuMaterialAtlas(GpuMaterialAtlas &atlas) {
	core_free(atlas.albedoRgba);
	core_free(atlas.materials);
	atlas = GpuMaterialAtlas{};
}

void collectScenePalettes(const scenegraph::SceneGraph &sceneGraph, core::Buffer<const palette::Palette *> &palettes) {
	palettes.reserve(sceneGraph.size());
	for (const auto &e : sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode &node = e->value;
		if (!node.isAnyModelNode() || !node.visible()) {
			continue;
		}
		const voxel::RawVolume *v = sceneGraph.resolveVolume(node);
		if (v == nullptr || !v->region().isValid()) {
			continue;
		}
		findOrAddPalette(palettes, sceneGraph.resolvePalette(node));
	}
}

bool buildSparseGpuVolume(const scenegraph::SceneGraph &sceneGraph, core::Buffer<const palette::Palette *> &palettes,
						  const glm::ivec3 &worldMin, const glm::ivec3 &volumeSize, int maxTextureAxis,
						  SparseGpuVolume &out, uint8_t *transmissive) {
	freeSparseGpuVolume(out);
	out.coarseSize = glm::ivec3(volumeSize.x / BrickSize, volumeSize.y / BrickSize, volumeSize.z / BrickSize);
	const size_t coarseCount = (size_t)out.coarseSize.x * (size_t)out.coarseSize.y * (size_t)out.coarseSize.z;
	uint8_t *occupancy = (uint8_t *)core_malloc(coarseCount);
	if (occupancy == nullptr) {
		Log::error("Failed to allocate pathtracer brick occupancy");
		return false;
	}
	core_memset(occupancy, 0, coarseCount);

	const voxel::Region packedRegion = voxel::Region::fromSize(volumeSize);
	// Pass 1: mark occupied bricks (parallel-safe: all writers store 255).
	for (const auto &e : sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode &node = e->value;
		if (!node.isAnyModelNode() || !node.visible()) {
			continue;
		}
		const voxel::RawVolume *v = sceneGraph.resolveVolume(node);
		if (v == nullptr) {
			continue;
		}
		const glm::mat4 &world = sceneGraph.worldMatrix(node);
		voxelutil::visitVolumeParallel(
			*v,
			[&](int x, int y, int z, const voxel::Voxel &) {
				const glm::vec3 worldPos =
					glm::vec3(world * glm::vec4((float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f, 1.0f));
				const glm::ivec3 dest = glm::ivec3(glm::floor(worldPos)) - worldMin;
				if (!packedRegion.containsPoint(dest)) {
					return;
				}
				const int cx = dest.x / BrickSize;
				const int cy = dest.y / BrickSize;
				const int cz = dest.z / BrickSize;
				occupancy[volumeOffset(cx, cy, cz, out.coarseSize)] = 255u;
			},
			voxelutil::VisitSolid());
	}

	int occupiedBricks = 0;
	for (size_t i = 0; i < coarseCount; ++i) {
		if (occupancy[i] == 255u) {
			++occupiedBricks;
		}
	}

	const int bricksPerRow = core_max(1, maxTextureAxis / BrickSize);
	const int maxBrickRows = core_max(1, maxTextureAxis / BrickSize);
	const int maxBricks = bricksPerRow * maxBrickRows;
	if (occupiedBricks > maxBricks) {
		out.bricksTruncated = true;
		occupiedBricks = maxBricks;
		Log::warn("Pathtracer brick pool truncated to %i bricks (atlas %ix%i)", maxBricks, bricksPerRow, maxBrickRows);
	}

	uint32_t *brickIndexOfCell = (uint32_t *)core_malloc(coarseCount * sizeof(uint32_t));
	if (brickIndexOfCell == nullptr) {
		core_free(occupancy);
		Log::error("Failed to allocate pathtracer brick index map");
		return false;
	}
	for (size_t i = 0; i < coarseCount; ++i) {
		brickIndexOfCell[i] = ~0u;
	}

	out.brickCount = 0;
	out.brickCoords = (glm::ivec3 *)core_malloc((size_t)core_max(1, occupiedBricks) * sizeof(glm::ivec3));
	out.bricks =
		(uint32_t *)core_malloc((size_t)core_max(1, occupiedBricks) * (size_t)VoxelsPerBrick * sizeof(uint32_t));
	if (out.brickCoords == nullptr || out.bricks == nullptr) {
		core_free(occupancy);
		core_free(brickIndexOfCell);
		freeSparseGpuVolume(out);
		Log::error("Failed to allocate pathtracer brick pool");
		return false;
	}
	core_memset(out.bricks, 0, (size_t)core_max(1, occupiedBricks) * (size_t)VoxelsPerBrick * sizeof(uint32_t));

	for (int cz = 0; cz < out.coarseSize.z; ++cz) {
		for (int cy = 0; cy < out.coarseSize.y; ++cy) {
			for (int cx = 0; cx < out.coarseSize.x; ++cx) {
				const size_t cell = (size_t)volumeOffset(cx, cy, cz, out.coarseSize);
				if (occupancy[cell] != 255u) {
					continue;
				}
				if (out.brickCount >= occupiedBricks) {
					occupancy[cell] = 0u;
					continue;
				}
				brickIndexOfCell[cell] = (uint32_t)out.brickCount;
				out.brickCoords[out.brickCount] = glm::ivec3(cx, cy, cz);
				++out.brickCount;
			}
		}
	}

	// Pass 2: write temporary palette (slot,color) indices; remapped to material payloads later.
	for (const auto &e : sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode &node = e->value;
		if (!node.isAnyModelNode() || !node.visible()) {
			continue;
		}
		const voxel::RawVolume *v = sceneGraph.resolveVolume(node);
		if (v == nullptr) {
			continue;
		}
		const palette::Palette &nodePalette = sceneGraph.resolvePalette(node);
		const int slot = findOrAddPalette(palettes, nodePalette);
		const glm::mat4 &world = sceneGraph.worldMatrix(node);
		voxelutil::visitVolumeParallel(
			*v,
			[&](int x, int y, int z, const voxel::Voxel &voxel) {
				const glm::vec3 worldPos =
					glm::vec3(world * glm::vec4((float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f, 1.0f));
				const glm::ivec3 dest = glm::ivec3(glm::floor(worldPos)) - worldMin;
				if (!packedRegion.containsPoint(dest)) {
					return;
				}
				const int cx = dest.x / BrickSize;
				const int cy = dest.y / BrickSize;
				const int cz = dest.z / BrickSize;
				const uint32_t brickIndex = brickIndexOfCell[volumeOffset(cx, cy, cz, out.coarseSize)];
				if (brickIndex == ~0u) {
					return;
				}
				const uint32_t index = packVoxelIndex(slot, voxel.getColor());
				const int lx = dest.x - cx * BrickSize;
				const int ly = dest.y - cy * BrickSize;
				const int lz = dest.z - cz * BrickSize;
				out.bricks[(size_t)brickIndex * (size_t)VoxelsPerBrick + (size_t)brickLocalOffset(lx, ly, lz)] = index;
				if (index > 0 &&
					(voxel::isTransparent(voxel.getMaterial()) || nodePalette.hasAlpha(voxel.getColor()))) {
					transmissive[index - 1] = 1;
				}
			},
			voxelutil::VisitSolid());
	}

	buildEmptySpaceHierarchy(occupancy, out.coarseSize);

	out.indirection = (uint32_t *)core_malloc(coarseCount * sizeof(uint32_t));
	if (out.indirection == nullptr) {
		core_free(occupancy);
		core_free(brickIndexOfCell);
		freeSparseGpuVolume(out);
		Log::error("Failed to allocate pathtracer indirection grid");
		return false;
	}
	for (size_t i = 0; i < coarseCount; ++i) {
		if (occupancy[i] == 255u) {
			out.indirection[i] = brickIndexOfCell[i] + BrickIdBias;
		} else {
			out.indirection[i] = (uint32_t)occupancy[i];
		}
	}
	core_free(occupancy);
	core_free(brickIndexOfCell);

	if (out.brickCount <= 0) {
		out.bricksPerRow = 1;
		out.atlasSize = glm::ivec3(BrickSize, BrickSize, BrickSize);
	} else {
		out.bricksPerRow = bricksPerRow;
		const int brickRows = (out.brickCount + bricksPerRow - 1) / bricksPerRow;
		out.atlasSize = glm::ivec3(bricksPerRow * BrickSize, brickRows * BrickSize, BrickSize);
	}
	return true;
}

bool buildDenseMaterialAtlas(SparseGpuVolume &volume, const core::Buffer<const palette::Palette *> &palettes,
							 const uint8_t *transmissive, GpuMaterialAtlas &out) {
	freeGpuMaterialAtlas(out);
	const int paletteCount = (int)palettes.size();
	const size_t paletteEntries = (size_t)paletteCount * (size_t)palette::PaletteMaxColors;
	if (paletteEntries == 0u || volume.bricks == nullptr) {
		return true;
	}

	uint8_t *used = (uint8_t *)core_malloc(paletteEntries);
	if (used == nullptr) {
		Log::error("Failed to allocate pathtracer material usage map");
		return false;
	}
	core_memset(used, 0, paletteEntries);

	const size_t brickVoxels = (size_t)core_max(1, volume.brickCount) * (size_t)VoxelsPerBrick;
	for (size_t i = 0; i < brickVoxels; ++i) {
		const uint32_t index = volume.bricks[i];
		if (index == 0u || index > paletteEntries) {
			continue;
		}
		used[index - 1u] = 1u;
	}

	uint32_t *remap = (uint32_t *)core_malloc((paletteEntries + 1u) * sizeof(uint32_t));
	if (remap == nullptr) {
		core_free(used);
		Log::error("Failed to allocate pathtracer material remap");
		return false;
	}
	remap[0] = 0u;
	uint32_t nextId = 1u;
	const uint32_t idMask = (uint32_t)shader::VoxelpathtracerShaderConstants::getVoxelMaterialIdMask();
	for (size_t i = 0; i < paletteEntries; ++i) {
		if (used[i] == 0u) {
			remap[i + 1u] = 0u;
			continue;
		}
		if (nextId > idMask) {
			core_free(used);
			core_free(remap);
			Log::error("Pathtracer material count exceeds %u (14-bit payload field)", idMask);
			return false;
		}
		remap[i + 1u] = nextId++;
	}
	core_free(used);

	out.materialCount = (int)(nextId - 1u);
	if (out.materialCount <= 0) {
		core_free(remap);
		return true;
	}

	out.albedoRgba = (uint8_t *)core_malloc((size_t)out.materialCount * 4u);
	out.materials = (GpuMaterial *)core_malloc((size_t)out.materialCount * sizeof(GpuMaterial));
	if (out.albedoRgba == nullptr || out.materials == nullptr) {
		core_free(remap);
		freeGpuMaterialAtlas(out);
		Log::error("Failed to allocate pathtracer dense material atlas");
		return false;
	}
	core_memset(out.albedoRgba, 0, (size_t)out.materialCount * 4u);
	core_memset(out.materials, 0, (size_t)out.materialCount * sizeof(GpuMaterial));

	for (size_t oldIndex = 1; oldIndex <= paletteEntries; ++oldIndex) {
		const uint32_t denseId = remap[oldIndex];
		if (denseId == 0u) {
			continue;
		}
		const int slot = (int)((oldIndex - 1u) / (uint32_t)palette::PaletteMaxColors);
		const int color = (int)((oldIndex - 1u) % (uint32_t)palette::PaletteMaxColors);
		const palette::Palette &p = *palettes[slot];
		const color::RGBA c = p.color((uint8_t)color);
		uint8_t *rgba = out.albedoRgba + (size_t)(denseId - 1u) * 4u;
		rgba[0] = c.r;
		rgba[1] = c.g;
		rgba[2] = c.b;
		rgba[3] = c.a;
		out.materials[denseId - 1u] =
			packGpuMaterial(p.material((uint8_t)color), c.a, transmissive[oldIndex - 1u] != 0);
	}

	for (size_t i = 0; i < brickVoxels; ++i) {
		const uint32_t oldIndex = volume.bricks[i];
		if (oldIndex == 0u || oldIndex > paletteEntries) {
			volume.bricks[i] = 0u;
			continue;
		}
		const uint32_t denseId = remap[oldIndex];
		if (denseId == 0u) {
			volume.bricks[i] = 0u;
			continue;
		}
		const GpuMaterial &mat = out.materials[denseId - 1u];
		volume.bricks[i] = packVoxelPayload(denseId, mat.transmits > 0.0f, mat.emission > 0.0f);
	}
	core_free(remap);
	return true;
}

void gatherSceneEmitLights(const SparseGpuVolume &volume, int materialCount, const GpuMaterial *materials,
						   const uint8_t *albedoRgba, GpuEmitLight *emitLights, int &emitCount,
						   float &emitterLuminance) {
	emitCount = 0;
	emitterLuminance = 0.0f;
	if (emitLights == nullptr || volume.bricks == nullptr || volume.brickCoords == nullptr || materials == nullptr ||
		albedoRgba == nullptr) {
		return;
	}
	core_memset(emitLights, 0, (size_t)MaxEmitLights * sizeof(GpuEmitLight));
	int seen = 0;
	for (int brickIndex = 0; brickIndex < volume.brickCount; ++brickIndex) {
		const glm::ivec3 brick = volume.brickCoords[brickIndex];
		const uint32_t *brickVoxels = volume.bricks + (size_t)brickIndex * (size_t)VoxelsPerBrick;
		for (int lz = 0; lz < BrickSize; ++lz) {
			for (int ly = 0; ly < BrickSize; ++ly) {
				for (int lx = 0; lx < BrickSize; ++lx) {
					const uint32_t payload = brickVoxels[brickLocalOffset(lx, ly, lz)];
					if (!voxelPayloadEmits(payload)) {
						continue;
					}
					const uint32_t mid = voxelMaterialId(payload);
					if (mid == 0u || (int)mid > materialCount) {
						continue;
					}
					const size_t materialIndex = (size_t)(mid - 1u);
					if (materials[materialIndex].emission <= 0.0f) {
						continue;
					}
					++seen;
					int dest = emitCount;
					if (emitCount < MaxEmitLights) {
						++emitCount;
					} else {
						dest = (int)((uint32_t)(seen * KnuthHashMul) % (uint32_t)seen);
						if (dest >= MaxEmitLights) {
							continue;
						}
					}
					const uint8_t *rgba = albedoRgba + materialIndex * 4u;
					const glm::vec3 srgb = glm::vec3(rgba[0], rgba[1], rgba[2]) / 255.0f;
					const glm::ivec3 cell(brick.x * BrickSize + lx, brick.y * BrickSize + ly, brick.z * BrickSize + lz);
					emitLights[dest] = packGpuEmitLight(cell, srgb, materials[materialIndex]);
				}
			}
		}
	}
	emitterLuminance = buildEmitterAliasTable(emitLights, emitCount);
}

} // namespace voxelpathtracer
