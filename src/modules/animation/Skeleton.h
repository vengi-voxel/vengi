/**
 * @file
 */

#pragma once

#include "Bone.h"
#include "BoneId.h"
#include "core/Enum.h"
#include "core/collection/Array.h"
#include "SkeletonShaderConstants.h"

namespace animation {

class AnimationSettings;

#define SKELETON_BONE_UPDATE(boneid, assign) \
	const int8_t boneid##boneIdx = settings.mapBoneIdToArrayIndex(BoneId::boneid); \
	if (boneid##boneIdx >= 0 && boneid##boneIdx < shader::SkeletonShaderConstants::getMaxBones()) { \
		bones[boneid##boneIdx] = assign; \
	} else { \
		Log::warn("Invalid bone idx for bone %s", toBoneId(BoneId::boneid)); \
	}

/**
 * @brief Calculates the skeleton by the single bones of the entity
 * @ingroup Animation
 */
class Skeleton {
private:
	core::Array<Bone, core::enumVal(BoneId::Max)> _bones;
public:
	Skeleton();
	virtual ~Skeleton() {}
	const Bone& bone(BoneId id) const;
	Bone& bone(BoneId id);
	Bone& torsoBone(float scale = 1.0f);

	/**
	 * @brief Calculate the skeleton bones matrices which indices are assigned to the
	 * mesh vertices to perform the skeletal animation.
	 */
	virtual void update(const AnimationSettings& settings, glm::mat4 (&bones)[shader::SkeletonShaderConstants::getMaxBones()]) const = 0;
	/**
	 * @brief Linear interpolate from one skeletal animation state to a new one.
	 */
	void lerp(const Skeleton& previous, double deltaFrameSeconds);
};

}
