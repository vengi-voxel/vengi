/**
 * @file
 */

#include "Tool.h"
#include "animation/BoneUtil.h"

namespace animation {
namespace chr {
namespace tool {

static inline void head(double animTime, CharacterSkeleton& skeleton, const CharacterSkeletonAttribute &skeletonAttr) {
	const float movement = glm::sin(animTime * 12.0f);
	const float headMovement = movement * 0.1f;
	Bone &head = skeleton.headBone(skeletonAttr);
	head.translation = glm::vec3(skeletonAttr.neckRight, skeletonAttr.neckHeight + skeletonAttr.headY, skeletonAttr.neckForward);
	head.orientation = rotateXYZ(headMovement, headMovement, headMovement);
}

static void swing(double animTime, CharacterSkeleton &skeleton, const CharacterSkeletonAttribute &skeletonAttr) {
	const float betweenOneAndTwoFast = 1.0f - glm::cos(animTime * 14.0f);
	Bone &righthand = skeleton.handBone(BoneId::RightHand, skeletonAttr);
	righthand.translation = glm::vec3(skeletonAttr.handRight + betweenOneAndTwoFast, 0.0f, skeletonAttr.handForward + 2.0f + betweenOneAndTwoFast * 2.0f);
	righthand.orientation = rotateXYZ(betweenOneAndTwoFast * 0.8f, betweenOneAndTwoFast * 0.8f, betweenOneAndTwoFast * 0.4f * glm::radians(45.0f));

	Bone &lefthand = skeleton.handBone(BoneId::LeftHand, skeletonAttr);
	lefthand.translation = glm::vec3(-skeletonAttr.handRight, 0.0f, skeletonAttr.handForward - betweenOneAndTwoFast);
	lefthand.scale.x = -righthand.scale.x;
	lefthand.orientation = glm::quat_identity<float, glm::defaultp>();

	Bone &rightfoot = skeleton.footBone(BoneId::RightFoot, skeletonAttr);
	rightfoot.translation = glm::vec3(skeletonAttr.footRight, skeletonAttr.hipOffset, betweenOneAndTwoFast * 0.5f);
	rightfoot.orientation = glm::quat_identity<float, glm::defaultp>();

	Bone &leftfoot = skeleton.bone(BoneId::LeftFoot);
	leftfoot = mirrorX(rightfoot);
	leftfoot.translation = glm::vec3(-skeletonAttr.footRight, skeletonAttr.hipOffset, -1.0f);

	const float movement = glm::sin(animTime * 12.0f);
	const float torsoRotationX = movement * 0.1f;
	const float torsoRotationY = movement * 0.01f;
	const float torsoRotationZ = torsoRotationY;
	Bone &torso = skeleton.torsoBone(skeletonAttr.scaler);
	torso.orientation = rotateXYZ(torsoRotationX, torsoRotationY, torsoRotationZ);
}

static void tense(double animTime, CharacterSkeleton &skeleton, const CharacterSkeletonAttribute &skeletonAttr) {
}

static void twiddle(double animTime, CharacterSkeleton &skeleton, const CharacterSkeletonAttribute &skeletonAttr) {
}

static void stroke(double animTime, CharacterSkeleton &skeleton, const CharacterSkeletonAttribute &skeletonAttr) {
	const float betweenOneAndTwoFast = 1.0f - glm::cos(animTime * 14.0f);
	Bone &righthand = skeleton.handBone(BoneId::RightHand, skeletonAttr);
	righthand.translation = glm::vec3(skeletonAttr.handRight + betweenOneAndTwoFast, 0.0f, skeletonAttr.handForward + 2.0f + betweenOneAndTwoFast * 2.0f);
	righthand.orientation = rotateXY(betweenOneAndTwoFast * 0.8f, betweenOneAndTwoFast * 0.4f);

	Bone &lefthand = skeleton.handBone(BoneId::LeftHand, skeletonAttr);
	lefthand.translation = glm::vec3(-skeletonAttr.handRight, 0.0f, skeletonAttr.handForward - betweenOneAndTwoFast);
	lefthand.scale.x = -righthand.scale.x;
	lefthand.orientation = glm::quat_identity<float, glm::defaultp>();

	Bone &rightfoot = skeleton.footBone(BoneId::RightFoot, skeletonAttr);
	rightfoot.translation = glm::vec3(skeletonAttr.footRight, skeletonAttr.hipOffset, betweenOneAndTwoFast * 0.5f);
	rightfoot.orientation = glm::quat_identity<float, glm::defaultp>();

	Bone &leftfoot = skeleton.bone(BoneId::LeftFoot);
	leftfoot = mirrorX(rightfoot);
	leftfoot.translation = glm::vec3(-skeletonAttr.footRight, skeletonAttr.hipOffset, -1.0f);

	const float movement = glm::sin(animTime * 12.0f);
	const float torsoRotationX = movement * 0.1f;
	const float torsoRotationY = movement * 0.01f;
	const float torsoRotationZ = movement * 0.01f;
	Bone &torso = skeleton.torsoBone(skeletonAttr.scaler);
	torso.orientation = rotateXYZ(torsoRotationX, torsoRotationY, torsoRotationZ);
}

void update(double animTime, ToolAnimationType animation, CharacterSkeleton &skeleton, const CharacterSkeletonAttribute &skeletonAttr) {
	head(animTime, skeleton, skeletonAttr);

	skeleton.bone(BoneId::Chest) = translate(0.0f, skeletonAttr.chestY, 0.0f);
	skeleton.bone(BoneId::Belt) = translate(0.0f, skeletonAttr.beltY, 0.0f);
	skeleton.bone(BoneId::Pants) = translate(0.0f, skeletonAttr.pantsY, 0.0f);

	Bone &rightshoulder = skeleton.shoulderBone(BoneId::RightShoulder, skeletonAttr);
	skeleton.bone(BoneId::LeftShoulder) = mirrorX(rightshoulder);

	switch (animation) {
	case ToolAnimationType::Stroke:
		stroke(animTime, skeleton, skeletonAttr);
		break;
	case ToolAnimationType::Swing:
		swing(animTime, skeleton, skeletonAttr);
		break;
	case ToolAnimationType::Tense:
		tense(animTime, skeleton, skeletonAttr);
		break;
	case ToolAnimationType::Twiddle:
		twiddle(animTime, skeleton, skeletonAttr);
		break;
	case ToolAnimationType::None:
	case ToolAnimationType::Max:
		break;
	}

	const Bone &righthand = skeleton.bone(BoneId::RightHand);
	Bone &tool = skeleton.toolBone(skeletonAttr);
	tool.translation = righthand.translation;
	tool.orientation = righthand.orientation;

	skeleton.bone(BoneId::Glider) = zero();
}
}
}
}
