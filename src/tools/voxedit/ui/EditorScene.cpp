#include "EditorScene.h"
#include "frontend/Movement.h"
#include "core/Common.h"
#include "core/Var.h"
#include "video/GLDebug.h"
#include "video/ScopedViewPort.h"
#include "core/Color.h"
#include "video/ScopedPolygonMode.h"
#include "video/ScopedScissor.h"
#include "video/ScopedFrameBuffer.h"
#include "voxel/VoxFormat.h"
#include "voxel/polyvox/VolumeMerger.h"
#include "ui/UIApp.h"
#define VOXELIZER_IMPLEMENTATION
#include "../voxelizer.h"

EditorScene::EditorScene() :
		ui::Widget(), _rawVolumeRenderer(true), _cursorVolume(nullptr), _modelVolume(nullptr),
		_bitmap((tb::UIRendererGL*)tb::g_renderer) {
	registerMoveCmd("+move_right", MOVERIGHT);
	registerMoveCmd("+move_left", MOVELEFT);
	registerMoveCmd("+move_forward", MOVEFORWARD);
	registerMoveCmd("+move_backward", MOVEBACKWARD);
	_currentVoxel = voxel::createVoxel(voxel::VoxelType::Grass1);
	SetIsFocusable(true);
}

EditorScene::~EditorScene() {
	core::Command::unregisterCommand("+move_right");
	core::Command::unregisterCommand("+move_left");
	core::Command::unregisterCommand("+move_upt");
	core::Command::unregisterCommand("+move_down");

	_axis.shutdown();
	_frameBuffer.shutdown();
	delete _cursorVolume;
	delete _modelVolume;
	delete _rawVolumeRenderer.shutdown();
}

const voxel::Voxel& EditorScene::getVoxel(const glm::ivec3& pos) const {
	return _modelVolume->getVoxel(pos);
}

bool EditorScene::setVoxel(const glm::ivec3& pos, const voxel::Voxel& voxel) {
	return _modelVolume->setVoxel(pos, voxel);
}

void EditorScene::newVolume() {
	const voxel::Region region(glm::ivec3(0), glm::ivec3(_size));
	setNewVolume(new voxel::RawVolume(region));
}

void EditorScene::setNewVolume(voxel::RawVolume *volume) {
	delete _modelVolume;
	_modelVolume = volume;

	const voxel::Region& region = volume->getEnclosingRegion();
	delete _cursorVolume;
	_cursorVolume = new voxel::RawVolume(region);

	delete _rawVolumeRenderer.setVolume(new voxel::RawVolume(region));

	_extract = true;
	_dirty = false;
	_lastRaytraceX = _lastRaytraceY = -1;
}

void EditorScene::render() {
	core_trace_scoped(EditorSceneRender);
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

	core_trace_scoped(EditorSceneExecuteAction);
	const long now = core::App::getInstance()->currentMillis();
	if (_lastAction == _action) {
		if (now - _lastActionExecution < _actionExecutionDelay) {
			return;
		}
	}
	_lastAction = _action;
	_lastActionExecution = now;

	bool extract = false;
	if (_result.didHit && _action == Action::CopyVoxel) {
		_currentVoxel = getVoxel(_result.hitVoxel);
	} else if (_result.didHit && _action == Action::OverrideVoxel) {
		extract = setVoxel(_result.hitVoxel, _currentVoxel);
	} else if (_result.didHit && _action == Action::DeleteVoxel) {
		extract = setVoxel(_result.hitVoxel, voxel::createVoxel(voxel::VoxelType::Air));
	} else if (_result.validPreviousVoxel && _action == Action::PlaceVoxel) {
		extract = setVoxel(_result.previousVoxel, _currentVoxel);
	} else if (_result.didHit && _action == Action::PlaceVoxel) {
		extract = setVoxel(_result.hitVoxel, _currentVoxel);
	}

	if (extract) {
		_lastRaytraceX = _lastRaytraceY = -1;
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
	core_trace_scoped(EditorSceneNewModel);
	if (_dirty && !force) {
		return false;
	}
	_dirty = false;
	newVolume();
	_result = voxel::PickResult();
	_extract = true;
	_lastRaytraceX = _lastRaytraceY = -1;
	return true;
}

bool EditorScene::saveModel(std::string_view file) {
	core_trace_scoped(EditorSceneSaveModel);
	if (!_dirty) {
		// nothing to save yet
		return true;
	}
	if (_modelVolume == nullptr) {
		return false;
	}
	const io::FilePtr& filePtr = core::App::getInstance()->filesystem()->open(std::string(file));
	voxel::VoxFormat f;
	if (f.save(_modelVolume, filePtr)) {
		_dirty = false;
	}
	return !_dirty;
}

bool EditorScene::voxelizeModel(const video::MeshPtr& meshPtr) {
	const video::Mesh::Vertices& positions = meshPtr->vertices();
	const video::Mesh::Indices& indices = meshPtr->indices();
	vx_mesh_t* mesh = vx_mesh_alloc(positions.size(), indices.size());

	for (size_t f = 0; f < indices.size(); f++) {
		mesh->indices[f] = indices[f];
	}

	for (size_t v = 0; v < positions.size() / 3; v++) {
		const core::Vertex& vertex = positions[v];
		mesh->vertices[v].x = vertex._pos.x;
		mesh->vertices[v].y = vertex._pos.y;
		mesh->vertices[v].z = vertex._pos.z;
	}

	const glm::vec3& maxs = meshPtr->maxs();
	const float size = _size;
	const glm::vec3& scale = maxs / size;
	const float precision = scale.x / 10.0f;

	vx_mesh_t* result = vx_voxelize(mesh, scale.x, scale.y, scale.z, precision);

	Log::info("Number of vertices: %i", (int)result->nvertices);
	Log::info("Number of indices: %i", (int)result->nindices);

	vx_mesh_free(result);
	vx_mesh_free(mesh);
	return false;
}

bool EditorScene::loadModel(std::string_view file) {
	core_trace_scoped(EditorSceneLoadModel);
	const io::FilePtr& filePtr = core::App::getInstance()->filesystem()->open(std::string(file));
	if (!(bool)filePtr) {
		return false;
	}
	voxel::VoxFormat f;
	voxel::RawVolume* newVolume = f.load(filePtr);
	if (newVolume == nullptr) {
		return false;
	}
	setNewVolume(newVolume);
	return true;
}

void EditorScene::resetCamera() {
	_camera.setAngles(0.0f, 0.0f, 0.0f);
	_camera.setPosition(glm::vec3(50.0f, 50.0f, 100.0f));
	_camera.lookAt(glm::vec3(0.0001f));
}

bool EditorScene::OnEvent(const tb::TBWidgetEvent &ev) {
	core_trace_scoped(EditorSceneOnEvent);
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
		const bool current = isRelativeMouseMode();
		if (current) {
			const float yaw = x - _mouseX;
			const float pitch = y - _mouseY;
			const float s = _rotationSpeed->floatVal();
			_camera.turn(yaw * s);
			_camera.pitch(pitch * s);
			_mouseX = x;
			_mouseY = y;
			return true;
		}
		const int deltaX = std::abs(x - _mouseX);
		const int deltaY = std::abs(y - _mouseY);
		const int minMove = 2;
		// prevent micro movement from executing the action over and over again
		if (deltaX <= minMove && deltaY <= minMove) {
			return Super::OnEvent(ev);
		}
		_mouseX = x;
		_mouseY = y;
		executeAction(tx, ty);
		return true;
	}
	return Super::OnEvent(ev);
}

