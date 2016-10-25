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
#include "video/ScopedPolygonMode.h"
#include "voxel/VoxLoader.h"

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
	const io::FilePtr& filePtr = core::App::getInstance()->filesystem()->open(std::string(file));
	if (!(bool)filePtr) {
		return false;
	}
	voxel::VoxLoader loader;
	voxel::RawVolume* newVolume = loader.load(filePtr);
	if (newVolume == nullptr) {
		return false;
	}
	voxel::RawVolume* old = _rawVolumeRenderer.setVolume(newVolume);
	delete old;
	_extract = true;
	_dirty = false;
	return true;
}

bool VoxEdit::newFile(bool force) {
	if (_dirty && !force) {
		return false;
	}
	_dirty = false;
	const int size = 64;
	const voxel::Region region(glm::ivec3(0), glm::ivec3(size));
	voxel::RawVolume* volume = new voxel::RawVolume(region);
	voxel::RawVolume* old = _rawVolumeRenderer.setVolume(volume);
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
	newFile(true);

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

	{
		video::ScopedPolygonMode polygonMode(_camera.polygonMode());
		_rawVolumeRenderer.render(_camera);
	}
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

void VoxEdit::onMouseButtonRelease(int32_t x, int32_t y, uint8_t button) {
	Super::onMouseButtonRelease(x, y, button);
	_action = Action::None;
}

void VoxEdit::onMouseButtonPress(int32_t x, int32_t y, uint8_t button) {
	Super::onMouseButtonPress(x, y, button);
	_action = Action::None;

	const bool placeVoxel = button == SDL_BUTTON_LEFT;
	const bool deleteVoxel = button == SDL_BUTTON_RIGHT;
	const bool copyVoxel = button == SDL_BUTTON_MIDDLE;

	if (!placeVoxel && !deleteVoxel && !copyVoxel) {
		return;
	}

	if (placeVoxel) {
		_action = Action::PlaceVoxel;
	} else if (deleteVoxel) {
		_action = Action::DeleteVoxel;
	} else if (copyVoxel) {
		_action = Action::CopyVoxel;
	}

	executeAction(x, y);
}

void VoxEdit::onMouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) {
	Super::onMouseMotion(x, y, relX, relY);
	if (_cameraMotion) {
		const bool current = SDL_GetRelativeMouseMode();
		if (current) {
			_camera.rotate(glm::vec3(relY, relX, 0.0f) * _rotationSpeed->floatVal());
		}
	}

	executeAction(x, y);
}

void VoxEdit::executeAction(int32_t x, int32_t y) {
	if (_action == Action::None) {
		return;
	}

	voxel::RawVolume* volume = _rawVolumeRenderer.volume();
	if (volume == nullptr) {
		return;
	}

	voxel::Voxel voxel;
	const video::Ray& ray = _camera.mouseRay(glm::ivec2(x, y));
	const glm::vec3 dirWithLength = ray.direction * _camera.farPlane();
	const voxel::PickResult& result = voxel::pickVoxel(volume, ray.origin, dirWithLength, voxel::createVoxel(voxel::Air));
	bool extract = false;
	const bool override = SDL_GetModState() & KMOD_CTRL;
	if (result.didHit && _action == Action::CopyVoxel) {
		_currentVoxel = volume->getVoxel(result.hitVoxel);
	} else if (result.didHit && _action == Action::PlaceVoxel && override) {
		extract = volume->setVoxel(result.hitVoxel, _currentVoxel);
	} else if (result.didHit && _action == Action::DeleteVoxel) {
		extract = volume->setVoxel(result.hitVoxel, voxel::createVoxel(voxel::Air));
	} else if (result.validPreviousVoxel && _action == Action::PlaceVoxel) {
		extract = volume->setVoxel(result.previousVoxel, _currentVoxel);
	}

	_extract |= extract;
	_dirty |= extract;
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr timeProvider = std::make_shared<core::TimeProvider>();
	VoxEdit app(filesystem, eventBus, timeProvider);
	return app.startMainLoop(argc, argv);
}
