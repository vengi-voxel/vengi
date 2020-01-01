/**
 * @file
 */

#pragma once

#include "voxel/RawVolume.h"
#include "core/Common.h"
#include "core/Trace.h"

namespace voxel {

template<class Volume1, class Volume2>
int moveVolume(Volume1* destination, const Volume2* source, const glm::ivec3& offsets, const Voxel& skipVoxel = voxel::Voxel()) {
	core_trace_scoped(MoveVolume);
	int cnt = 0;

	const voxel::Region& destReg = destination->region();
	const voxel::Region& sourceReg = source->region();

	for (int32_t z = sourceReg.getLowerZ(); z <= sourceReg.getUpperZ(); ++z) {
		const int destZ = destReg.getLowerZ() + z - sourceReg.getLowerZ() + offsets.z;
		for (int32_t y = sourceReg.getLowerY(); y <= sourceReg.getUpperY(); ++y) {
			const int destY = destReg.getLowerY() + y - sourceReg.getLowerY() + offsets.y;
			for (int32_t x = sourceReg.getLowerX(); x <= sourceReg.getUpperX(); ++x) {
				const int destX = destReg.getLowerX() + x - sourceReg.getLowerX() + offsets.x;
				const Voxel& voxel = source->voxel(x, y, z);
				if (voxel == skipVoxel) {
					continue;
				}
				destination->setVoxel(destX, destY, destZ, voxel);
			}
		}
	}
	return cnt;
}

}
