/**
 * @file
 */

#include "Tool.h"
#include "animation/Animation.h"
#include "animation/BoneUtil.h"

namespace animation {
namespace tool {
void update(float animTime, ToolAnimationType animation, CharacterSkeleton &skeleton, const SkeletonAttribute &skeletonAttr) {
	const float movement = glm::sin(animTime * 12.0f);

	// TODO: handle ToolAnimationType properly

	const float headMovement = movement * 0.1f;
	Bone &head = skeleton.headBone(skeletonAttr);
	head.translation = glm::vec3(skeletonAttr.neckRight, skeletonAttr.neckHeight + skeletonAttr.headY, skeletonAttr.neckForward);
	head.orientation = rotateXYZ(headMovement, headMovement, headMovement);

	skeleton.bone(BoneId::Chest) = translate(0.0f, skeletonAttr.chestY, 0.0f);
	skeleton.bone(BoneId::Belt) = translate(0.0f, skeletonAttr.beltY, 0.0f);
	skeleton.bone(BoneId::Pants) = translate(0.0f, skeletonAttr.pantsY, 0.0f);

	const float betweenOneAndTwoFast = 1.0f - glm::cos(animTime * 14.0f);
	Bone &righthand = skeleton.handBone(BoneId::RightHand, skeletonAttr);
	righthand.translation = glm::vec3(skeletonAttr.handRight + betweenOneAndTwoFast, 0.0f, skeletonAttr.handForward + 2.0f + betweenOneAndTwoFast * 2.0f);
	righthand.orientation = rotateXY(betweenOneAndTwoFast * 0.8f, betweenOneAndTwoFast * 0.4f);

	Bone &lefthand = skeleton.handBone(BoneId::LeftHand, skeletonAttr);
	lefthand.translation = glm::vec3(-skeletonAttr.handRight, 0.0f, skeletonAttr.handForward);
	lefthand.scale.x = -righthand.scale.x;
	lefthand.orientation = glm::quat_identity<float, glm::defaultp>();

	Bone &rightfoot = skeleton.footBone(BoneId::RightFoot, skeletonAttr);
	rightfoot.translation = glm::vec3(skeletonAttr.footRight, skeletonAttr.hipOffset, 1.0f);
	rightfoot.orientation = glm::quat_identity<float, glm::defaultp>();

	Bone &leftfoot = skeleton.bone(BoneId::LeftFoot);
	leftfoot = mirrorX(rightfoot);
	leftfoot.translation = glm::vec3(-skeletonAttr.footRight, skeletonAttr.hipOffset, -1.0f);

	Bone &tool = skeleton.toolBone(skeletonAttr);
	tool.translation = righthand.translation;
	tool.orientation = righthand.orientation;

	Bone &rightshoulder = skeleton.shoulderBone(BoneId::RightShoulder, skeletonAttr);

	skeleton.bone(BoneId::LeftShoulder) = mirrorX(rightshoulder);

	const float torsoRotationX = movement * 0.1f;
	const float torsoRotationY = movement * 0.01f;
	const float torsoRotationZ = movement * 0.01f;
	Bone &torso = skeleton.torsoBone(skeletonAttr);
	torso.translation = glm::zero<glm::vec3>();
	torso.orientation = rotateXYZ(torsoRotationX, torsoRotationY, torsoRotationZ);

	skeleton.bone(BoneId::Glider) = zero();
}
}
}
