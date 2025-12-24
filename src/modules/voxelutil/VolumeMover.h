/**
 * @file
 */

#pragma once

#include "app/Async.h"
#include "voxel/Voxel.h"
#include "voxel/Region.h"
#include "core/Trace.h"

namespace voxelutil {

template<class Volume1, class Volume2>
int moveVolume(Volume1* destination, const Volume2* source, const glm::ivec3& offsets) {
	core_trace_scoped(MoveVolume);
	int cnt = 0;

	const voxel::Region& destReg = destination->region();
	const voxel::Region& sourceReg = source->region();

	app::for_parallel(sourceReg.getLowerZ(), sourceReg.getUpperZ() + 1, [&destination, &source, &destReg, &sourceReg, &offsets, &cnt] (int start, int end) {
		typename Volume1::Sampler destSampler(destination);
		typename Volume2::Sampler sourceSampler(source);
		for (int32_t z = start; z < end; ++z) {
			sourceSampler.setPosition(sourceReg.getLowerX(), sourceReg.getLowerY(), z);
			destSampler.setPosition(destReg.getLowerX() + offsets.x, destReg.getLowerY() + offsets.y, destReg.getLowerZ() + offsets.z);
			for (int32_t y = sourceReg.getLowerY(); y <= sourceReg.getUpperY(); ++y) {
				sourceSampler.setPosition(sourceReg.getLowerX(), y, z);
				destSampler.setPosition(destReg.getLowerX() + offsets.x, y + offsets.y, destReg.getLowerZ() + offsets.z);
				for (int32_t x = sourceReg.getLowerX(); x <= sourceReg.getUpperX(); ++x) {
					const voxel::Voxel& voxel = sourceSampler.voxel();
					if (voxel::isAir(voxel.getMaterial())) {
						sourceSampler.movePositiveX();
						destSampler.movePositiveX();
						continue;
					}
					destSampler.setVoxel(voxel);
					++cnt;
					sourceSampler.movePositiveX();
					destSampler.movePositiveX();
				}
				sourceSampler.movePositiveY();
				destSampler.movePositiveY();
			}
			sourceSampler.movePositiveZ();
			destSampler.movePositiveZ();
		}
	});
	return cnt;
}

}
