/**
 * @file
 */

#pragma once

#include "app/ForParallel.h"
#include "core/TraceScoped.h"
#include "core/concurrent/Atomic.h"
#include "voxel/RawVolume.h"
#include "voxel/VolumeSamplerDispatch.h"
#include "voxel/VolumeTileDispatch.h"
#include "voxel/Voxel.h"
#include "voxel/Region.h"
#include <type_traits>

namespace voxelutil {

template<class Volume1, class Volume2>
int moveVolume(Volume1* destination, const Volume2* source, const glm::ivec3& offsets) {
	core_trace_scoped(MoveVolume);
	core::AtomicInt cnt = 0;

	const voxel::Region& destReg = destination->region();
	const voxel::Region& sourceReg = source->region();

	if constexpr (std::is_same_v<Volume1, voxel::RawVolume>) {
		if (destination->isSparse()) {
			const glm::ivec3 delta(destReg.getLowerX() + offsets.x - sourceReg.getLowerX(),
								   destReg.getLowerY() + offsets.y - sourceReg.getLowerY(), offsets.z);
			const voxel::Region targetReg(sourceReg.getLowerCorner() + delta, sourceReg.getUpperCorner() + delta);
			voxel::forEachTileWriteParallel(*destination, targetReg, [&](voxel::ParallelTileWriter &tile) {
				const voxel::Region &tr = tile.region();
				const voxel::Region srcTile(tr.getLowerCorner() - delta, tr.getUpperCorner() - delta);
				if constexpr (std::is_same_v<Volume2, voxel::RawVolume>) {
					if (source->isSparseAirDefault() && !source->hasOccupiedVoxelsInRegion(srcTile)) {
						return;
					}
				}
				int localCnt = 0;
				voxel::withVolumeSampler(*source, [&](auto &srcSampler) {
					srcSampler.setPosition(srcTile.getLowerCorner());
					for (int32_t z = srcTile.getLowerZ(); z <= srcTile.getUpperZ(); ++z) {
						const int32_t dz = z + delta.z;
						auto srcSamplerY = srcSampler;
						for (int32_t y = srcTile.getLowerY(); y <= srcTile.getUpperY(); ++y) {
							const int32_t dy = y + delta.y;
							auto srcSamplerX = srcSamplerY;
							for (int32_t x = srcTile.getLowerX(); x <= srcTile.getUpperX(); ++x) {
								const voxel::Voxel &vox = srcSamplerX.voxel();
								if (!voxel::isAir(vox.getMaterial())) {
									tile.setVoxel(x + delta.x, dy, dz, vox);
									++localCnt;
								}
								srcSamplerX.movePositiveX();
							}
							srcSamplerY.movePositiveY();
						}
						srcSampler.movePositiveZ();
					}
				});
				if (localCnt > 0) {
					cnt.increment(localCnt);
				}
			});
			return cnt;
		}
	}

	app::for_parallel(sourceReg.getLowerZ(), sourceReg.getUpperZ() + 1, [&destination, &source, &destReg, &sourceReg, &offsets, &cnt] (int start, int end) {
		typename Volume1::Sampler destSampler(destination);
		typename Volume2::Sampler sourceSampler(source);
		for (int32_t z = start; z < end; ++z) {
			const int destZ = z + offsets.z;
			sourceSampler.setPosition(sourceReg.getLowerX(), sourceReg.getLowerY(), z);
			destSampler.setPosition(destReg.getLowerX() + offsets.x, destReg.getLowerY() + offsets.y, destZ);
			for (int32_t y = sourceReg.getLowerY(); y <= sourceReg.getUpperY(); ++y) {
				typename Volume2::Sampler sourceSamplerX = sourceSampler;
				typename Volume1::Sampler destSamplerX = destSampler;
				for (int32_t x = sourceReg.getLowerX(); x <= sourceReg.getUpperX(); ++x) {
					const voxel::Voxel& voxel = sourceSamplerX.voxel();
					if (!voxel::isAir(voxel.getMaterial())) {
						destSamplerX.setVoxel(voxel);
						++cnt;
					}
					sourceSamplerX.movePositiveX();
					destSamplerX.movePositiveX();
				}
				sourceSampler.movePositiveY();
				destSampler.movePositiveY();
			}
		}
	});
	return cnt;
}

}
