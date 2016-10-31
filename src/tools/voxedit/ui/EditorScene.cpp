#include "EditorScene.h"
#include "frontend/Movement.h"
#include "core/Common.h"
#include "core/Var.h"
#include "video/GLDebug.h"
#include "video/ScopedViewPort.h"
#include "core/Color.h"
#include "voxel/polyvox/Picking.h"
#include "video/ScopedPolygonMode.h"
#include "video/ScopedScissor.h"
#include "video/ScopedFrameBuffer.h"
#include "voxel/VoxFormat.h"
#include "ui/UIApp.h"

EditorScene::EditorScene() :
		ui::Widget(), _rawVolumeRenderer(true), _bitmap((tb::UIRendererGL*)tb::g_renderer) {
	_currentVoxel = voxel::createVoxel(voxel::Grass1);
	SetIsFocusable(true);
}

EditorScene::~EditorScene() {
	_axis.shutdown();
	_frameBuffer.shutdown();
	voxel::RawVolume* old = _rawVolumeRenderer.shutdown();
	delete old;
}

void EditorScene::render() {
	{
		video::ScopedPolygonMode polygonMode(_camera.polygonMode());
		_rawVolumeRenderer.render(_camera);
	}
	if (_renderAxis) {
		_axis.render(_camera);
	}
}

void EditorScene::executeAction(int32_t x, int32_t y) {
	if (_action == Action::None) {
		return;
	}

	voxel::RawVolume* volume = _rawVolumeRenderer.volume();
	if (volume == nullptr) {
		return;
	}

	const long now = core::App::getInstance()->currentMillis();
	if (_lastAction == _action) {
		if (now - _lastActionExecution < _actionExecutionDelay) {
			return;
		}
	}
	_lastAction = _action;
	_lastActionExecution = now;

	voxel::Voxel voxel;
	const video::Ray& ray = _camera.mouseRay(glm::ivec2(x, y));
	const glm::vec3 dirWithLength = ray.direction * _camera.farPlane();
	const voxel::PickResult& result = voxel::pickVoxel(volume, ray.origin, dirWithLength, voxel::createVoxel(voxel::Air));
	bool extract = false;
	if (result.didHit && _action == Action::CopyVoxel) {
		_currentVoxel = volume->getVoxel(result.hitVoxel);
	} else if (result.didHit && _action == Action::OverrideVoxel) {
		extract = volume->setVoxel(result.hitVoxel, _currentVoxel);
	} else if (result.didHit && _action == Action::DeleteVoxel) {
		extract = volume->setVoxel(result.hitVoxel, voxel::createVoxel(voxel::Air));
	} else if (result.validPreviousVoxel && _action == Action::PlaceVoxel) {
		extract = volume->setVoxel(result.previousVoxel, _currentVoxel);
	}

	_extract |= extract;
	_dirty |= extract;
}

EditorScene::Action EditorScene::action() const {
	return _uiAction;
}

void EditorScene::setAction(EditorScene::Action action) {
	_uiAction = action;
}

