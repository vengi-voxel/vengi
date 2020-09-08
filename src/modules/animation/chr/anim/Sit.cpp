/**
 * @file
 */

#include "Sit.h"
#include "animation/BoneUtil.h"

namespace animation {
namespace chr {
namespace sit {
static void update(double animTime, CharacterSkeleton &skeleton, const CharacterSkeletonAttribute &skeletonAttr) {
	const float sine = glm::sin(animTime);
	const float cosine = glm::cos(animTime);
	const float movement = sine * skeletonAttr.idleTimeFactor - skeletonAttr.footHeight;

	Bone &head = skeleton.headBone(skeletonAttr);
	head.translation = glm::vec3(skeletonAttr.neckRight, skeletonAttr.neckHeight + skeletonAttr.headY + movement, skeletonAttr.neckForward);
	head.orientation = rotateYZ(sine * 0.1f, cosine * 0.05f);

	skeleton.bone(BoneId::Chest) = translate(0.0f, skeletonAttr.chestY + movement, 0.0f);
	skeleton.bone(BoneId::Belt) = translate(0.0f, skeletonAttr.beltY + movement, 0.0f);
	skeleton.bone(BoneId::Pants) = translate(0.0f, skeletonAttr.pantsY + movement, 0.0f);

	skeleton.toolBone(skeletonAttr);

	skeleton.torsoBone(skeletonAttr.scaler);

	Bone &righthand = skeleton.handBone(BoneId::RightHand, skeletonAttr);
	righthand.translation = glm::vec3(skeletonAttr.handRight, sine * 0.5f, skeletonAttr.handForward + cosine * 0.15f);
	righthand.orientation = rotateX(sine * -0.06f);

	Bone& rightfoot = skeleton.footBone(BoneId::RightFoot, skeletonAttr);
	rightfoot.translation = glm::vec3(skeletonAttr.footRight, skeletonAttr.hipOffset - skeletonAttr.footHeight, 1.0f);
	rightfoot.orientation = rotateX(glm::radians(-80.0f));

	Bone& leftfoot = skeleton.footBone(BoneId::LeftFoot, skeletonAttr);
	leftfoot.translation = mirrorX(rightfoot.translation);
	leftfoot.scale = mirrorX(rightfoot.scale);
	leftfoot.orientation = rightfoot.orientation;

	Bone &rightshoulder = skeleton.shoulderBone(BoneId::RightShoulder, skeletonAttr);

	skeleton.bone(BoneId::Glider) = zero();
	skeleton.bone(BoneId::LeftHand) = mirrorX(righthand);
	skeleton.bone(BoneId::LeftShoulder) = mirrorX(rightshoulder);
}
}
}
}

void animation_chr_sit_update(double animTime, animation::CharacterSkeleton* skeleton, const animation::CharacterSkeletonAttribute* skeletonAttr) {
	animation::chr::sit::update(animTime, *skeleton, *skeletonAttr);
}
