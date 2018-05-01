/**
 * @file
 */

#pragma once

#include "voxel/polyvox/RawVolume.h"

namespace voxedit {
namespace selections {

#define SelectionSingleton(Class) static Class& get() { static Class instance; return instance; }

class Select {
protected:
	bool sixDirectionsExecute(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection, const voxel::Voxel& voxel, int& cnt) const;
	void goLeft(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection, int& cnt) const;
	void goRight(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection, int& cnt) const;
	void goUp(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection, int& cnt) const;
	void goDown(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection, int& cnt) const;
	void goSixDirections(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection, const voxel::Voxel voxel, int& cnt) const;

	virtual int execute(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection);

public:
	virtual ~Select();

	virtual void unselect() {}

	virtual int execute(const voxel::RawVolume *model, voxel::RawVolume *selection, const glm::ivec3& pos);
};

}
}
