/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "frontend/ShapeRenderer.h"
#include "video/ShapeBuilder.h"

class TestShapeBuilder: public TestApp {
private:
	using Super = TestApp;
	mutable video::ShapeBuilder _shapeBuilder;
	frontend::ShapeRenderer _shapeRenderer;
	int _meshCount = 0;
	glm::ivec3 _position[frontend::ShapeRenderer::MAX_MESHES] {};
	glm::vec3 _scale[frontend::ShapeRenderer::MAX_MESHES] {};
	glm::vec4 _color;
	bool _near = false;
	float _stepWidth = 1.0f;
	glm::vec3 _mins {-10.0f, -10.0f, -10.0f};
	glm::vec3 _maxs { 10.0f,  10.0f,  10.0f};
	int _meshes[frontend::ShapeRenderer::MAX_MESHES] {-1};
	int _meshUnitCube = -1;

	void doRender() override;
	void onRenderUI() override;
public:
	TestShapeBuilder(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	virtual core::AppState onInit() override;
	virtual core::AppState onCleanup() override;
};
