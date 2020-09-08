/**
 * @file
 */

#include "CharacterSkeleton.h"
#include "animation/BoneUtil.h"
#include "animation/AnimationSettings.h"
#include "core/Log.h"
#include "core/GLM.h"

namespace animation {

void CharacterSkeleton::update(const AnimationSettings& settings, glm::mat4 (&bones)[shader::SkeletonShaderConstants::getMaxBones()]) const {
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
