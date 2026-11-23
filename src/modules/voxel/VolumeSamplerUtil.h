/**
 * @file
 */

#pragma once

#include "app/ForParallel.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxel/VoxelSampling.h"
#include <float.h>
#include "math/Vec3i.h"
#include "math/Vec3f.h"

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

template<>
inline bool setVoxels<RawVolume>(RawVolume &volume, int x, int y, int z, int nx, int nz, const Voxel *voxels,
								 int amount) {
	if (volume.isSparse()) {
		app::for_parallel(0, nz, [nx, amount, &volume, voxels, x, y, z](int start, int end) {
			for (int k = start; k < end; ++k) {
				for (int ny = 0; ny < amount; ++ny) {
					volume.setVoxelsRow(x, y + ny, z + k, nx, voxels[ny]);
				}
			}
		});
		return true;
	}
	app::for_parallel(0, nz, [nx, amount, &volume, voxels, x, y, z](int start, int end) {
		RawVolume::DenseSampler sampler(volume);
		sampler.setPosition(x, y, z + start);
		for (int k = start; k < end; ++k) {
			RawVolume::DenseSampler samplerZ(sampler);
			for (int ny = 0; ny < amount; ++ny) {
				RawVolume::DenseSampler samplerY(samplerZ);
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
 * @brief Scan the 3x3x3 neighborhood for the closest non-air voxel.
 * Kept out-of-line so the solid hit path in sampleNearest stays tiny.
 */
template<class Sampler>
CORE_NO_INLINE static voxel::Voxel sampleNearestNeighborhood(const Sampler &sampler,
																		const math::Vec3f &pos,
																		const math::Vec3i &rounded) {
	voxel::Voxel best;
	float bestDistSq = FLT_MAX;

	const auto consider = [&](const voxel::Voxel &v, int dx, int dy, int dz) {
		if (voxel::isAir(v.getMaterial())) {
			return;
		}
		const float fx = (float)(rounded.x + dx) - pos.x;
		const float fy = (float)(rounded.y + dy) - pos.y;
		const float fz = (float)(rounded.z + dz) - pos.z;
		const float distSq = fx * fx + fy * fy + fz * fz;
		if (distSq < bestDistSq) {
			bestDistSq = distSq;
			best = v;
		}
	};

	// setPosition(rounded) already ran; peeks are relative to that cell.
	if (!sampler.currentPositionValid()) {
		consider(sampler.peekVoxel0px0py0pz(), 0, 0, 0);
	}
	consider(sampler.peekVoxel1nx1ny1nz(), -1, -1, -1);
	consider(sampler.peekVoxel1nx1ny0pz(), -1, -1, 0);
	consider(sampler.peekVoxel1nx1ny1pz(), -1, -1, 1);
	consider(sampler.peekVoxel1nx0py1nz(), -1, 0, -1);
	consider(sampler.peekVoxel1nx0py0pz(), -1, 0, 0);
	consider(sampler.peekVoxel1nx0py1pz(), -1, 0, 1);
	consider(sampler.peekVoxel1nx1py1nz(), -1, 1, -1);
	consider(sampler.peekVoxel1nx1py0pz(), -1, 1, 0);
	consider(sampler.peekVoxel1nx1py1pz(), -1, 1, 1);
	consider(sampler.peekVoxel0px1ny1nz(), 0, -1, -1);
	consider(sampler.peekVoxel0px1ny0pz(), 0, -1, 0);
	consider(sampler.peekVoxel0px1ny1pz(), 0, -1, 1);
	consider(sampler.peekVoxel0px0py1nz(), 0, 0, -1);
	consider(sampler.peekVoxel0px0py1pz(), 0, 0, 1);
	consider(sampler.peekVoxel0px1py1nz(), 0, 1, -1);
	consider(sampler.peekVoxel0px1py0pz(), 0, 1, 0);
	consider(sampler.peekVoxel0px1py1pz(), 0, 1, 1);
	consider(sampler.peekVoxel1px1ny1nz(), 1, -1, -1);
	consider(sampler.peekVoxel1px1ny0pz(), 1, -1, 0);
	consider(sampler.peekVoxel1px1ny1pz(), 1, -1, 1);
	consider(sampler.peekVoxel1px0py1nz(), 1, 0, -1);
	consider(sampler.peekVoxel1px0py0pz(), 1, 0, 0);
	consider(sampler.peekVoxel1px0py1pz(), 1, 0, 1);
	consider(sampler.peekVoxel1px1py1nz(), 1, 1, -1);
	consider(sampler.peekVoxel1px1py0pz(), 1, 1, 0);
	consider(sampler.peekVoxel1px1py1pz(), 1, 1, 1);
	return best;
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
static voxel::Voxel sampleNearest(Sampler &sampler, const math::Vec3f &pos) {
	const math::Vec3i rounded = math::Vec3i(math::round(pos));
	if (sampler.setPosition(rounded) && !voxel::isAir(sampler.voxel().getMaterial())) {
		return sampler.voxel();
	}
	return sampleNearestNeighborhood(sampler, pos, rounded);
}

// Generic trilinear sampling
template<class Sampler>
static voxel::Voxel sampleTrilinear(Sampler &sampler, const math::Vec3f &pos) {
	const math::Vec3i base = math::Vec3i(math::round(pos));
	if (!sampler.setPosition(base)) {
		return voxel::Voxel();
	}

	const math::Vec3f frac = pos - math::Vec3f(base);

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
static voxel::Voxel sampleCubic(const Sampler &sampler, const math::Vec3f &pos) {
	static constexpr int HalfExtent = 2;
	static constexpr float MaxCubicDistanceSq = 3.5f * 3.5f;

	const math::Vec3i center = math::Vec3i(math::round(pos));
	const int x0 = center.x - HalfExtent + 1;
	const int y0 = center.y - HalfExtent + 1;
	const int z0 = center.z - HalfExtent + 1;
	const int x1 = center.x + HalfExtent;
	const int y1 = center.y + HalfExtent;
	const int z1 = center.z + HalfExtent;

	// Color-indexed weights: O(1) per voxel, stack-local (no per-instance cost).
	float weights[256] = {};
	voxel::Voxel bestVoxel;
	float bestWeight = 0.0f;

	Sampler zSampler = sampler;
	zSampler.setPosition(x0, y0, z0);
	for (int z = z0; z <= z1; ++z) {
		Sampler ySampler = zSampler;
		for (int y = y0; y <= y1; ++y) {
			Sampler xSampler = ySampler;
			for (int x = x0; x <= x1; ++x) {
				if (xSampler.currentPositionValid()) {
					const voxel::Voxel &v = xSampler.voxel();
					if (!voxel::isAir(v.getMaterial())) {
						const float dx = (float)x - pos.x;
						const float dy = (float)y - pos.y;
						const float dz = (float)z - pos.z;
						const float dist2 = dx * dx + dy * dy + dz * dz;
						if (dist2 <= MaxCubicDistanceSq) {
							const float weight = 1.0f / (dist2 + 0.001f);
							const uint8_t color = v.getColor();
							weights[color] += weight;
							if (weights[color] > bestWeight) {
								bestWeight = weights[color];
								bestVoxel = v;
							}
						}
					}
				}
				xSampler.movePositiveX();
			}
			ySampler.movePositiveY();
		}
		zSampler.movePositiveZ();
	}

	return bestVoxel;
}

template<class Sampler>
voxel::Voxel sampleVoxel(Sampler &sampler, VoxelSampling sampling, const math::Vec3f &srcPos) {
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
