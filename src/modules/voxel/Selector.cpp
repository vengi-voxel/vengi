/**
 * @file
 */

#include "Selector.h"

namespace voxel {

bool Selector::executeStep(voxel::RawVolume::Sampler& sampler, SelectorCallback& callback, voxel::FaceNames face, Selector::Visited& set) const {
	if (!sampler.currentPositionValid()) {
		return false;
	}
	const glm::ivec3& pos = sampler.position();
	if (set.find(pos) != set.end()) {
		return false;
	}
	set.insert(pos);
	if (callback(sampler, face)) {
		executeWalk(sampler, callback, set);
		return true;
	}
	return false;
}

void Selector::executeWalk(voxel::RawVolume::Sampler& sampler, SelectorCallback& callback, Selector::Visited& set) const {
	const glm::ivec3 pos = sampler.position();

	sampler.moveNegativeX();
	executeStep(sampler, callback, voxel::FaceNames::NegativeX, set);
	sampler.setPosition(pos);

	sampler.moveNegativeY();
	executeStep(sampler, callback, voxel::FaceNames::NegativeY, set);
	sampler.setPosition(pos);

	sampler.moveNegativeZ();
	executeStep(sampler, callback, voxel::FaceNames::NegativeZ, set);
	sampler.setPosition(pos);

	sampler.movePositiveX();
	executeStep(sampler, callback, voxel::FaceNames::PositiveX, set);
	sampler.setPosition(pos);

	sampler.movePositiveY();
	executeStep(sampler, callback, voxel::FaceNames::PositiveY, set);
	sampler.setPosition(pos);

	sampler.movePositiveZ();
	executeStep(sampler, callback, voxel::FaceNames::PositiveZ, set);
	sampler.setPosition(pos);
}

void Selector::walk(voxel::RawVolume::Sampler& sampler, SelectorCallback& callback) const {
	Visited set;
	executeWalk(sampler, callback, set);
}

}
