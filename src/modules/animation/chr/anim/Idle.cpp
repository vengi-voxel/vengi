/**
 * @file
 */

#include "Idle.h"
#include "animation/Animation.h"
#include "animation/BoneUtil.h"

namespace animation {
namespace chr {
namespace idle {
void update(float animTime, CharacterSkeleton &skeleton, const CharacterSkeletonAttribute &skeletonAttr) {
	const float sine = glm::sin(animTime);
	const float cosine = glm::cos(animTime);
	const float movement = sine * skeletonAttr.idleTimeFactor;

	Bone &head = skeleton.headBone(skeletonAttr);
	head.translation = glm::vec3(skeletonAttr.neckRight, skeletonAttr.neckHeight + skeletonAttr.headY + movement, skeletonAttr.neckForward);
	head.orientation = rotateYZ(sine * 0.1f, cosine * 0.05f);

	skeleton.bone(BoneId::Chest) = translate(0.0f, skeletonAttr.chestY + movement, 0.0f);
	skeleton.bone(BoneId::Belt) = translate(0.0f, skeletonAttr.beltY + movement, 0.0f);
	skeleton.bone(BoneId::Pants) = translate(0.0f, skeletonAttr.pantsY + movement, 0.0f);

	skeleton.toolBone(skeletonAttr);

	Bone &torso = skeleton.torsoBone(skeletonAttr.scaler);
	torso.translation = glm::zero<glm::vec3>();
	torso.orientation = glm::quat_identity<float, glm::defaultp>();

	Bone &righthand = skeleton.handBone(BoneId::RightHand, skeletonAttr);
	righthand.translation = glm::vec3(skeletonAttr.handRight, sine * 0.5f, skeletonAttr.handForward + cosine * 0.15f);
	righthand.orientation = rotateX(sine * -0.06f);

	skeleton.bone(BoneId::RightFoot) = translate(skeletonAttr.footRight, skeletonAttr.hipOffset, 0.0f);

	Bone &rightshoulder = skeleton.shoulderBone(BoneId::RightShoulder, skeletonAttr);

	skeleton.bone(BoneId::Glider) = zero();
	skeleton.bone(BoneId::LeftHand) = mirrorX(righthand);
	skeleton.bone(BoneId::LeftFoot) = mirrorX(skeleton.bone(BoneId::RightFoot));
	skeleton.bone(BoneId::LeftShoulder) = mirrorX(rightshoulder);
}
}
}
}
