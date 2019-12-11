/**
 * @file
 */

#include "BirdSkeleton.h"
#include "core/Common.h"
#include "animation/BoneUtil.h"
#include "animation/AnimationSettings.h"

namespace animation {

void BirdSkeleton::update(const AnimationSettings& settings, glm::mat4 (&bones)[shader::SkeletonShader::getMaxBones()]) const {
	const glm::mat4& torsoMat = bone(BoneId::Torso).matrix();
	const glm::mat4& headMat  = bone(BoneId::Head).matrix();

	SKELETON_BONE_UPDATE(Torso,         torsoMat);
	SKELETON_BONE_UPDATE(Head,          torsoMat * headMat);

	SKELETON_BONE_UPDATE(LeftFoot,      torsoMat * bone(BoneId::LeftFoot).matrix());
	SKELETON_BONE_UPDATE(RightFoot,     torsoMat * bone(BoneId::RightFoot).matrix());

	SKELETON_BONE_UPDATE(LeftWing,      torsoMat * bone(BoneId::LeftWing).matrix());
	SKELETON_BONE_UPDATE(RightWing,     torsoMat * bone(BoneId::RightWing).matrix());
}

}
