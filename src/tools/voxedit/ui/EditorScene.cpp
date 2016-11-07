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
#include "voxel/model/VoxFormat.h"
#include "voxel/model/QB2Format.h"
#include "voxel/model/MeshExporter.h"
#include "voxel/polyvox/VolumeMerger.h"
#include "ui/UIApp.h"
#include "select/Single.h"

static const struct Selection {
	EditorScene::SelectType type;
	selections::Select& select;
} selectionsArray[] = {
	{EditorScene::SelectType::Single, selections::Single::get()},
	{EditorScene::SelectType::Same, selections::Single::get()},
	{EditorScene::SelectType::LineVertical, selections::Single::get()},
	{EditorScene::SelectType::LineHorizontal, selections::Single::get()},
	{EditorScene::SelectType::Edge, selections::Single::get()}
};
static_assert(SDL_arraysize(selectionsArray) == std::enum_value(EditorScene::SelectType::Max), "Array size doesn't match selection modes");

#define VOXELIZER_IMPLEMENTATION
#include "../voxelizer.h"

EditorScene::EditorScene() :
		Super(), _rawVolumeRenderer(true, false, true), _rawVolumeSelectionRenderer(false, false, false),
		_cursorVolume(nullptr), _cursorPositionVolume(nullptr), _modelVolume(nullptr),
		_bitmap((tb::UIRendererGL*) tb::g_renderer) {
	//_rawVolumeRenderer.setAmbientColor(core::Color::White.xyz());
	SetIsFocusable(true);
	_cursorVolume = new voxel::RawVolume(voxel::Region(0, 1));
	_cursorVolume->setVoxel(0, 0, 0, createVoxel(voxel::VoxelType::Grass1));
}

