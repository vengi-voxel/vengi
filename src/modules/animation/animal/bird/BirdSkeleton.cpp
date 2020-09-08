/**
 * @file
 */

#include "BirdSkeleton.h"
#include "core/Common.h"
#include "animation/BoneUtil.h"
#include "animation/AnimationSettings.h"
#include "core/Log.h"

namespace animation {

void BirdSkeleton::update(const AnimationSettings& settings, glm::mat4 (&bones)[shader::SkeletonShaderConstants::getMaxBones()]) const {
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
