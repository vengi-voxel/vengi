/**
 * @file
 */

#pragma once

#include "voxel/RawVolume.h"

namespace voxel {

/** Select storage once for bulk traversal. Wrappers retain their own samplers. */
template<class Volume, class Func>
CORE_FORCE_INLINE auto withVolumeSampler(Volume &volume, Func &&func) {
	typename Volume::Sampler sampler(volume);
	return func(sampler);
}

template<class Func>
CORE_FORCE_INLINE auto withVolumeSampler(const RawVolume &volume, Func &&func) {
	if (volume.isContiguous()) {
		RawVolume::DenseSampler sampler(volume);
		return func(sampler);
	}
	RawVolume::Sampler sampler(volume);
	return func(sampler);
}

template<class Func>
CORE_FORCE_INLINE auto withVolumeSampler(RawVolume &volume, Func &&func) {
	return withVolumeSampler((const RawVolume &)volume, func);
}

} // namespace voxel
