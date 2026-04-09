/**
 * @file
 */

#pragma once

#include "app/ForParallel.h"
#include "voxel/Voxel.h"
#include "voxel/VoxelSampling.h"
#include <float.h>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace voxel {

template<class Volume>
inline bool setVoxels(Volume &volume, int x, int z, const Voxel *voxels, int amount) {
	typename Volume::Sampler sampler(volume);
	sampler.setPosition(x, 0, z);
	for (int y = 0; y < amount; ++y) {
		sampler.setVoxel(voxels[y]);
		sampler.movePositiveY();
	}
	return true;
}

template<class Volume>
inline bool setVoxels(Volume &volume, int x, int y, int z, int nx, int nz, const Voxel *voxels, int amount) {
	app::for_parallel(0, nz, [nx, amount, &volume, &voxels, x, y, z](int start, int end) {
		typename Volume::Sampler sampler(volume);
		sampler.setPosition(x, y, z + start);
		for (int k = start; k < end; ++k) {
			typename Volume::Sampler samplerZ(sampler);
			for (int ny = 0; ny < amount; ++ny) {
				typename Volume::Sampler samplerY(samplerZ);
				for (int j = 0; j < nx; ++j) {
					samplerY.setVoxel(voxels[ny]);
					samplerY.movePositiveX();
				}
				samplerZ.movePositiveY();
			}
			sampler.movePositiveZ();
		}
	});
	return true;
}

/**
 * @brief Nearest-neighbor sampling: find the closest non-air voxel in a 3x3x3 neighborhood.
 *
 * First checks the rounded position. If solid, returns it directly.
 * If air, searches all 26 neighbors and returns the nearest non-air voxel
 * regardless of distance. This prevents gaps at surface boundaries where
 * rounding lands on the air side.
 */
template<class Sampler>
static voxel::Voxel sampleNearest(Sampler &sampler, const glm::vec3 &pos) {
	const glm::ivec3 rounded = glm::ivec3(glm::round(pos));
	if (sampler.setPosition(rounded) && !voxel::isAir(sampler.voxel().getMaterial())) {
		return sampler.voxel();
	}

	// TODO: use the sampler peek functions and make the sampler const
	voxel::Voxel best;
	float bestDistSq = FLT_MAX;
	for (int dz = -1; dz <= 1; ++dz) {
		for (int dy = -1; dy <= 1; ++dy) {
			for (int dx = -1; dx <= 1; ++dx) {
				const glm::ivec3 neighbor(rounded.x + dx, rounded.y + dy, rounded.z + dz);
				if (!sampler.setPosition(neighbor)) {
					continue;
				}
				const voxel::Voxel &v = sampler.voxel();
				if (voxel::isAir(v.getMaterial())) {
					continue;
				}
				const glm::vec3 d = glm::vec3(neighbor) - pos;
				const float distSq = glm::dot(d, d);
				if (distSq < bestDistSq) {
					bestDistSq = distSq;
					best = v;
				}
			}
		}
	}
	return best;
}

// Generic trilinear sampling
template<class Sampler>
static voxel::Voxel sampleTrilinear(Sampler &sampler, const glm::vec3 &pos) {
	const glm::ivec3 base = glm::ivec3(glm::round(pos));
	if (!sampler.setPosition(base)) {
		return voxel::Voxel();
	}

	const glm::vec3 frac = pos - glm::vec3(base);

	// Fetch 8 corner voxels via peek APIs relative to current sampler position
	const voxel::Voxel v000 = sampler.peekVoxel0px0py0pz();
	const voxel::Voxel v100 = sampler.peekVoxel1px0py0pz();
	const voxel::Voxel v010 = sampler.peekVoxel0px1py0pz();
	const voxel::Voxel v110 = sampler.peekVoxel1px1py0pz();
	const voxel::Voxel v001 = sampler.peekVoxel0px0py1pz();
	const voxel::Voxel v101 = sampler.peekVoxel1px0py1pz();
	const voxel::Voxel v011 = sampler.peekVoxel0px1py1pz();
	const voxel::Voxel v111 = sampler.peekVoxel1px1py1pz();

	// Compute trilinear weights
	const float w000 = (1.0f - frac.x) * (1.0f - frac.y) * (1.0f - frac.z);
	const float w100 = frac.x * (1.0f - frac.y) * (1.0f - frac.z);
	const float w010 = (1.0f - frac.x) * frac.y * (1.0f - frac.z);
	const float w110 = frac.x * frac.y * (1.0f - frac.z);
	const float w001 = (1.0f - frac.x) * (1.0f - frac.y) * frac.z;
	const float w101 = frac.x * (1.0f - frac.y) * frac.z;
	const float w011 = (1.0f - frac.x) * frac.y * frac.z;
	const float w111 = frac.x * frac.y * frac.z;

	// Select voxel with maximum contribution (discrete nearest in trilinear space)
	float maxWeight = w000;
	voxel::Voxel result = v000;
	if (w100 > maxWeight) {
		maxWeight = w100;
		result = v100;
	}
	if (w010 > maxWeight) {
		maxWeight = w010;
		result = v010;
	}
	if (w110 > maxWeight) {
		maxWeight = w110;
		result = v110;
	}
	if (w001 > maxWeight) {
		maxWeight = w001;
		result = v001;
	}
	if (w101 > maxWeight) {
		maxWeight = w101;
		result = v101;
	}
	if (w011 > maxWeight) {
		maxWeight = w011;
		result = v011;
	}
	if (w111 > maxWeight) {
		maxWeight = w111;
		result = v111;
	}

	return result;
}

