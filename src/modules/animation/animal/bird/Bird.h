/**
 * @file
 */

#pragma once

#include "animation/AnimationEntity.h"
#include "BirdSkeleton.h"
#include <stdint.h>

namespace animation {

/**
 * @brief Handles the loading and creation of the meshes and the bones regarding
 * the given @c BirdSettings
 * @ingroup Animation
 */
class Bird : public AnimationEntity {
protected:
	BirdSkeleton _skeleton;
	BirdSkeletonAttribute _attributes;
public:
	bool initMesh(const AnimationCachePtr& cache) override;
	bool initSettings(const std::string& luaString) override;
	void update(uint64_t dt, const attrib::ShadowAttributes& attrib) override;
	const Skeleton& skeleton() const override;
	BirdSkeletonAttribute& skeletonAttributes() override;
	const BirdSkeletonAttribute& skeletonAttributes() const;
};

inline const Skeleton& Bird::skeleton() const {
	return _skeleton;
}

inline BirdSkeletonAttribute& Bird::skeletonAttributes() {
	return _attributes;
}

inline const BirdSkeletonAttribute& Bird::skeletonAttributes() const {
	return _attributes;
}

}
