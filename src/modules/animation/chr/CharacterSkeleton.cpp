/**
 * @file
 */

#include "CharacterSkeleton.h"
#include "core/Common.h"
#include "animation/BoneUtil.h"

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

void CharacterSkeleton::update(glm::mat4 (&bones)[shader::SkeletonShader::getMaxBones()]) const {
	const glm::mat4& chestMat = bone(BoneId::Chest).matrix();
	const glm::mat4& torsoMat = bone(BoneId::Torso).matrix();
	const glm::mat4& headMat  = bone(BoneId::Head).matrix();
	const glm::mat4& neckMat  = torsoMat * chestMat;

	bones[std::enum_value(BoneId::Head)] =          torsoMat * headMat;

	bones[std::enum_value(BoneId::Chest)] =         neckMat;
	bones[std::enum_value(BoneId::LeftHand)] =      neckMat  * bone(BoneId::LeftHand).matrix();
	bones[std::enum_value(BoneId::RightHand)] =     neckMat  * bone(BoneId::RightHand).matrix();
	bones[std::enum_value(BoneId::LeftShoulder)] =  neckMat  * bone(BoneId::LeftShoulder).matrix();
	bones[std::enum_value(BoneId::RightShoulder)] = neckMat  * bone(BoneId::RightShoulder).matrix();
	bones[std::enum_value(BoneId::Tool)] =          neckMat  * bone(BoneId::Tool).matrix();

	bones[std::enum_value(BoneId::Belt)] =          torsoMat * bone(BoneId::Belt).matrix();
	bones[std::enum_value(BoneId::Pants)] =         torsoMat * bone(BoneId::Pants).matrix();
	bones[std::enum_value(BoneId::LeftFoot)] =      torsoMat * bone(BoneId::LeftFoot).matrix();
	bones[std::enum_value(BoneId::RightFoot)] =     torsoMat * bone(BoneId::RightFoot).matrix();

	bones[std::enum_value(BoneId::Glider)] =        torsoMat * bone(BoneId::Glider).matrix();
}

}
