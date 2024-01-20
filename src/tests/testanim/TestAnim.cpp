/**
 * @file
 */

#include "TestAnim.h"
#include "app/App.h"
#include "core/Log.h"
#include "testcore/TestAppMain.h"

#include <ozz/animation/offline/animation_builder.h>
#include <ozz/animation/offline/raw_animation.h>
#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/sampling_job.h>

TestAnim::TestAnim(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider)
	: Super(filesystem, timeProvider) {
	init(ORGANISATION, "testanim");
}

ozz::unique_ptr<ozz::animation::Animation> TestAnim::assembleAnimation() {
	ozz::animation::offline::RawAnimation rawAnimation;

	// Sets animation duration (to 1.4s).
	// All the animation keyframes times must be within range [0, duration].
	rawAnimation.duration = 1.4f;

	// Creates 3 animation tracks.
	// There should be as much tracks as there are joints in the skeleton that
	// this animation targets.
	rawAnimation.tracks.resize(3);

	// Fills each track with keyframes, in joint local-space.
	// Tracks should be ordered in the same order as joints in the
	// ozz::animation::Skeleton. Joint's names can be used to find joint's
	// index in the skeleton.

	// Fills 1st track with 2 translation keyframes.
	{
		// Create a keyframe, at t=0, with a translation value.
		const ozz::animation::offline::RawAnimation::TranslationKey key0 = {0.f, ozz::math::Float3(0.f, 4.6f, 0.f)};

		rawAnimation.tracks[0].translations.push_back(key0);

		// Create a new keyframe, at t=0.93 (must be less than duration), with a
		// translation value.
		const ozz::animation::offline::RawAnimation::TranslationKey key1 = {.93f, ozz::math::Float3(0.f, 9.9f, 0.f)};

		rawAnimation.tracks[0].translations.push_back(key1);
	}

	// Fills 1st track with a rotation keyframe. It's not mandatory to have the
	// same number of keyframes for translation, rotations and scales.
	{
		// Create a keyframe, at t=.46, with a quaternion value.
		const ozz::animation::offline::RawAnimation::RotationKey key0 = {.46f,
																		 ozz::math::Quaternion(0.f, 1.f, 0.f, 0.f)};

		rawAnimation.tracks[0].rotations.push_back(key0);
	}

	// For this example, don't fill scale with any key. The default value will be
	// identity, which is ozz::math::Float3(1.f, 1.f, 1.f) for scale.

	//...and so on with all other tracks...

	// Test for animation validity. These are the errors that could invalidate
	// an animation:
	//  1. Animation duration is less than 0.
	//  2. Keyframes' are not sorted in a strict ascending order.
	//  3. Keyframes' are not within [0, duration] range.
	if (!rawAnimation.Validate()) {
		Log::error("The animation is invalid");
		return {};
	}

	//////////////////////////////////////////////////////////////////////////////
	// This final section converts the RawAnimation to a runtime Animation.
	//////////////////////////////////////////////////////////////////////////////

	// Creates a AnimationBuilder instance.
	ozz::animation::offline::AnimationBuilder builder;

	// Executes the builder on the previously prepared RawAnimation, which returns
	// a new runtime animation instance.
	// This operation will fail and return an empty unique_ptr if the RawAnimation
	// isn't valid.
	return builder(rawAnimation);
}

ozz::unique_ptr<ozz::animation::Skeleton> TestAnim::assembleSkeleton() {
	ozz::animation::offline::RawSkeleton rawSkeleton;

	// Creates the root joint.
	rawSkeleton.roots.resize(1);
	ozz::animation::offline::RawSkeleton::Joint &root = rawSkeleton.roots[0];

	// Setup root joints name.
	root.name = "root";

	// Setup root joints rest pose transformation, in joint local-space.
	// This is the default skeleton posture (most of the time a T-pose). It's
	// used as a fallback when there's no animation for a joint.
	root.transform.translation = ozz::math::Float3(0.f, 1.f, 0.f);
	root.transform.rotation = ozz::math::Quaternion(0.f, 0.f, 0.f, 1.f);
	root.transform.scale = ozz::math::Float3(1.f, 1.f, 1.f);

	// Now adds 2 children to the root.
	root.children.resize(2);

	// Setups the 1st child name (left) and transformation.
	ozz::animation::offline::RawSkeleton::Joint &left = root.children[0];
	left.name = "left";
	left.transform.translation = ozz::math::Float3(1.f, 0.f, 0.f);
	left.transform.rotation = ozz::math::Quaternion(0.f, 0.f, 0.f, 1.f);
	left.transform.scale = ozz::math::Float3(1.f, 1.f, 1.f);

	// Setups the 2nd child name (right) and transformation.
	ozz::animation::offline::RawSkeleton::Joint &right = root.children[1];
	right.name = "right";
	right.transform.translation = ozz::math::Float3(-1.f, 0.f, 0.f);
	right.transform.rotation = ozz::math::Quaternion(0.f, 0.f, 0.f, 1.f);
	right.transform.scale = ozz::math::Float3(1.f, 1.f, 1.f);

	//...and so on with the whole skeleton hierarchy...

	// Test for skeleton validity.
	// The main invalidity reason is the number of joints, which must be lower
	// than ozz::animation::Skeleton::kMaxJoints.
	if (!rawSkeleton.Validate()) {
		Log::error("The skeleton is invalid");
		return {};
	}

	ozz::animation::offline::SkeletonBuilder builder;
	return builder(rawSkeleton);
}

