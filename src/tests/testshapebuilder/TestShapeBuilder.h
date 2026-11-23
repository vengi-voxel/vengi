/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "render/ShapeRenderer.h"
#include "video/ShapeBuilder.h"
#include "math/Vec4f.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"

class TestShapeBuilder: public TestApp {
private:
	using Super = TestApp;
	mutable video::ShapeBuilder _shapeBuilder;
	render::ShapeRenderer _shapeRenderer;
	int _meshCount = 0;
	math::Vec3i _position[render::ShapeRenderer::MAX_MESHES] {};
	math::Vec3f _scale[render::ShapeRenderer::MAX_MESHES] {};
	math::Vec3f _rotation[render::ShapeRenderer::MAX_MESHES] {};
	math::Vec4f _color;
	bool _near = false;
	float _stepWidth = 1.0f;
	math::Vec3f _mins {-10.0f, -10.0f, -10.0f};
	math::Vec3f _maxs { 10.0f,  10.0f,  10.0f};
	int _meshes[render::ShapeRenderer::MAX_MESHES] {-1};
	int _meshUnitCube = -1;
	struct {
		int numSlices = 5;
		int numStacks = 4;
		float radius = 20.0f;
	} _sphere;
	struct {
		float length = 20.0f;
		float posSize = 1.0f;
		float boneSize = 4.0f;
	} _bone;
	struct {
		math::Vec3f start {0.0f};
		math::Vec3f end {10.0f};
	} _line;

	void doRender() override;
	void onRenderUI() override;
public:
	TestShapeBuilder(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider);

	virtual app::AppState onInit() override;
	virtual app::AppState onCleanup() override;
};
