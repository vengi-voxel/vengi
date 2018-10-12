/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "render/ShapeRenderer.h"
#include "video/ShapeBuilder.h"

class TestShapeBuilder: public TestApp {
private:
	using Super = TestApp;
	mutable video::ShapeBuilder _shapeBuilder;
	render::ShapeRenderer _shapeRenderer;
	int _meshCount = 0;
	glm::ivec3 _position[render::ShapeRenderer::MAX_MESHES] {};
	glm::vec3 _scale[render::ShapeRenderer::MAX_MESHES] {};
	glm::vec4 _color;
	bool _near = false;
	float _stepWidth = 1.0f;
	glm::vec3 _mins {-10.0f, -10.0f, -10.0f};
	glm::vec3 _maxs { 10.0f,  10.0f,  10.0f};
	int _meshes[render::ShapeRenderer::MAX_MESHES] {-1};
	int _meshUnitCube = -1;
	struct {
		int numSlices = 5;
		int numStacks = 4;
		float radius = 20.0f;
	} _sphere;

	void doRender() override;
	void onRenderUI() override;
public:
	TestShapeBuilder(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	virtual core::AppState onInit() override;
	virtual core::AppState onCleanup() override;
};
