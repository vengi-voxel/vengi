/**
 * @file
 */

#include "Jump.h"
#include "animation/Animation.h"
#include "animation/BoneUtil.h"

namespace animation {
namespace chr {
namespace jump {
void update(double animTime, CharacterSkeleton &skeleton, const CharacterSkeletonAttribute &skeletonAttr) {
	const float scaledAnimTime = animTime * skeletonAttr.jumpTimeFactor;
	const float sine = glm::sin(scaledAnimTime);
	const float sineSlow = glm::sin(scaledAnimTime / 2.0f);
	const float sineStop = glm::sin((glm::min)((float)(animTime * 5.0), glm::half_pi<float>()));
	const float sineStopAlt = glm::sin((glm::min)((float)(animTime * 4.5), glm::half_pi<float>()));
	const float handWaveStop = sineStopAlt * 0.6f;

	Bone &head = skeleton.headBone(skeletonAttr);
	head.translation = glm::vec3(skeletonAttr.neckRight, skeletonAttr.neckHeight + skeletonAttr.headY, skeletonAttr.neckForward);
	head.orientation = rotateX(0.25f + sineStop * 0.1f + sineSlow * 0.04f);

	skeleton.bone(BoneId::Chest) = translate(0.0f, skeletonAttr.chestY, 0.0f);
	skeleton.bone(BoneId::Belt) = translate(0.0f, skeletonAttr.beltY, 0.0f);
	skeleton.bone(BoneId::Pants) = translate(0.0f, skeletonAttr.pantsY, 0.0f);

	const float sineHand = sine * 0.4f;
	const float sineStopHandY = sineStop * 3.2f - sineHand;
	const float sineStopHandZ = sineStop * 3.8f;
	Bone &righthand = skeleton.handBone(BoneId::RightHand, skeletonAttr);
	righthand.translation = glm::vec3(skeletonAttr.handRight + 0.5f, sineStopHandY, skeletonAttr.handForward + sineStopHandZ);
	righthand.orientation = rotateX(-handWaveStop);

	Bone &lefthand = skeleton.bone(BoneId::LeftHand);
	lefthand.translation = glm::vec3(-skeletonAttr.handRight - 0.5f, sineStopHandY, skeletonAttr.handForward - sineStopHandZ);
	lefthand.orientation = rotateX(handWaveStop);

	Bone &rightfoot = skeleton.footBone(BoneId::RightFoot, skeletonAttr);
	rightfoot.translation = glm::vec3(skeletonAttr.footRight, skeletonAttr.hipOffset, -1.0f);
	const float sineStopFoot = sineStop * 1.2f;
	const float sineSlowFoot = sineSlow * 0.2f;
	rightfoot.orientation = rotateX(-sineStopFoot + sineSlowFoot);

	Bone &leftfoot = skeleton.bone(BoneId::LeftFoot);
	leftfoot = mirrorX(rightfoot);
	leftfoot.orientation = rotateX(sineStopFoot + sineSlowFoot);

	skeleton.toolBone(skeletonAttr);

	const float sineStopShoulder = sineStopAlt * 0.3f;
	Bone &rightshoulder = skeleton.shoulderBone(BoneId::RightShoulder, skeletonAttr, rotateX(-sineStopShoulder));

	skeleton.bone(BoneId::LeftShoulder) = mirrorX(rightshoulder);

	Bone &torso = skeleton.torsoBone(skeletonAttr.scaler);
	torso.translation = glm::vec3(0.0f, 0.0f, -0.2f);
	static const glm::quat torsoOrientation = glm::angleAxis(-0.2f, glm::right);
	torso.orientation = torsoOrientation;

	skeleton.bone(BoneId::Glider) = zero();
}
}
}
}