bool EditorScene::newModel(bool force) {
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

bool EditorScene::saveModel(std::string_view file) {
	if (!_dirty) {
		// nothing to save yet
		return true;
	}
	voxel::RawVolume* volume = _rawVolumeRenderer.volume();
	if (volume == nullptr) {
		return false;
	}
	const io::FilePtr& filePtr = core::App::getInstance()->filesystem()->open(std::string(file));
	if (voxel::VoxFormat::save(volume, filePtr)) {
		_dirty = false;
	}
	return !_dirty;
}

bool EditorScene::loadModel(std::string_view file) {
	const io::FilePtr& filePtr = core::App::getInstance()->filesystem()->open(std::string(file));
	if (!(bool)filePtr) {
		return false;
	}
	voxel::RawVolume* newVolume = voxel::VoxFormat::load(filePtr);
	if (newVolume == nullptr) {
		return false;
	}
	voxel::RawVolume* old = _rawVolumeRenderer.setVolume(newVolume);
	delete old;
	_extract = true;
	_dirty = false;
	return true;
}

void EditorScene::resetCamera() {
	_camera.setAngles(0.0f, 0.0f, 0.0f);
	_camera.setPosition(glm::vec3(50.0f, 50.0f, 100.0f));
	_camera.lookAt(glm::vec3(0.0001f));
}

bool EditorScene::OnEvent(const tb::TBWidgetEvent &ev) {
	const int x = ev.target_x;
	const int y = ev.target_y;
	ui::UIRect rect = GetRect();
	ConvertToRoot(rect.x, rect.y);
	const int tx = x + rect.x;
	const int ty = y + rect.y;
	if (ev.type == tb::EVENT_TYPE_POINTER_DOWN) {
		//Log::info("x: %i, y: %i, rect.x: %i, rect.y: %i", x, y, rect.x, rect.y);
		if (ev.modifierkeys & tb::TB_ALT) {
			_action = Action::CopyVoxel;
		} else if (ev.modifierkeys & tb::TB_SHIFT) {
			_action = Action::OverrideVoxel;
		} else if (ev.modifierkeys & tb::TB_CTRL) {
			_action = Action::DeleteVoxel;
		} else {
			_action = _uiAction;
		}
		executeAction(tx, ty);
		return true;
	} else if (ev.type == tb::EVENT_TYPE_POINTER_UP) {
		_action = Action::None;
		executeAction(tx, ty);
		return true;
	} else if (ev.type == tb::EVENT_TYPE_POINTER_MOVE) {
		const bool current = SDL_GetRelativeMouseMode();
		if (current) {
			const float yaw = x - _mouseX;
			const float pitch = y - _mouseY;
			const float s = _rotationSpeed->floatVal();
			_camera.turn(yaw * s);
			_camera.pitch(pitch * s);
		} else {
			const int deltaX = x - _mouseX;
			const int deltaY = y - _mouseY;
			const int minMove = 2;
			// prevent micro movement from executing the action over and over again
			if (deltaX <= minMove && deltaY <= minMove) {
				return Super::OnEvent(ev);
			}
		}
		_mouseX = x;
		_mouseY = y;
		executeAction(tx, ty);
		return true;
	}
	return Super::OnEvent(ev);
}

void EditorScene::OnPaint(const PaintProps &paintProps) {
	Super::OnPaint(paintProps);
	const glm::ivec2& dimension = _frameBuffer.dimension();
	ui::UIRect rect = GetRect();
	int x = rect.x;
	int y = rect.y;
	ConvertToRoot(x, y);
	// the fbo is flipped in memory, we have to deal with it here
	const tb::TBRect srcRect(x, dimension.y - y, rect.w, -rect.h);
	tb::g_renderer->DrawBitmap(rect, srcRect, &_bitmap);
}

void EditorScene::OnInflate(const tb::INFLATE_INFO &info) {
	Super::OnInflate(info);
	_axis.init();

	_rawVolumeRenderer.init();
	_rotationSpeed = core::Var::get(cfg::ClientMouseRotationSpeed, "0.01");
	const ui::UIApp* app = (ui::UIApp*)core::App::getInstance();
	const glm::ivec2& d = app->dimension();
	_camera.init(glm::ivec2(), d);
	_frameBuffer.init(d);
	_bitmap.Init(d.x, d.y, _frameBuffer.texture());
	_rawVolumeRenderer.onResize(glm::ivec2(), d);

	resetCamera();

	registerMoveCmd("+move_right", MOVERIGHT);
	registerMoveCmd("+move_left", MOVELEFT);
	registerMoveCmd("+move_forward", MOVEFORWARD);
	registerMoveCmd("+move_backward", MOVEBACKWARD);
}

void EditorScene::OnProcess() {
	const long deltaFrame = core::App::getInstance()->deltaFrame();
	const float speed = _cameraSpeed * static_cast<float>(deltaFrame);
	const glm::vec3& moveDelta = getMoveDelta(speed, _moveMask);
	_camera.move(moveDelta);
	_camera.update(deltaFrame);
	if (_extract) {
		_extract = false;
		_rawVolumeRenderer.extract();
	}
	glClearColor(core::Color::Clear.r, core::Color::Clear.g, core::Color::Clear.b, core::Color::Clear.a);
	_frameBuffer.bind(false);
	render();
	_frameBuffer.unbind();
}

namespace tb {
TB_WIDGET_FACTORY(EditorScene, TBValue::TYPE_NULL, WIDGET_Z_TOP) {}
}
