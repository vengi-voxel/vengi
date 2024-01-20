/**
 * @file
 */

#pragma once

#include "ozz/animation/runtime/sampling_job.h"
#include "render/ShapeRenderer.h"
#include "testcore/TestApp.h"
#include "video/ShapeBuilder.h"

#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/memory/unique_ptr.h>

class TestAnim : public TestApp {
private:
	using Super = TestApp;

	mutable video::ShapeBuilder _shapeBuilder;
	render::ShapeRenderer _shapeRenderer;
	ozz::unique_ptr<ozz::animation::Skeleton> _skeleton;
	ozz::unique_ptr<ozz::animation::Animation> _animation;
	ozz::vector<ozz::math::SoaTransform> _localMatrices;
	ozz::vector<ozz::math::Float4x4> _modelMatrices;
	ozz::animation::SamplingJob::Context _context;
	int32_t _meshIndex = -1;

	ozz::unique_ptr<ozz::animation::Animation> assembleAnimation();
	ozz::unique_ptr<ozz::animation::Skeleton> assembleSkeleton();

	void buildJoint(int jointIndex, int parentJointIndex);
	void buildLine(const ozz::math::SimdFloat4 &v0, const ozz::math::SimdFloat4 &v1);

	void doRender() override;
	void onRenderUI() override;

	void runAnimation();

public:
	TestAnim(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider);

	virtual app::AppState onInit() override;
	virtual app::AppState onCleanup() override;
};
