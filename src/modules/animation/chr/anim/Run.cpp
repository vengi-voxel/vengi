/**
 * @file
 */

#include "Run.h"
#include "animation/BoneUtil.h"

namespace animation {
namespace chr {
namespace run {
static void update(double animTime, double velocity, CharacterSkeleton &skeleton, const CharacterSkeletonAttribute &skeletonAttr) {
	const float timeFactor = skeletonAttr.runTimeFactor;
	const float scaleAnimTime = animTime * timeFactor;
	const float sine = glm::sin(scaleAnimTime);
	const float cosine = glm::cos(scaleAnimTime);
	const float cosineDouble = glm::cos(scaleAnimTime * 2.0f);
	const float movement = 0.35f * sine;
	const float headLookX = 0.05f * glm::cos(animTime) + glm::radians(10.0f);
	const float headLookY = 0.1f * sine;

	velocity = glm::clamp((float)(0.01 * velocity), 0.1f, 2.5f);

	Bone &head = skeleton.headBone(skeletonAttr);
	head.translation = glm::vec3(0.0f, skeletonAttr.neckHeight + skeletonAttr.headY + 1.3f * cosine, -1.0f + skeletonAttr.neckForward);
	head.orientation = rotateXY(headLookX, headLookY);

	const glm::quat& rotateYMovement = rotateY(movement);
	const float bodyMoveY = 1.1f * cosine;
	Bone &chest = skeleton.chestBone(skeletonAttr);
	chest.translation = glm::vec3(0.0f, skeletonAttr.chestY + bodyMoveY, 0.0f);
	chest.orientation = rotateYMovement;

	Bone &belt = skeleton.beltBone(skeletonAttr);
	belt.translation = glm::vec3(0.0f, skeletonAttr.beltY + bodyMoveY, 0.0f);
	belt.orientation = rotateYMovement;

	Bone &pants = skeleton.pantsBone(skeletonAttr);
	pants.translation = glm::vec3(0.0f, skeletonAttr.pantsY + bodyMoveY, 0.0f);
	pants.orientation = rotateYMovement;

	const float handAngle = sine * 0.2f;
	const float handMoveY = cosine;
	const float handMoveZ = cosine * 4.0f;
	Bone &righthand = skeleton.handBone(BoneId::RightHand, skeletonAttr);
	righthand.translation = glm::vec3(skeletonAttr.handRight + cosineDouble, handMoveY, skeletonAttr.handForward + handMoveZ);
	righthand.orientation = rotateX(handAngle);

	skeleton.bone(BoneId::LeftHand) = mirrorX(righthand);

	const float footAngle = cosine * 1.5f;
	const float footMoveY = cosineDouble * 0.5f;
	Bone &rightfoot = skeleton.footBone(BoneId::RightFoot, skeletonAttr);
	rightfoot.translation = glm::vec3(skeletonAttr.footRight, skeletonAttr.hipOffset - footMoveY, 0.0f);
	rightfoot.orientation = rotateX(footAngle);

	skeleton.bone(BoneId::LeftFoot) = mirrorX(rightfoot);

	skeleton.toolBone(skeletonAttr, cosine * 0.25f);

	Bone &rightshoulder = skeleton.shoulderBone(BoneId::RightShoulder, skeletonAttr, rotateX(sine * 0.15f));

	Bone &torso = skeleton.torsoBone(skeletonAttr.scaler);
	torso.translation = glm::vec3(0.0f, 0.0f, sine * 0.04f);
	torso.orientation = rotateX(cosine * 0.1f);

	skeleton.bone(BoneId::Glider) = zero();
	skeleton.bone(BoneId::LeftShoulder) = mirrorX(rightshoulder);
}
}
}
}

void animation_chr_run_update(double animTime, double velocity, animation::CharacterSkeleton* skeleton, const animation::CharacterSkeletonAttribute* skeletonAttr) {
	animation::chr::run::update(animTime, velocity, *skeleton, *skeletonAttr);
}