EditorScene::~EditorScene() {
	_axis.shutdown();
	_frameBuffer.shutdown();
	if (!_reference) {
		delete _cursorPositionVolume;
		delete _cursorVolume;
		delete _modelVolume;
		delete _rawVolumeRenderer.shutdown();
		delete _rawVolumeSelectionRenderer.shutdown();
	} else {
		_rawVolumeRenderer.shutdown();
		_rawVolumeSelectionRenderer.shutdown();
	}
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

void EditorScene::setupReference(EditorScene* ref) {
	ref->_rawVolumeRenderer.setVolume(_rawVolumeRenderer.volume());
	ref->_rawVolumeSelectionRenderer.setVolume(_rawVolumeSelectionRenderer.volume());
	ref->_modelVolume = _modelVolume;
	ref->_cursorPositionVolume = _cursorPositionVolume;
	ref->_cursorVolume = _cursorVolume;
	ref->_reference = true;

	ref->resetCamera();
}

void EditorScene::addReference(EditorScene* ref) {
	_references.push_back(ref);
	delete ref->_cursorVolume;
	ref->_cursorVolume = nullptr;
	setupReference(ref);
}

void EditorScene::setNewVolume(voxel::RawVolume *volume) {
	delete _modelVolume;
	_modelVolume = volume;

	const voxel::Region& region = volume->getEnclosingRegion();
	delete _cursorPositionVolume;
	_cursorPositionVolume = new voxel::RawVolume(region);

	delete _rawVolumeSelectionRenderer.setVolume(new voxel::RawVolume(region));
	delete _rawVolumeRenderer.setVolume(new voxel::RawVolume(region));

	for (EditorScene* ref : _references) {
		setupReference(ref);
	}

	_empty = true;
	_extract = true;
	_dirty = false;
	_lastRaytraceX = _lastRaytraceY = -1;
	resetCamera();
}

void EditorScene::render() {
	core_trace_scoped(EditorSceneRender);
	{
		video::ScopedPolygonMode polygonMode(_camera.polygonMode());
		_rawVolumeRenderer.render(_camera);
	}
	{
		video::ScopedPolygonMode polygonMode(video::PolygonMode::WireFrame);
		_rawVolumeSelectionRenderer.render(_camera);
	}
	if (_renderAxis) {
		_axis.render(_camera);
	}
}

void EditorScene::select(const glm::ivec3& pos) {
	voxel::RawVolume* selectionVolume = _rawVolumeSelectionRenderer.volume();
	const Selection& mode = selectionsArray[std::enum_value(_selectionType)];
	if (mode.select.execute(_modelVolume, selectionVolume, pos)) {
		_selectionExtract = true;
	}
}

void EditorScene::executeAction(int32_t x, int32_t y) {
	if (_action == Action::None || !_mouseDown) {
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
	} else if (_result.didHit && _action == Action::SelectVoxels) {
		select(_result.hitVoxel);
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

void EditorScene::setInternalAction(EditorScene::Action action) {
	if (action == _action) {
		return;
	}
	switch (action) {
	case Action::None:
		Log::debug("Action: None");
		break;
	case Action::PlaceVoxel:
		Log::debug("Action: PlaceVoxel");
		break;
	case Action::CopyVoxel:
		Log::debug("Action: CopyVoxel");
		break;
	case Action::DeleteVoxel:
		Log::debug("Action: DeleteVoxel");
		break;
	case Action::OverrideVoxel:
		Log::debug("Action: OverrideVoxel");
		break;
	case Action::SelectVoxels:
		Log::debug("Action: SelectVoxel");
		break;
	}
	_action = action;
}

void EditorScene::setAction(EditorScene::Action action) {
	_uiAction = action;
}

void EditorScene::setSelectionType(SelectType type) {
	_selectionType = type;
}

EditorScene::SelectType EditorScene::selectionType() const {
	return _selectionType;
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

bool EditorScene::isEmpty() const {
	return _empty;
}

bool EditorScene::exportModel(std::string_view file) {
	core_trace_scoped(EditorSceneExportModel);
	const io::FilePtr& filePtr = core::App::getInstance()->filesystem()->open(std::string(file));
	if (!(bool)filePtr) {
		return false;
	}

	return voxel::exportMesh(_rawVolumeRenderer.mesh(), filePtr->getName().c_str());
}

bool EditorScene::loadModel(std::string_view file) {
	core_trace_scoped(EditorSceneLoadModel);
	const io::FilePtr& filePtr = core::App::getInstance()->filesystem()->open(std::string(file));
	if (!(bool)filePtr) {
		Log::error("Failed to open model file %s", file.data());
		return false;
	}
	voxel::VoxFormat f;
	voxel::RawVolume* newVolume = f.load(filePtr);
	if (newVolume == nullptr) {
		Log::error("Failed to load model file %s", file.data());
		return false;
	}
	Log::info("Loaded model file %s", file.data());
	setNewVolume(newVolume);
	return true;
}

void EditorScene::resetCamera() {
	_camera.setAngles(0.0f, 0.0f, 0.0f);
	if (_modelVolume == nullptr) {
		return;
	}
	const voxel::Region& region = _modelVolume->getEnclosingRegion();
	const glm::ivec3& center = region.getCentre();
	if (_camMode == SceneCameraMode::Free) {
		_camera.setPosition(glm::vec3(-center));
		_camera.lookAt(glm::vec3(0.0001f));
	} else if (_camMode == SceneCameraMode::Top) {
		_camera.setPosition(glm::vec3(center.x, region.getHeightInCells() + center.y, center.z));
		_camera.lookAt(glm::down);
	} else if (_camMode == SceneCameraMode::Left) {
		_camera.setPosition(glm::vec3(region.getWidthInCells() + center.x, center.y, center.z));
		_camera.lookAt(glm::right);
	} else if (_camMode == SceneCameraMode::Front) {
		_camera.setPosition(glm::vec3(center.x, center.y, region.getDepthInCells() + center.z));
		_camera.lookAt(glm::backward);
	}
}

void EditorScene::setVoxelType(voxel::VoxelType type) {
	Log::info("Change voxel to %i", std::enum_value(type));
	_currentVoxel = voxel::createVoxel(type);
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
		_mouseDown = true;
		if (_keyAction != Action::None) {
			setInternalAction(_keyAction);
		} else {
			setInternalAction(_uiAction);
		}
		executeAction(tx, ty);
		return true;
	} else if (ev.type == tb::EVENT_TYPE_POINTER_UP) {
		_mouseDown = false;
		setInternalAction(Action::None);
		return true;
	} else if (ev.type == tb::EVENT_TYPE_KEY_DOWN) {
		if (ev.modifierkeys) {
			if (ev.modifierkeys & tb::TB_ALT) {
				_keyAction = Action::CopyVoxel;
			} else if (ev.modifierkeys & tb::TB_SHIFT) {
				_keyAction = Action::OverrideVoxel;
			} else if (ev.modifierkeys & tb::TB_CTRL) {
				_keyAction = Action::DeleteVoxel;
			}
			if (_mouseDown) {
				setInternalAction(_keyAction);
			}
			return true;
		}
	} else if (ev.type == tb::EVENT_TYPE_KEY_UP) {
		if (ev.modifierkeys && _keyAction != Action::None) {
			_keyAction = Action::None;
			if (_mouseDown) {
				setInternalAction(_uiAction);
			}
			return true;
		}
	} else if (ev.type == tb::EVENT_TYPE_WHEEL && ev.delta_y != 0) {
		const glm::vec3& moveDelta = glm::backward * _cameraSpeed * (float)(ev.delta_y * 100);
		_camera.move(moveDelta);
		return true;
	} else if (ev.type == tb::EVENT_TYPE_POINTER_MOVE) {
		const bool relative = isRelativeMouseMode();
		const bool middle = isMiddleMouseButtonPressed();
		const bool alt = _action == Action::None && (ev.modifierkeys & tb::TB_ALT);
		if (relative || middle || alt) {
			const float yaw = x - _mouseX;
			const float pitch = y - _mouseY;
			const float s = _rotationSpeed->floatVal();
			if (_camMode == SceneCameraMode::Free) {
				_camera.turn(yaw * s);
				_camera.pitch(pitch * s);
			}
			_mouseX = x;
			_mouseY = y;
			return true;
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
	if (focused) {
		registerMoveCmd("+move_right", MOVERIGHT);
		registerMoveCmd("+move_left", MOVELEFT);
		registerMoveCmd("+move_forward", MOVEFORWARD);
		registerMoveCmd("+move_backward", MOVEBACKWARD);
	} else {
		core::Command::unregisterCommand("+move_right");
		core::Command::unregisterCommand("+move_left");
		core::Command::unregisterCommand("+move_upt");
		core::Command::unregisterCommand("+move_down");
	}
}

void EditorScene::OnResized(int oldw, int oldh) {
	core_trace_scoped(EditorSceneOnResized);
	Super::OnResized(oldw, oldh);
	const tb::TBRect& rect = GetRect();
	const glm::ivec2 pos(0, 0);
	const glm::ivec2 dim(rect.w, rect.h);
	_camera.init(pos, dim);
	_frameBuffer.shutdown();
	_frameBuffer.init(dim);
	_bitmap.Init(dim.x, dim.y, _frameBuffer.texture());
	_rawVolumeRenderer.onResize(pos, dim);
	_rawVolumeSelectionRenderer.onResize(pos, dim);
}

void EditorScene::OnPaint(const PaintProps &paintProps) {
	core_trace_scoped(EditorSceneOnPaint);
	Super::OnPaint(paintProps);
	const glm::ivec2& dimension = _frameBuffer.dimension();
	ui::UIRect rect = GetRect();
	// the fbo is flipped in memory, we have to deal with it here
	const tb::TBRect srcRect(0, dimension.y, rect.w, -rect.h);
	tb::g_renderer->DrawBitmap(rect, srcRect, &_bitmap);
}

void EditorScene::OnInflate(const tb::INFLATE_INFO &info) {
	Super::OnInflate(info);
	_axis.init();

	const char *cameraMode = info.node->GetValueString("camera", "free");
	if (!strcmp(cameraMode, "top")) {
		_camMode = SceneCameraMode::Top;
		_camera.setMode(video::CameraMode::Orthogonal);
	} else if (!strcmp(cameraMode, "front")) {
		_camMode = SceneCameraMode::Front;
		_camera.setMode(video::CameraMode::Orthogonal);
	} else if (!strcmp(cameraMode, "left")) {
		_camMode = SceneCameraMode::Left;
		_camera.setMode(video::CameraMode::Orthogonal);
	} else {
		_camMode = SceneCameraMode::Free;
		_camera.setMode(video::CameraMode::Perspective);
	}

	_rawVolumeRenderer.init();
	_rawVolumeSelectionRenderer.init();

	_rotationSpeed = core::Var::get(cfg::ClientMouseRotationSpeed, "0.01");

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
	if (_modelVolume == nullptr) {
		return;
	}
	_angle += deltaFrame * 0.001f;
	const glm::vec3 direction(glm::sin(_angle), 0.5f, glm::cos(_angle));
	_rawVolumeRenderer.setSunDirection(direction);
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

		if (_result.validPreviousVoxel && (!_result.didHit || !actionRequiresExistingVoxel(_action))) {
			_cursorPositionVolume->clear();
			const glm::ivec3& center = _cursorVolume->getEnclosingRegion().getCentre();
			const glm::ivec3& cursorPos = _result.previousVoxel - center;
			voxel::mergeRawVolumes(_cursorPositionVolume, _cursorVolume, cursorPos);
		} else if (_result.didHit) {
			_cursorPositionVolume->clear();
			const glm::ivec3& center = _cursorVolume->getEnclosingRegion().getCentre();
			const glm::ivec3& cursorPos = _result.previousVoxel - center;
			voxel::mergeRawVolumes(_cursorPositionVolume, _cursorVolume, cursorPos);
			_cursorPositionVolume->setVoxel(_result.hitVoxel, _currentVoxel);
		}

		core_trace_scoped(EditorSceneOnProcessMergeRawVolumes);
		voxel::RawVolume* volume = _rawVolumeRenderer.volume();
		volume->clear();
		const bool current = isRelativeMouseMode();
		if (!current) {
			voxel::mergeRawVolumesSameDimension(volume, _cursorPositionVolume);
		}
		_empty = voxel::mergeRawVolumesSameDimension(volume, _modelVolume) == 0;
		_extract = true;
	}

	if (_extract) {
		_extract = false;
		_rawVolumeRenderer.extract();
	}
	if (_selectionExtract) {
		_selectionExtract = false;
		_rawVolumeSelectionRenderer.extract();
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
