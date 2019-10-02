/**
 * @file
 */

#pragma once

#include "Bone.h"
#include "BoneId.h"
#include "SkeletonAttribute.h"
#include "core/GLM.h"
#include "core/Assert.h"
#include "core/Common.h"
#include <vector>

namespace animation {

/**
 * @brief The bones of the @c Character
 */
class CharacterSkeleton {
public:
	Bone _bones[std::enum_value(BoneId::Max)];
public:
	const Bone& bone(BoneId id) const;
	Bone& bone(BoneId id);
	/**
	 * @brief Calculate the skeleton bones matrices which indices are assigned to the
	 * mesh vertices to perform the skeletal animation.
	 */
	void update(glm::mat4 (&bones)[16]) const;
	/**
	 * @brief Linear interpolate from one skeletal animation state to a new one.
	 */
	void lerp(const CharacterSkeleton& previous, float dt);

	Bone& handBone(BoneId id, const SkeletonAttribute& skeletonAttr);
	Bone& footBone(BoneId id, const SkeletonAttribute& skeletonAttr);
	Bone& shoulderBone(BoneId id, const SkeletonAttribute& skeletonAttr, const glm::quat& orientation = glm::quat_identity<float, glm::defaultp>());
	Bone& pantsBone(const SkeletonAttribute& skeletonAttr);
	Bone& chestBone(const SkeletonAttribute& skeletonAttr);
	Bone& beltBone(const SkeletonAttribute& skeletonAttr);
	Bone& torsoBone(const SkeletonAttribute& skeletonAttr);
	Bone& gliderBone(const SkeletonAttribute& skeletonAttr);
	Bone& toolBone(const SkeletonAttribute& skeletonAttr, float movementY = 0.0f);
	Bone& headBone(const SkeletonAttribute& skeletonAttr);
};

inline const Bone& CharacterSkeleton::bone(BoneId id) const {
	core_assert(id != BoneId::Max);
	return _bones[std::enum_value(id)];
}

inline Bone& CharacterSkeleton::bone(BoneId id) {
	core_assert(id != BoneId::Max);
	return _bones[std::enum_value(id)];
}


}
