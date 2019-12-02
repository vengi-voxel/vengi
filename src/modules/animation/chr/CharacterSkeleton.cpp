/**
 * @file
 */

#include "CharacterSkeleton.h"
#include "core/Common.h"
#include "animation/BoneUtil.h"
#include "animation/AnimationSettings.h"

namespace animation {

Bone& CharacterSkeleton::handBone(BoneId id, const CharacterSkeletonAttribute& skeletonAttr) {
	Bone& hand = bone(id);
	hand.scale = glm::one<glm::vec3>();
	return hand;
}

Bone& CharacterSkeleton::footBone(BoneId id, const CharacterSkeletonAttribute& skeletonAttr) {
	Bone& foot = bone(id);
	foot.scale = glm::one<glm::vec3>();
	return foot;
}

Bone& CharacterSkeleton::shoulderBone(BoneId id, const CharacterSkeletonAttribute& skeletonAttr, const glm::quat& orientation) {
	Bone& shoulder = bone(id);
	shoulder.scale = glm::vec3(skeletonAttr.shoulderScale);
	shoulder.translation = glm::vec3(skeletonAttr.shoulderRight, skeletonAttr.chestHeight, skeletonAttr.shoulderForward);
	shoulder.orientation = orientation;
	return shoulder;
}

Bone& CharacterSkeleton::pantsBone(const CharacterSkeletonAttribute& skeletonAttr) {
	Bone& pants = bone(BoneId::Pants);
	pants.scale = glm::one<glm::vec3>();
	return pants;
}

Bone& CharacterSkeleton::chestBone(const CharacterSkeletonAttribute& skeletonAttr) {
	Bone& pants = bone(BoneId::Chest);
	pants.scale = glm::one<glm::vec3>();
	return pants;
}

Bone& CharacterSkeleton::beltBone(const CharacterSkeletonAttribute& skeletonAttr) {
	Bone& belt = bone(BoneId::Belt);
	belt.scale = glm::one<glm::vec3>();
	return belt;
}

Bone& CharacterSkeleton::torsoBone(const CharacterSkeletonAttribute& skeletonAttr) {
	Bone& torso = bone(BoneId::Torso);
	torso.scale = glm::vec3(_private::defaultScale * skeletonAttr.scaler);
	return torso;
}

Bone& CharacterSkeleton::gliderBone(const CharacterSkeletonAttribute& skeletonAttr) {
	Bone& torso = bone(BoneId::Glider);
	torso.scale = glm::one<glm::vec3>();
	return torso;
}

Bone& CharacterSkeleton::toolBone(const CharacterSkeletonAttribute& skeletonAttr, float movementY) {
	Bone& tool = bone(BoneId::Tool);
	tool.scale = glm::vec3(skeletonAttr.toolScale);
	tool.translation = glm::vec3(skeletonAttr.toolRight, skeletonAttr.headY + 2.0f, skeletonAttr.toolForward);
	tool.orientation = rotateYZ(glm::radians(-90.0f) + movementY, glm::radians(143.0f));
	return tool;
}

Bone& CharacterSkeleton::headBone(const CharacterSkeletonAttribute& skeletonAttr) {
	Bone& head = bone(BoneId::Head);
	head.scale = glm::vec3(skeletonAttr.headScale);
	return head;
}

void CharacterSkeleton::update(const AnimationSettings& settings, glm::mat4 (&bones)[shader::SkeletonShader::getMaxBones()]) const {
	const glm::mat4& chestMat = bone(BoneId::Chest).matrix();
	const glm::mat4& torsoMat = bone(BoneId::Torso).matrix();
	const glm::mat4& headMat  = bone(BoneId::Head).matrix();
	const glm::mat4& neckMat  = torsoMat * chestMat;

	SKELETON_BONE_UPDATE(Head,          torsoMat * headMat);

	SKELETON_BONE_UPDATE(Chest,         neckMat);
	SKELETON_BONE_UPDATE(LeftHand,      neckMat  * bone(BoneId::LeftHand).matrix());
	SKELETON_BONE_UPDATE(RightHand,     neckMat  * bone(BoneId::RightHand).matrix());
	SKELETON_BONE_UPDATE(LeftShoulder,  neckMat  * bone(BoneId::LeftShoulder).matrix());
	SKELETON_BONE_UPDATE(RightShoulder, neckMat  * bone(BoneId::RightShoulder).matrix());
	SKELETON_BONE_UPDATE(Tool,          neckMat  * bone(BoneId::Tool).matrix());

	SKELETON_BONE_UPDATE(Belt,          torsoMat * bone(BoneId::Belt).matrix());
	SKELETON_BONE_UPDATE(Pants,         torsoMat * bone(BoneId::Pants).matrix());
	SKELETON_BONE_UPDATE(LeftFoot,      torsoMat * bone(BoneId::LeftFoot).matrix());
	SKELETON_BONE_UPDATE(RightFoot,     torsoMat * bone(BoneId::RightFoot).matrix());

	SKELETON_BONE_UPDATE(Glider,        torsoMat * bone(BoneId::Glider).matrix());
}

}