app::AppState TestAnim::onInit() {
	app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}

	_skeleton = assembleSkeleton();
	if (!_skeleton) {
		Log::error("Failed to build the skeleton");
		return app::AppState::InitFailure;
	}

	_animation = assembleAnimation();
	if (!_animation) {
		Log::error("Failed to build the animation");
		return app::AppState::InitFailure;
	}

	if (_skeleton->num_joints() != _animation->num_tracks()) {
		Log::error("The number of joints and the number of tracks must be the same");
		return app::AppState::InitFailure;
	}

	if (!_shapeRenderer.init()) {
		Log::error("Failed to init the shape renderer");
		return app::AppState::InitFailure;
	}

	_localMatrices.resize(_skeleton->num_soa_joints());
	_modelMatrices.resize(_skeleton->num_joints());
	_context.Resize(_animation->num_tracks());
	return state;
}

void TestAnim::runAnimation() {
	// sample animation
	ozz::animation::SamplingJob samplingJob;
	samplingJob.animation = _animation.get();
	samplingJob.context = &_context;
	// convert current time to animation ration (0.0 .. 1.0)
	samplingJob.ratio = fmodf((float)_nowSeconds / _animation->duration(), 1.0f);
	samplingJob.output = ozz::make_span(_localMatrices);
	if (!samplingJob.Run()) {
		Log::error("Failed to sample animation");
		requestQuit();
	}

	// convert joint matrices from local to model space
	ozz::animation::LocalToModelJob localToModelJob;
	localToModelJob.skeleton = _skeleton.get();
	localToModelJob.input = ozz::make_span(_localMatrices);
	localToModelJob.output = ozz::make_span(_modelMatrices);
	if (!localToModelJob.Run()) {
		Log::error("Failed to convert local to model space");
		requestQuit();
	}
}

app::AppState TestAnim::onCleanup() {
	_shapeRenderer.shutdown();
	_shapeBuilder.shutdown();

	_skeleton = nullptr;
	return Super::onCleanup();
}

void TestAnim::buildLine(const ozz::math::SimdFloat4 &v0, const ozz::math::SimdFloat4 &v1) {
	const glm::vec3 v_0(ozz::math::GetX(v0), ozz::math::GetY(v0), ozz::math::GetZ(v0));
	const glm::vec3 v_1(ozz::math::GetX(v1), ozz::math::GetY(v1), ozz::math::GetZ(v1));
	_shapeBuilder.line(v_0, v_1);
}

void TestAnim::buildJoint(int jointIndex, int parentJointIndex) {
	if (parentJointIndex < 0) {
		return;
	}

	const ozz::math::Float4x4 &m0 = _modelMatrices[jointIndex];
	const ozz::math::Float4x4 &m1 = _modelMatrices[parentJointIndex];

	const ozz::math::SimdFloat4 p0 = m0.cols[3];
	const ozz::math::SimdFloat4 p1 = m1.cols[3];
	const ozz::math::SimdFloat4 ny = m1.cols[1];
	const ozz::math::SimdFloat4 nz = m1.cols[2];

	const ozz::math::SimdFloat4 len =
		ozz::math::SplatX(ozz::math::Length3(p1 - p0)) * ozz::math::simd_float4::Load1(0.1f);

	const ozz::math::SimdFloat4 middle = p0 + (p1 - p0) * ozz::math::simd_float4::Load1(0.66f);
	const ozz::math::SimdFloat4 p2 = middle + ny * len;
	const ozz::math::SimdFloat4 p3 = middle + nz * len;
	const ozz::math::SimdFloat4 p4 = middle - ny * len;
	const ozz::math::SimdFloat4 p5 = middle - nz * len;

	buildLine(p0, p2);
	buildLine(p0, p3);
	buildLine(p0, p4);
	buildLine(p0, p5);
	buildLine(p1, p2);
	buildLine(p1, p3);
	buildLine(p1, p4);
	buildLine(p1, p5);
	buildLine(p2, p3);
	buildLine(p3, p4);
	buildLine(p4, p5);
	buildLine(p5, p2);
}

void TestAnim::onRenderUI() {
	Super::onRenderUI();

	ImGui::Text("num joints: %i", _skeleton->num_joints());
}

void TestAnim::doRender() {
	runAnimation();

	const int numJoints = _skeleton->num_joints();
	ozz::span<const int16_t> jointParents = _skeleton->joint_parents();
	_shapeBuilder.clear();
	_shapeBuilder.setColor(core::Color::White());

	for (int jointIndex = 0; jointIndex < numJoints; jointIndex++) {
		buildJoint(jointIndex, jointParents[jointIndex]);
	}

	_shapeRenderer.createOrUpdate(_meshIndex, _shapeBuilder);
	_shapeRenderer.renderAll(camera());
}

TEST_APP(TestAnim)
