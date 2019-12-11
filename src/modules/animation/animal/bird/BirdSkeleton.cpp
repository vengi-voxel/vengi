/**
 * @file
 */

#include "BirdSkeleton.h"
#include "core/Common.h"
#include "animation/BoneUtil.h"
#include "animation/AnimationSettings.h"

namespace animation {

Bone& BirdSkeleton::footBone(BoneId id, const BirdSkeletonAttribute& skeletonAttr) {
	Bone& foot = bone(id);
	foot.scale = glm::one<glm::vec3>();
	return foot;
}

Bone& BirdSkeleton::bodyBone(const BirdSkeletonAttribute& skeletonAttr) {
	Bone& body = bone(BoneId::Body);
	body.scale = glm::vec3(skeletonAttr.bodyScale);
	return body;
}

Bone& BirdSkeleton::headBone(const BirdSkeletonAttribute& skeletonAttr) {
	Bone& head = bone(BoneId::Head);
	head.scale = glm::vec3(skeletonAttr.headScale);
	return head;
}

void BirdSkeleton::update(const AnimationSettings& settings, glm::mat4 (&bones)[shader::SkeletonShader::getMaxBones()]) const {
	const glm::mat4& torsoMat = bone(BoneId::Torso).matrix();
	const glm::mat4& bodyMat = torsoMat * bone(BoneId::Body).matrix();

	SKELETON_BONE_UPDATE(Head,          torsoMat * bone(BoneId::Head).matrix());
	SKELETON_BONE_UPDATE(Body,          bodyMat);

	SKELETON_BONE_UPDATE(LeftFoot,      torsoMat * bone(BoneId::LeftFoot).matrix());
	SKELETON_BONE_UPDATE(RightFoot,     torsoMat * bone(BoneId::RightFoot).matrix());

	SKELETON_BONE_UPDATE(LeftWing,      bodyMat * bone(BoneId::LeftWing).matrix());
	SKELETON_BONE_UPDATE(RightWing,     bodyMat * bone(BoneId::RightWing).matrix());
}

}
