#pragma once

#include "voxel/polyvox/RawVolume.h"

namespace voxedit {
namespace selections {

#define SelectionSingleton(Class) static Class& get() { static Class instance; return instance; }

class Select {
protected:
	bool sixDirectionsExecute(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection, const voxel::Voxel& voxel) const;
	void goLeft(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection) const;
	void goRight(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection) const;
	void goUp(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection) const;
	void goDown(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection) const;
	void goSixDirections(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection, const voxel::Voxel voxel) const;

	virtual bool execute(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection) const;

public:
	virtual ~Select();

	virtual bool execute(const voxel::RawVolume *model, voxel::RawVolume *selection, const glm::ivec3& pos) const;
};

}
}
