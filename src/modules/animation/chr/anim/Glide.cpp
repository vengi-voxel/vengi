/**
 * @file
 */

#include "Glide.h"
#include "animation/Animation.h"
#include "animation/BoneUtil.h"

namespace animation {
namespace chr {
namespace glide {
void update(double animTime, CharacterSkeleton &skeleton, const CharacterSkeletonAttribute &skeletonAttr) {
	const float sine = glm::sin(animTime * 3.0f) * 0.1f;
	const float cosine = glm::cos(animTime * 3.0f);
	const float movement = glm::sin(animTime) * skeletonAttr.idleTimeFactor;

	Bone &head = skeleton.headBone(skeletonAttr);
	head.translation = glm::vec3(skeletonAttr.neckRight, skeletonAttr.glidingUpwards + skeletonAttr.neckHeight + skeletonAttr.headY + movement, skeletonAttr.neckForward + skeletonAttr.glidingForward);
	const float headRotation = glm::radians(-20.0f);
	head.orientation = rotateXYZ(headRotation, sine, cosine * 0.05f);

	skeleton.bone(BoneId::Chest) = translate(0.0f, skeletonAttr.glidingUpwards + skeletonAttr.chestY + movement, skeletonAttr.glidingForward);
	skeleton.bone(BoneId::Belt) = translate(0.0f, skeletonAttr.glidingUpwards + skeletonAttr.beltY + movement, skeletonAttr.glidingForward);
	skeleton.bone(BoneId::Pants) = translate(0.0f, skeletonAttr.glidingUpwards + skeletonAttr.pantsY + movement, skeletonAttr.glidingForward);

	const float handRotation = glm::radians(-50.0f);
	Bone &righthand = skeleton.handBone(BoneId::RightHand, skeletonAttr);
	righthand.translation = glm::vec3(skeletonAttr.handRight, skeletonAttr.headY + sine, skeletonAttr.handForward + cosine * 0.15f);
	righthand.orientation = rotateXZ(handRotation, glm::radians(180.0f));

	Bone &lefthand = skeleton.handBone(BoneId::LeftHand, skeletonAttr);
	lefthand.translation = glm::vec3(-skeletonAttr.handRight, skeletonAttr.headY + sine, skeletonAttr.handForward - cosine * 0.15f);
	lefthand.orientation = rotateXZ(handRotation, glm::radians(-180.0f));

	Bone &rightfoot = skeleton.footBone(BoneId::RightFoot, skeletonAttr);
	rightfoot.translation = glm::vec3(skeletonAttr.footRight, skeletonAttr.glidingUpwards + skeletonAttr.hipOffset, skeletonAttr.glidingForward);
	rightfoot.orientation = glm::quat_identity<float, glm::defaultp>();

	Bone &leftfoot = skeleton.footBone(BoneId::LeftFoot, skeletonAttr);
	leftfoot = mirrorX(rightfoot);
	leftfoot.orientation = glm::quat_identity<float, glm::defaultp>();

	skeleton.toolBone(skeletonAttr);

	Bone &rightshoulder = skeleton.shoulderBone(BoneId::RightShoulder, skeletonAttr);

	const float torsoRotation = glm::radians(55.0f);
	Bone &glider = skeleton.gliderBone(skeletonAttr);
	glider.translation = glm::vec3(0.0f, skeletonAttr.gliderY, sine);
	glider.orientation = rotateX(-torsoRotation);

	Bone &torso = skeleton.torsoBone(skeletonAttr.scaler);
	torso.translation = glm::zero<glm::vec3>();
	torso.orientation = rotateXZ(torsoRotation + sine * 0.3f, sine);

	skeleton.bone(BoneId::LeftHand) = mirrorX(skeleton.bone(BoneId::RightHand));
	skeleton.bone(BoneId::LeftFoot) = mirrorX(skeleton.bone(BoneId::RightFoot));
	skeleton.bone(BoneId::LeftShoulder) = mirrorX(rightshoulder);
}
}
}
}
