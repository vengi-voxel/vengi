/**
 * @file
 */

#pragma once

#include "animation/Bone.h"
#include "animation/BoneId.h"
#include "animation/Skeleton.h"
#include "CharacterSkeletonAttribute.h"
#include <glm/fwd.hpp>

namespace animation {

/**
 * @brief The bones of the @c Character
 * @ingroup Animation
 */
class CharacterSkeleton : public Skeleton {
public:
	void update(const AnimationSettings& settings, glm::mat4 (&bones)[shader::SkeletonShaderConstants::getMaxBones()]) const override;

	Bone& handBone(BoneId id, const CharacterSkeletonAttribute& skeletonAttr);
	Bone& footBone(BoneId id, const CharacterSkeletonAttribute& skeletonAttr);
	Bone& shoulderBone(BoneId id, const CharacterSkeletonAttribute& skeletonAttr, const glm::quat& orientation = glm::quat_identity<float, glm::defaultp>());
	Bone& pantsBone(const CharacterSkeletonAttribute& skeletonAttr);
	Bone& chestBone(const CharacterSkeletonAttribute& skeletonAttr);
	Bone& beltBone(const CharacterSkeletonAttribute& skeletonAttr);
	Bone& gliderBone(const CharacterSkeletonAttribute& skeletonAttr);
	Bone& toolBone(const CharacterSkeletonAttribute& skeletonAttr, float movementY = 0.0f);
	Bone& headBone(const CharacterSkeletonAttribute& skeletonAttr);
};

}
