/**
 * @file
 */

#pragma once

#include "app/Async.h"
#include "voxel/Voxel.h"

namespace voxel {

template<class Volume>
inline bool setVoxels(Volume &volume, int x, int z, const Voxel* voxels, int amount) {
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

} // namespace voxel
