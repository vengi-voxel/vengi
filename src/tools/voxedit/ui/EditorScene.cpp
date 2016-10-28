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
#include "voxel/VoxFormat.h"

EditorScene::EditorScene() :
		ui::Widget(), _rawVolumeRenderer(true) {
	_currentVoxel = voxel::createVoxel(voxel::Grass1);
	SetIsFocusable(true);
}

EditorScene::~EditorScene() {
	_axis.shutdown();
	voxel::RawVolume* old = _rawVolumeRenderer.shutdown();
	delete old;
}

void EditorScene::executeAction(int32_t x, int32_t y) {
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

bool EditorScene::OnEvent(const tb::TBWidgetEvent &ev) {
	// TODO: once we have widget here, use the widget (not _root) luke
	//const ui::UIRect& rect = _root.GetRect();
	//_root.ConvertToRoot(x, y);

	const int x = ev.target_x;
	const int y = ev.target_y;
	/*if (ev.type == tb::EVENT_TYPE_RIGHT_POINTER_DOWN) {
		_action = Action::DeleteVoxel;
		executeAction(x, y);
		return true;
	} else*/ if (ev.type == tb::EVENT_TYPE_POINTER_DOWN) {
		_action = Action::PlaceVoxel;
		executeAction(x, y);
		return true;
	} else if (ev.type == tb::EVENT_TYPE_POINTER_UP) {
		_action = Action::None;
		executeAction(x, y);
		return true;
	} else if (ev.type == tb::EVENT_TYPE_POINTER_MOVE) {
		const bool current = SDL_GetRelativeMouseMode();
		if (current) {
			// TODO: use a delta here
			_camera.rotate(glm::vec3(ev.target_x, ev.target_y, 0.0f) * _rotationSpeed->floatVal());
		}
		executeAction(x, y);
		return true;
	}
	return Super::OnEvent(ev);
}

void EditorScene::OnPaint(const PaintProps &paintProps) {
	const long deltaFrame = core::App::getInstance()->deltaFrame();
	const float speed = _cameraSpeed * static_cast<float>(deltaFrame);
	const glm::vec3& moveDelta = getMoveDelta(speed, _moveMask);
	_camera.move(moveDelta);
	_camera.update(deltaFrame);
	if (_extract) {
		_extract = false;
		_rawVolumeRenderer.extract();
	}

	GLint v[4];
	glGetIntegerv(GL_VIEWPORT, v);
	tb::TBRect r = GetRect();
	tb::TBRect padr = GetPaddingRect();
	glViewport(padr.x, v[3] - r.h + padr.y, padr.w, padr.h);
	{
		video::ScopedPolygonMode polygonMode(_camera.polygonMode());
		_rawVolumeRenderer.render(_camera);
	}
	if (_renderAxis) {
		_axis.render(_camera);
	}

	glViewport(v[0], v[1], v[2], v[3]);
	Super::OnPaint(paintProps);
}

void EditorScene::OnResized(int oldWidth, int oldHeight) {
	Super::OnResized(oldWidth, oldHeight);

	const ui::UIRect& rect = GetRect();
	_rawVolumeRenderer.onResize(glm::ivec2(rect.x, rect.y), glm::ivec2(rect.w, rect.h));
	_camera.init(glm::ivec2(rect.x, rect.y), glm::ivec2(rect.w, rect.h));
}

void EditorScene::OnInflate(const tb::INFLATE_INFO &info) {
	Super::OnInflate(info);
	_axis.init();

	_rawVolumeRenderer.init();
	_rotationSpeed = core::Var::get(cfg::ClientMouseRotationSpeed, "0.01");

	_camera.setPosition(glm::vec3(0.0f, 50.0f, 100.0f));
	_camera.lookAt(glm::vec3(0.0001f));

	registerMoveCmd("+move_right", MOVERIGHT);
	registerMoveCmd("+move_left", MOVELEFT);
	registerMoveCmd("+move_forward", MOVEFORWARD);
	registerMoveCmd("+move_backward", MOVEBACKWARD);
}

namespace tb {
TB_WIDGET_FACTORY(EditorScene, TBValue::TYPE_NULL, WIDGET_Z_TOP) {}
}
