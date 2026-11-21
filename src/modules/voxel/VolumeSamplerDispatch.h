/**
 * @file
 */

#pragma once

#include "voxel/RawVolume.h"

namespace voxel {

template<typename Volume, typename Func>
inline decltype(auto) withVolumeSampler(Volume &volume, Func &&fn) {
	typename Volume::Sampler sampler(volume);
	return fn(sampler);
}

template<typename Func>
inline decltype(auto) withVolumeSampler(RawVolume &volume, Func &&fn) {
	if (volume.isContiguous()) {
		RawVolume::DenseSampler sampler(volume);
		return fn(sampler);
	}
	RawVolume::Sampler sampler(volume);
	return fn(sampler);
}

template<typename Func>
inline decltype(auto) withVolumeSampler(const RawVolume &volume, Func &&fn) {
	if (volume.isContiguous()) {
		RawVolume::DenseSampler sampler(volume);
		return fn(sampler);
	}
	RawVolume::Sampler sampler(volume);
	return fn(sampler);
}

} // namespace voxel