/**
 * @brief Cubic sampling: check a 4x4x4 neighborhood centered on the position.
 * Each voxel votes with a weight inversely proportional to its distance.
 * The voxel color with the highest total weight wins.
 */
template<class Sampler>
static voxel::Voxel sampleCubic(const Sampler &sampler, const glm::vec3 &pos) {
	static constexpr int HalfExtent = 2;
	static constexpr float MaxCubicDistanceSq = 3.5f * 3.5f;
	static constexpr int MaxMaterials = 64; // 4x4x4 worst case

	const glm::ivec3 center = glm::ivec3(glm::round(pos));

	struct MaterialWeight {
		voxel::Voxel voxel;
		float weight;
	};
	MaterialWeight materials[MaxMaterials] = {};
	int materialCount = 0;

	Sampler zSampler = sampler;
	zSampler.setPosition(center.x - HalfExtent + 1, center.y - HalfExtent + 1, center.z - HalfExtent + 1);
	for (int dz = -HalfExtent + 1; dz <= HalfExtent; ++dz) {
		Sampler ySampler = zSampler;
		for (int dy = -HalfExtent + 1; dy <= HalfExtent; ++dy) {
			Sampler xSampler = ySampler;
			for (int dx = -HalfExtent + 1; dx <= HalfExtent; ++dx) {
				if (!xSampler.currentPositionValid()) {
					xSampler.movePositiveX();
					continue;
				}
				const voxel::Voxel v = xSampler.voxel();
				xSampler.movePositiveX();
				if (voxel::isAir(v.getMaterial())) {
					continue;
				}
				const glm::vec3 d = glm::vec3(xSampler.position()) - pos;
				const float dist2 = glm::dot(d, d);
				if (dist2 > MaxCubicDistanceSq) {
					continue;
				}
				const float weight = 1.0f / (dist2 + 0.001f);
				const uint8_t color = v.getColor();
				bool found = false;
				for (int mi = 0; mi < materialCount; ++mi) {
					if (materials[mi].voxel.getColor() == color) {
						materials[mi].weight += weight;
						found = true;
						break;
					}
				}
				if (!found && materialCount < MaxMaterials) {
					materials[materialCount++] = {v, weight};
				}
			}
			ySampler.movePositiveY();
		}
		zSampler.movePositiveZ();
	}

	if (materialCount == 0) {
		return voxel::Voxel();
	}

	int bestIdx = 0;
	for (int mi = 1; mi < materialCount; ++mi) {
		if (materials[mi].weight > materials[bestIdx].weight) {
			bestIdx = mi;
		}
	}
	return materials[bestIdx].voxel;
}

template<class Sampler>
voxel::Voxel sampleVoxel(Sampler &sampler, VoxelSampling sampling, const glm::vec3 &srcPos) {
	switch (sampling) {
	case VoxelSampling::Linear:
		return sampleTrilinear(sampler, srcPos);
	case VoxelSampling::Cubic:
		return sampleCubic(sampler, srcPos);
	case VoxelSampling::Nearest:
	default:
		return sampleNearest(sampler, srcPos);
	}
}

} // namespace voxel
