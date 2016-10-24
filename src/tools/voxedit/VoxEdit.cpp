/**
 * @file
 */

#include "VoxEdit.h"
#include "Actions.h"
#include "ui/VoxEditUI.h"
#include "core/Common.h"
#include "core/Var.h"
#include "video/GLDebug.h"
#include "video/ScopedViewPort.h"
#include "core/Color.h"
#include "frontend/Movement.h"
#include "voxel/polyvox/Picking.h"

VoxEdit::VoxEdit(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		ui::UIApp(filesystem, eventBus, timeProvider), _rawVolumeRenderer(true) {
	init("engine", "voxedit");
}

bool VoxEdit::saveFile(std::string_view file) {
	if (!_dirty) {
		// nothing to save yet
		return true;
	}
	_dirty = false;
	// TODO
	return false;
}

bool VoxEdit::loadFile(std::string_view file) {
	// TODO
	_extract = true;
	_dirty = false;
	return false;
}

bool VoxEdit::newFile(bool force) {
	if (_dirty && !force) {
		return false;
	}
	_dirty = false;
	const int size = 64;
	const voxel::Region region(glm::ivec3(0), glm::ivec3(size));
	voxel::RawVolume* old = _rawVolumeRenderer.setVolume(new voxel::RawVolume(region));
	delete old;
	// TODO
	return false;
}

void VoxEdit::onWindowResize() {
	Super::onWindowResize();
	_camera.init(dimension());
}

core::AppState VoxEdit::onInit() {
	const core::AppState state = Super::onInit();
	if (!_axis.init()) {
		return core::AppState::Cleanup;
	}

	if (!_plane.init(glm::zero<glm::vec3>())) {
		return core::AppState::Cleanup;
	}

	if (!_rawVolumeRenderer.init()) {
		return core::AppState::Cleanup;
	}

	_lastDirectory = core::Var::get("ve_lastdirectory", core::App::getInstance()->filesystem()->homePath().c_str());
	_rotationSpeed = core::Var::get(cfg::ClientMouseRotationSpeed, "0.01");

	_camera.init(dimension());
	_camera.setPosition(glm::vec3(0.0f, 50.0f, 100.0f));
	_camera.lookAt(glm::vec3(0.0001f));

	registerMoveCmd("+move_right", MOVERIGHT);
	registerMoveCmd("+move_left", MOVELEFT);
	registerMoveCmd("+move_forward", MOVEFORWARD);
	registerMoveCmd("+move_backward", MOVEBACKWARD);

	registerWindows(this);
	registerActions(this, _lastDirectory);

	// TODO: if tmpfile exists, load that one
	newFile();

	const glm::vec4& color = ::core::Color::Black;
	glClearColor(color.r, color.g, color.b, color.a);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glDepthMask(GL_TRUE);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	SDL_SetRelativeMouseMode(SDL_FALSE);

	_currentVoxel = voxel::createVoxel(voxel::Grass1);

	return state;
}

void VoxEdit::beforeUI() {
	Super::beforeUI();

	if (_cameraMotion) {
		const bool current = SDL_GetRelativeMouseMode();
		if (current) {
			SDL_WarpMouseInWindow(_window, width() / 2, height() / 2);
		}
	}

	const float speed = _cameraSpeed * static_cast<float>(_deltaFrame);
	const glm::vec3& moveDelta = getMoveDelta(speed, _moveMask);
	_camera.move(moveDelta);

	_camera.update(_deltaFrame);

	if  (_renderPlane) {
		_plane.render(_camera);
	}
	_rawVolumeRenderer.render(_camera);
	// TODO: render rawvolume planes
	if (_renderAxis) {
		_axis.render(_camera);
	}
}

core::AppState VoxEdit::onRunning() {
	if (_extract) {
		_extract = false;
		_rawVolumeRenderer.extract();
	}
	return Super::onRunning();
}

core::AppState VoxEdit::onCleanup() {
	_axis.shutdown();
	_plane.shutdown();
	voxel::RawVolume* old = _rawVolumeRenderer.shutdown();
	delete old;

	core::Command::unregisterCommand("+move_right");
	core::Command::unregisterCommand("+move_left");
	core::Command::unregisterCommand("+move_upt");
	core::Command::unregisterCommand("+move_down");

	// TODO: saveFile(tmpFilename);
	// TODO: cvar with tmpFilename to load on next start

	return Super::onCleanup();
}

bool VoxEdit::onKeyPress(int32_t key, int16_t modifier) {
	if (key == SDLK_ESCAPE) {
		const SDL_bool current = SDL_GetRelativeMouseMode();
		const SDL_bool mode = current ? SDL_FALSE : SDL_TRUE;
		SDL_SetRelativeMouseMode(mode);
		if (mode) {
			_root.SetVisibility(tb::WIDGET_VISIBILITY::WIDGET_VISIBILITY_INVISIBLE);
		} else {
			_root.SetVisibility(tb::WIDGET_VISIBILITY::WIDGET_VISIBILITY_VISIBLE);
		}
	}
	return Super::onKeyPress(key, modifier);
}

void VoxEdit::onMouseWheel(int32_t x, int32_t y) {
	Super::onMouseWheel(x, y);
	const float targetDistance = glm::clamp(_camera.targetDistance() - y, 0.0f, 500.0f);
	_camera.setTargetDistance(targetDistance);
}

void VoxEdit::onMouseButtonPress(int32_t x, int32_t y, uint8_t button) {
	Super::onMouseButtonPress(x, y, button);

	voxel::RawVolume* rawVolume = _rawVolumeRenderer.volume();
	if (rawVolume == nullptr) {
		return;
	}

	const bool placeVoxel = button == SDL_BUTTON_LEFT;
	const bool deleteVoxel = button == SDL_BUTTON_RIGHT;
	const bool copyVoxel = button == SDL_BUTTON_MIDDLE;

	voxel::Voxel voxel;
	const video::Ray& ray = _camera.mouseRay(glm::ivec2(x, y));
	const glm::vec3 dirWithLength = ray.direction * _camera.farPlane();
	const voxel::PickResult& result = voxel::pickVoxel(rawVolume, ray.origin, dirWithLength, voxel::createVoxel(voxel::Air));
	bool extract = false;
	if (result.didHit) {
		if (copyVoxel) {
			_currentVoxel = rawVolume->getVoxel(result.hitVoxel);
		} else if (placeVoxel) {
			extract = rawVolume->setVoxel(result.hitVoxel, _currentVoxel);
		} else if (deleteVoxel) {
			extract = rawVolume->setVoxel(result.hitVoxel, voxel::createVoxel(voxel::Air));
		}
	} else {
		// TODO: intersection with the plane of the rawvolume
		Log::info("didn't hit voxel");
		int y = 0;
		do {
			if (y >= rawVolume->getEnclosingRegion().getHeightInCells()) {
				break;
			}
			extract = rawVolume->setVoxel(glm::ivec3(0, ++y, 0), _currentVoxel);
		} while (!extract);
	}
	if (extract) {
		Log::info("placed voxel");
	}
	_extract |= extract;
	_dirty |= extract;
}

void VoxEdit::onMouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) {
	Super::onMouseMotion(x, y, relX, relY);
	if (_cameraMotion) {
		const bool current = SDL_GetRelativeMouseMode();
		if (!current) {
			return;
		}
		_camera.rotate(glm::vec3(relY, relX, 0.0f) * _rotationSpeed->floatVal());
	}
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr timeProvider = std::make_shared<core::TimeProvider>();
	VoxEdit app(filesystem, eventBus, timeProvider);
	return app.startMainLoop(argc, argv);
}
