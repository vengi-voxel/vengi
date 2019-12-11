/**
 * @file
 */

#include "Jump.h"
#include "animation/Animation.h"
#include "animation/BoneUtil.h"

namespace animation {
namespace chr {
namespace jump {
void update(float animTime, CharacterSkeleton &skeleton, const CharacterSkeletonAttribute &skeletonAttr) {
	const float sine = glm::sin(animTime * skeletonAttr.jumpTimeFactor);
	const float sineSlow = glm::sin(animTime * skeletonAttr.jumpTimeFactor / 2.0f);
	const float sineStop = glm::sin((glm::min)(animTime * 5.0f, glm::half_pi<float>()));
	const float sineStopAlt = glm::sin((glm::min)(animTime * 4.5f, glm::half_pi<float>()));
	const float handWaveStop = sineStopAlt * 0.6f;

	Bone &head = skeleton.headBone(skeletonAttr);
	head.translation = glm::vec3(skeletonAttr.neckRight, skeletonAttr.neckHeight + skeletonAttr.headY, skeletonAttr.neckForward);
	head.orientation = rotateX(0.25f + sineStop * 0.1f + sineSlow * 0.04f);

	skeleton.bone(BoneId::Chest) = translate(0.0f, skeletonAttr.chestY, 0.0f);
	skeleton.bone(BoneId::Belt) = translate(0.0f, skeletonAttr.beltY, 0.0f);
	skeleton.bone(BoneId::Pants) = translate(0.0f, skeletonAttr.pantsY, 0.0f);

	Bone &righthand = skeleton.handBone(BoneId::RightHand, skeletonAttr);
	righthand.translation = glm::vec3(skeletonAttr.handRight + 0.5f, sineStop * 3.2f - sine * 0.4f, skeletonAttr.handForward + sineStop * 3.8f);
	righthand.orientation = rotateX(-handWaveStop);

	Bone &lefthand = skeleton.bone(BoneId::LeftHand);
	lefthand.translation = glm::vec3(-skeletonAttr.handRight - 0.5f, sineStop * 3.2f - sine * 0.4f, skeletonAttr.handForward + sineStop * -3.8f);
	lefthand.orientation = rotateX(handWaveStop);

	Bone &rightfoot = skeleton.footBone(BoneId::RightFoot, skeletonAttr);
	rightfoot.translation = glm::vec3(skeletonAttr.footRight, skeletonAttr.hipOffset, -1.0f);
	rightfoot.orientation = rotateX(-sineStop * 1.2f + sineSlow * 0.2f);

	Bone &leftfoot = skeleton.bone(BoneId::LeftFoot);
	leftfoot = mirrorX(rightfoot);
	leftfoot.orientation = rotateX(sineStop * 1.2f + sineSlow * 0.2f);

	skeleton.toolBone(skeletonAttr);

	Bone &rightshoulder = skeleton.shoulderBone(BoneId::RightShoulder, skeletonAttr, rotateX(-sineStopAlt * 0.3f));

	Bone &leftshoulder = skeleton.bone(BoneId::LeftShoulder);
	leftshoulder = mirrorX(rightshoulder);
	leftshoulder.orientation = rotateX(sineStopAlt * 0.3f);

	Bone &torso = skeleton.torsoBone(skeletonAttr.scaler);
	torso.translation = glm::vec3(0.0f, 0.0f, -0.2f);
	torso.orientation = glm::angleAxis(-0.2f, glm::right);

	skeleton.bone(BoneId::Glider) = zero();
}
}
}
}
