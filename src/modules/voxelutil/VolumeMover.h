/**
 * @file
 */

#pragma once

#include "app/ForParallel.h"
#include "core/concurrent/Atomic.h"
#include "voxel/Voxel.h"
#include "voxel/Region.h"
#include "core/Trace.h"

namespace voxelutil {

template<class Volume1, class Volume2>
int moveVolume(Volume1* destination, const Volume2* source, const glm::ivec3& offsets) {
	core_trace_scoped(MoveVolume);
	core::AtomicInt cnt = 0;

	const voxel::Region& destReg = destination->region();
	const voxel::Region& sourceReg = source->region();

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
