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
	/**
	 * @brief Recursively go left until you leave the valid area of the volume.
	 */
	void goLeft(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection, int& cnt) const;
	/**
	 * @brief Recursively go right until you leave the valid area of the volume.
	 */
	void goRight(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection, int& cnt) const;
	/**
	 * @brief Recursively go up until you leave the valid area of the volume.
	 */
	void goUp(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection, int& cnt) const;
	/**
	 * @brief Recursively go down until you leave the valid area of the volume.
	 */
	void goDown(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection, int& cnt) const;
	/**
	 * @brief Recursively go into all six directions a position until you leave the valid region of the volume.
	 */
	void goSixDirections(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection, const voxel::Voxel voxel, int& cnt) const;
	bool sixDirectionsExecute(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection, const voxel::Voxel& voxel, int& cnt) const;

	virtual int execute(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection);

public:
	virtual ~Select();

	/**
	 * @brief Allows the selection methods to cleanup if they have a state.
	 */
	virtual void unselect() {}

	virtual int execute(const voxel::RawVolume *model, voxel::RawVolume *selection, const glm::ivec3& pos);
};

}
}
