#include "TestCamera.h"

// TODO: zooming should update the far and near plane of the render camera (maybe alt + ctrl pressed)
// TODO: render the render camera frustum
// TODO: onMouseMotion for renderCamera (maybe also while ctrl or alt is held)
TestCamera::TestCamera(io::FilesystemPtr filesystem, core::EventBusPtr eventBus) :
		Super(filesystem, eventBus) {
	setCameraMotion(true);
	setRenderPlane(false);
}

core::AppState TestCamera::onInit() {
	core::AppState state = Super::onInit();

	if (!_colorShader.setup()) {
		return core::AppState::Cleanup;
	}

	_renderCamera.init(dimension());
	_renderCamera.setAspectRatio(_aspect);
	_renderCamera.setRotationType(video::CameraRotationType::Target);
	_renderCamera.setPosition(glm::vec3(1.0f, 10.0f, 1.0f));
	_renderCamera.setTarget(glm::vec3(10.0f, 70.0f, 10.0f));
	_renderCamera.update(0l);

	_camera.setPosition(glm::vec3(100.0f, 100.0f, 100.0f));
	_camera.lookAt(_renderCamera.target());

	glm::vec3 out[8];
	_renderCamera.frustumCorners(out);
	_frustumIndex = _frustumBuffer.create(out, sizeof(out));

	std::vector<glm::vec4> colors(SDL_arraysize(out));
	std::fill(colors.begin(), colors.end(), core::Color::Red);

	const int32_t cIndex = _frustumBuffer.create(&colors[0], colors.size() * sizeof(glm::vec4));

	_frustumBuffer.addAttribute(_colorShader.getLocationPos(), _frustumIndex, 4);
	_frustumBuffer.addAttribute(_colorShader.getLocationColor(), cIndex, 4);

	return state;
}

core::AppState TestCamera::onCleanup() {
	core::AppState state = Super::onCleanup();
	_colorShader.shutdown();
	_frustumBuffer.shutdown();
	return state;
}

void TestCamera::doRender() {
	glm::vec3 out[8];
	_renderCamera.frustumCorners(out);
	video::ScopedShader scoped(_colorShader);
	_colorShader.setView(_camera.viewMatrix());
	_colorShader.setProjection(_camera.projectionMatrix());
	core_assert_always(_frustumBuffer.bind());
	glDrawArrays(GL_POINTS, 0, _frustumBuffer.elements(_frustumIndex));
	_frustumBuffer.unbind();
	GL_checkError();
}

int main(int argc, char *argv[]) {
	return core::getApp<TestCamera>()->startMainLoop(argc, argv);
}
