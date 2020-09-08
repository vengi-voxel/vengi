/**
 * @file
 */

#pragma once

#include "animation/Bone.h"
#include "animation/BoneId.h"
#include "animation/BoneUtil.h"
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

inline Bone& CharacterSkeleton::handBone(BoneId id, const CharacterSkeletonAttribute& skeletonAttr) {
	Bone& hand = bone(id);
	hand.scale = glm::one<glm::vec3>();
	return hand;
}

inline Bone& CharacterSkeleton::footBone(BoneId id, const CharacterSkeletonAttribute& skeletonAttr) {
	Bone& foot = bone(id);
	foot.scale = glm::one<glm::vec3>();
	return foot;
}

inline Bone& CharacterSkeleton::shoulderBone(BoneId id, const CharacterSkeletonAttribute& skeletonAttr, const glm::quat& orientation) {
	Bone& shoulder = bone(id);
	shoulder.scale = glm::vec3(skeletonAttr.shoulderScale);
	shoulder.translation = glm::vec3(skeletonAttr.shoulderRight, skeletonAttr.chestHeight, skeletonAttr.shoulderForward);
	shoulder.orientation = orientation;
	return shoulder;
}

inline Bone& CharacterSkeleton::pantsBone(const CharacterSkeletonAttribute& skeletonAttr) {
	Bone& pants = bone(BoneId::Pants);
	pants.scale = glm::one<glm::vec3>();
	return pants;
}

inline Bone& CharacterSkeleton::chestBone(const CharacterSkeletonAttribute& skeletonAttr) {
	Bone& pants = bone(BoneId::Chest);
	pants.scale = glm::one<glm::vec3>();
	return pants;
}

inline Bone& CharacterSkeleton::beltBone(const CharacterSkeletonAttribute& skeletonAttr) {
	Bone& belt = bone(BoneId::Belt);
	belt.scale = glm::one<glm::vec3>();
	return belt;
}

inline Bone& CharacterSkeleton::gliderBone(const CharacterSkeletonAttribute& skeletonAttr) {
	Bone& torso = bone(BoneId::Glider);
	torso.scale = glm::one<glm::vec3>();
	return torso;
}

inline Bone& CharacterSkeleton::toolBone(const CharacterSkeletonAttribute& skeletonAttr, float movementY) {
	Bone& tool = bone(BoneId::Tool);
	tool.scale = glm::vec3(skeletonAttr.toolScale);
	tool.translation = glm::vec3(skeletonAttr.toolRight, skeletonAttr.headY + 2.0f, skeletonAttr.toolForward);
	tool.orientation = rotateYZ(glm::radians(-90.0f) + movementY, glm::radians(143.0f));
	return tool;
}

inline Bone& CharacterSkeleton::headBone(const CharacterSkeletonAttribute& skeletonAttr) {
	Bone& head = bone(BoneId::Head);
	head.scale = glm::vec3(skeletonAttr.headScale);
	return head;
}

}