void EditorScene::OnFocusChanged(bool focused) {
	Super::OnFocusChanged(focused);
}

void EditorScene::OnPaint(const PaintProps &paintProps) {
	core_trace_scoped(EditorSceneOnPaint);
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
}

void EditorScene::OnProcess() {
	Super::OnProcess();
	core_trace_scoped(EditorSceneOnProcess);
	const long deltaFrame = core::App::getInstance()->deltaFrame();
	const float speed = _cameraSpeed * static_cast<float>(deltaFrame);
	const glm::vec3& moveDelta = getMoveDelta(speed, _moveMask);
	_camera.move(moveDelta);
	_camera.update(deltaFrame);

	if (_lastRaytraceX != _mouseX || _lastRaytraceY != _mouseY) {
		core_trace_scoped(EditorSceneOnProcessUpdateRay);
		_lastRaytraceX = _mouseX;
		_lastRaytraceY = _mouseY;

		ui::UIRect rect = GetRect();
		ConvertToRoot(rect.x, rect.y);
		const int tx = _mouseX + rect.x;
		const int ty = _mouseY + rect.y;
		const video::Ray& ray = _camera.mouseRay(glm::ivec2(tx, ty));
		const glm::vec3& dirWithLength = ray.direction * _camera.farPlane();
		const voxel::Voxel& air = voxel::createVoxel(voxel::VoxelType::Air);
		_result = voxel::pickVoxel(_modelVolume, ray.origin, dirWithLength, air);

		if (_result.validPreviousVoxel) {
			_cursorVolume->clear();
			_cursorVolume->setVoxel(_result.previousVoxel, _currentVoxel);
		} else if (_result.didHit) {
			_cursorVolume->clear();
			_cursorVolume->setVoxel(_result.hitVoxel, _currentVoxel);
		}

		core_trace_scoped(EditorSceneOnProcessMergeRawVolumes);
		voxel::RawVolume* volume = _rawVolumeRenderer.volume();
		volume->clear();
		const bool current = isRelativeMouseMode();
		if (!current) {
			voxel::mergeRawVolumes(volume, _cursorVolume, air);
		}
		voxel::mergeRawVolumes(volume, _modelVolume, air);
		_extract = true;
	}

	if (_extract) {
		_extract = false;
		_rawVolumeRenderer.extract();
	}

	glClearColor(core::Color::Clear.r, core::Color::Clear.g, core::Color::Clear.b, core::Color::Clear.a);
	core_trace_scoped(EditorSceneRenderFramebuffer);
	_frameBuffer.bind(false);
	render();
	_frameBuffer.unbind();
}

namespace tb {
TB_WIDGET_FACTORY(EditorScene, TBValue::TYPE_NULL, WIDGET_Z_TOP) {}
}
