#include "TestCamera.h"

TestCamera::TestCamera(io::FilesystemPtr filesystem, core::EventBusPtr eventBus) :
		Super(filesystem, eventBus) {
	setCameraMotion(true);
	setRenderPlane(false);
	setRenderAxis(true);
}

core::AppState TestCamera::onInit() {
	core::AppState state = Super::onInit();
	const glm::vec4 colors[CAMERAS] = { core::Color::Red, core::Color::Yellow };
	static_assert(CAMERAS == 2, "Unexpected amount of cameras");
	for (int i = 0; i < CAMERAS; ++i) {
		const float p = i * 10.0f + 1.0f;
		_renderCamera[i].init(dimension());
		_renderCamera[i].setAspectRatio(_aspect);
		_renderCamera[i].setOmega(glm::vec3(0.0f, 0.1f, 0.0f));

		// TODO: per camera settings
		_renderCamera[i].setPosition(glm::vec3(p, 10.0f, p));
		_renderCamera[i].lookAt(glm::vec3(10.0f, 70.0f, 10.0f));
		_renderCamera[i].setNearPlane(5.0f);
		_renderCamera[i].setFarPlane(40.0f);

		_renderCamera[i].update(0l);

		if (!_frustums[i].init(_renderCamera[i], colors[i])) {
			return core::AppState::Cleanup;
		}
		_frustums[i].setRenderAABB(true);
	}

	_camera.setRotationType(video::CameraRotationType::Target);
	_camera.setTarget(_renderCamera[_targetCamera].position());

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
	for (int i = 0; i < CAMERAS; ++i) {
		_frustums[i].shutdown();
	}
	return state;
}

void TestCamera::onMouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) {
	const SDL_Keymod mods = SDL_GetModState();
	if (mods & KMOD_SHIFT) {
		_renderCamera[_targetCamera].rotate(glm::vec3(relY, relX, 0.0f) * _rotationSpeed->floatVal());
		return;
	}
	Super::onMouseMotion(x, y, relX, relY);
}

void TestCamera::onMouseWheel(int32_t x, int32_t y) {
	const SDL_Keymod mods = SDL_GetModState();
	if (mods & KMOD_SHIFT) {
		video::Camera& c = _renderCamera[_targetCamera];
		if (mods & KMOD_CTRL) {
			c.setNearPlane(c.nearPlane() + y);
		} else {
			c.setFarPlane(c.farPlane() + y);
		}
		return;
	}

	Super::onMouseWheel(x, y);
}

bool TestCamera::onKeyPress(int32_t key, int16_t modifier) {
	const bool retVal = Super::onKeyPress(key, modifier);
	if (key == SDLK_SPACE) {
		_targetCamera++;
		_targetCamera %= CAMERAS;
	}

	video::Camera& c = _renderCamera[_targetCamera];
	if (modifier & KMOD_SHIFT) {
		int delta = 0;
		if (key == SDLK_MINUS || key == SDLK_KP_MINUS) {
			delta = -1;
		} else if (key == SDLK_PLUS || key == SDLK_KP_PLUS) {
			delta = 1;
		}

		if (modifier & KMOD_CTRL) {
			c.setNearPlane(c.nearPlane() + delta);
		} else {
			c.setFarPlane(c.farPlane() + delta);
		}
		if (delta != 0) {
			return true;
		}
	}

	return retVal;
}

void TestCamera::doRender() {
	for (int i = 0; i < CAMERAS; ++i) {
		_frustums[i].render(_camera, _renderCamera[i]);
	}
}

int main(int argc, char *argv[]) {
	return core::getApp<TestCamera>()->startMainLoop(argc, argv);
}
