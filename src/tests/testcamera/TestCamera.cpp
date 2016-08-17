#include "TestCamera.h"

// TODO: zooming should update the far and near plane of the render camera (maybe alt + ctrl pressed)
// TODO: render the render camera frustum
// TODO: onMouseMotion for renderCamera (maybe also while ctrl or alt is held)
TestCamera::TestCamera(io::FilesystemPtr filesystem, core::EventBusPtr eventBus) :
		Super(filesystem, eventBus) {
	setCameraMotion(true);
	setRenderPlane(false);
	setRenderAxis(true);
}

core::AppState TestCamera::onInit() {
	core::AppState state = Super::onInit();

	if (!_colorShader.setup()) {
		return core::AppState::Cleanup;
	}

	for (int i = 0; i < CAMERAS; ++i) {
		const float p = i * 10.0f + 1.0f;
		_renderCamera[i].init(dimension());
		_renderCamera[i].setAspectRatio(_aspect);
		_renderCamera[i].setRotationType(video::CameraRotationType::Target);
		_renderCamera[i].setPosition(glm::vec3(p, 10.0f, p));
		_renderCamera[i].setOmega(glm::vec3(0.0f, 0.001f, 0.0f));
		_renderCamera[i].setTarget(glm::vec3(10.0f, 70.0f, 10.0f));
		_renderCamera[i].setNearPlane(5.0f);
		_renderCamera[i].setFarPlane(40.0f);
		_renderCamera[i].update(0l);
	}

	_camera.setRotationType(video::CameraRotationType::Target);
	_camera.setTarget(_renderCamera[0].position());

	// allocate buffer space
	glm::vec3 out[video::FRUSTUM_VERTICES_MAX];
	glm::vec4 out4[video::FRUSTUM_VERTICES_MAX];
	glm::vec4 colors[video::FRUSTUM_VERTICES_MAX];
	uint32_t indices[24];

	// fill buffers
	glm::vec4 color = core::Color::Red;
	for (size_t i = 0; i < video::FRUSTUM_VERTICES_MAX; ++i) {
		colors[i] = color;
		out4[i] = glm::vec4(out[i], 1.0f);
		color = core::Color::Brighter(color, 0.5f);
	}

	for (int i = 0; i < CAMERAS; ++i) {
		_renderCamera[i].frustumCorners(out, indices);

		// upload to gpu
		_vertexIndex[i] = _frustumBuffer[i].create(out4, sizeof(out4));
		_indexIndex[i] = _frustumBuffer[i].create(indices, sizeof(indices), GL_ELEMENT_ARRAY_BUFFER);
		const int32_t cIndex = _frustumBuffer[i].create(colors, sizeof(colors));

		// configure shader attributes
		_frustumBuffer[i].addAttribute(_colorShader.getLocationPos(), _vertexIndex[i], 4);
		_frustumBuffer[i].addAttribute(_colorShader.getLocationColor(), cIndex, 4);
	}
	return state;
}

core::AppState TestCamera::onRunning() {
	core::AppState state = Super::onRunning();
	for (int i = 0; i < CAMERAS; ++i) {
		_renderCamera[i].update(_deltaFrame);
	}
	_camera.setTarget(_renderCamera[_targetCamera].position());
	return state;
}

core::AppState TestCamera::onCleanup() {
	core::AppState state = Super::onCleanup();
	_colorShader.shutdown();
	for (int i = 0; i < CAMERAS; ++i) {
		_frustumBuffer[i].shutdown();
	}
	return state;
}

void TestCamera::doRender() {
	video::ScopedShader scoped(_colorShader);
	_colorShader.setView(_camera.viewMatrix());
	_colorShader.setProjection(_camera.projectionMatrix());

	// update the vertex buffer, because the reference camera might have changed
	glm::vec3 out[video::FRUSTUM_VERTICES_MAX];
	glm::vec4 out4[video::FRUSTUM_VERTICES_MAX];
	for (int i = 0; i < CAMERAS; ++i) {
		_renderCamera[i].frustumCorners(out, nullptr);
		for (size_t i = 0; i < video::FRUSTUM_VERTICES_MAX; ++i) {
			out4[i] = glm::vec4(out[i], 1.0f);
		}
		_frustumBuffer[i].update(_vertexIndex[i], out4, sizeof(out4));

		core_assert_always(_frustumBuffer[i].bind());
		const GLuint indices = _frustumBuffer[i].elements(_indexIndex[i], 1, sizeof(uint32_t));
		glDrawElements(GL_LINES, indices, GL_UNSIGNED_INT, 0);
		_frustumBuffer[i].unbind();
	}
	GL_checkError();
}

int main(int argc, char *argv[]) {
	return core::getApp<TestCamera>()->startMainLoop(argc, argv);
}
