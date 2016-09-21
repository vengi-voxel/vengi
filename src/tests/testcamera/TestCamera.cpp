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
		const float p = i * 100.0f + 1.0f;
		_renderCamera[i].init(dimension());
		_renderCamera[i].setAspectRatio(_aspect);
		_renderCamera[i].setOmega(glm::vec3(0.0f, 0.1f, 0.0f));

		// TODO: per camera settings
		_renderCamera[i].setPosition(glm::vec3(p, 10.0f, p));
		_renderCamera[i].lookAt(glm::vec3(10.0f, 70.0f, 10.0f));
		_renderCamera[i].setNearPlane(5.0f);
		_renderCamera[i].setFarPlane(40.0f);

		_renderCamera[i].update(0l);

		bool renderAABB = i == 0;
		bool renderSplitFrustum = !renderAABB;
		bool targetCamera = i == 0;
		if (targetCamera) {
			_renderCamera[i].setRotationType(video::CameraRotationType::Target);
		}
		if (!_frustums[i].init(_renderCamera[i], colors[i], renderSplitFrustum ? 4 : 0)) {
			return core::AppState::Cleanup;
		}
		_frustums[i].setRenderAABB(renderAABB);
	}

	_camera.setRotationType(video::CameraRotationType::Target);
	_camera.setTarget(_renderCamera[_targetCamera].position());
	_camera.setTargetDistance(200.0f);

	return state;
}

void TestCamera::doRender() {
	for (int i = 0; i < CAMERAS; ++i) {
		_renderCamera[i].update(_deltaFrame);
	}
	for (int i = 0; i < CAMERAS; ++i) {
		_frustums[i].render(_camera, _renderCamera[i]);
	}
}

void TestCamera::afterUI() {
	Super::afterUI();
	tb::TBStr str;
	const char *cameraRotType;
	const video::Camera& targetCamera = _renderCamera[_targetCamera];
	video::CameraRotationType rotType = targetCamera.rotationType();
	switch (rotType) {
	case video::CameraRotationType::Target:
		cameraRotType = "R: Target";
		break;
	case video::CameraRotationType::Eye:
		cameraRotType = "R: Eye";
		break;
	}
	const char *mode = "";
	if (_targetCamera == 1) {
		mode = "Frustum split";
	}
	enqueueShowStr(5, core::Color::White, "Camera: %s (%i) %s", cameraRotType, _targetCamera + 1, mode);
	if (_frustums[_targetCamera].renderAABB()) {
		const core::AABB<float>&& aabb = targetCamera.aabb();
		enqueueShowStr(5, core::Color::White, "AABB(mins(%.2f:%.2f:%.2f), maxs(%.2f:%.2f:%.2f))", aabb.getLowerX(), aabb.getLowerY(), aabb.getLowerZ(), aabb.getUpperX(), aabb.getUpperY(), aabb.getUpperZ());
	} else {
		enqueueShowStr(5, core::Color::White, "");
	}
	enqueueShowStr(5, core::Color::Gray, "Space: toggle camera");
	enqueueShowStr(5, core::Color::Gray, "Shift/MouseMove: rotate");
	enqueueShowStr(5, core::Color::Gray, "Backspace: toggle aabb");
	enqueueShowStr(5, core::Color::Gray, "Shift/+ Shift/-: far plane");
	enqueueShowStr(5, core::Color::Gray, "Ctrl/Shift/+ Ctrl/Shift/-: near plane");
	enqueueShowStr(5, core::Color::Gray, "Shift/MouseWheel: far plane");
	enqueueShowStr(5, core::Color::Gray, "Ctrl/Shift/MouseWheel: near plane");
}

core::AppState TestCamera::onRunning() {
	core::AppState state = Super::onRunning();
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

	if (key == SDLK_BACKSPACE) {
		const bool aabb = _frustums[_targetCamera].renderAABB();
		_frustums[_targetCamera].setRenderAABB(!aabb);
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

int main(int argc, char *argv[]) {
	const core::EventBusPtr eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr filesystem = std::make_shared<io::Filesystem>();
	TestCamera app(filesystem, eventBus);
	return app.startMainLoop(argc, argv);
}
