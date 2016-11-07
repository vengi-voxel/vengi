#include "EditorScene.h"
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
#include "EditorModel.h"

static const struct Selection {
	SelectType type;
	selections::Select& select;
} selectionsArray[] = {
	{SelectType::Single, selections::Single::get()},
	{SelectType::Same, selections::Single::get()},
	{SelectType::LineVertical, selections::Single::get()},
	{SelectType::LineHorizontal, selections::Single::get()},
	{SelectType::Edge, selections::Single::get()}
};
static_assert(SDL_arraysize(selectionsArray) == std::enum_value(SelectType::Max), "Array size doesn't match selection modes");

static inline EditorModel& m() {
	static EditorModel editorModel;
	return editorModel;
}

#define VOXELIZER_IMPLEMENTATION
#include "../voxelizer.h"

EditorScene::EditorScene() :
		Super(),
		_bitmap((tb::UIRendererGL*) tb::g_renderer) {
	//_rawVolumeRenderer.setAmbientColor(core::Color::White.xyz());
	SetIsFocusable(true);
	SetAutoFocusState(true);
	m().init();
}

EditorScene::~EditorScene() {
	_axis.shutdown();
	_frameBuffer.shutdown();
	m().shutdown();
}

const voxel::Voxel& EditorScene::getVoxel(const glm::ivec3& pos) const {
	return m()._modelVolume->getVoxel(pos);
}

bool EditorScene::setVoxel(const glm::ivec3& pos, const voxel::Voxel& voxel) {
	return m()._modelVolume->setVoxel(pos, voxel);
}

void EditorScene::newVolume() {
	const voxel::Region region(glm::ivec3(0), glm::ivec3(m()._size));
	setNewVolume(new voxel::RawVolume(region));
}

void EditorScene::setupReference(EditorScene* ref) {
	ref->_parent = this;
	ref->resetCamera();
}

void EditorScene::addReference(EditorScene* ref) {
	_references.push_back(ref);
	setupReference(ref);
}

void EditorScene::setNewVolume(voxel::RawVolume *volume) {
	delete m()._modelVolume;
	m()._modelVolume = volume;

	const voxel::Region& region = volume->getEnclosingRegion();
	delete m()._cursorPositionVolume;
	m()._cursorPositionVolume = new voxel::RawVolume(region);

	delete m()._rawVolumeSelectionRenderer.setVolume(new voxel::RawVolume(region));
	delete m()._rawVolumeRenderer.setVolume(new voxel::RawVolume(region));

	m()._empty = true;
	m()._extract = true;
	m()._dirty = false;
	m()._lastRaytraceX = m()._lastRaytraceY = -1;
	resetCamera();
}

void EditorScene::render() {
	core_trace_scoped(EditorSceneRender);
	{
		video::ScopedPolygonMode polygonMode(_camera.polygonMode());
		m()._rawVolumeRenderer.render(_camera);
	}
	{
		video::ScopedPolygonMode polygonMode(video::PolygonMode::WireFrame);
		m()._rawVolumeSelectionRenderer.render(_camera);
	}
	if (m()._renderAxis) {
		_axis.render(_camera);
	}
}

void EditorScene::select(const glm::ivec3& pos) {
	voxel::RawVolume* selectionVolume = m()._rawVolumeSelectionRenderer.volume();
	const Selection& mode = selectionsArray[std::enum_value(m()._selectionType)];
	if (mode.select.execute(m()._modelVolume, selectionVolume, pos)) {
		m()._selectionExtract = true;
	}
}

void EditorScene::executeAction(int32_t x, int32_t y) {
	if (m()._action == Action::None || !_mouseDown) {
		return;
	}

	core_trace_scoped(EditorSceneExecuteAction);
	const long now = core::App::getInstance()->currentMillis();
	if (m()._lastAction == m()._action) {
		if (now - m()._lastActionExecution < m()._actionExecutionDelay) {
			return;
		}
	}
	m()._lastAction = m()._action;
	m()._lastActionExecution = now;

	bool extract = false;
	if (m()._result.didHit && m()._action == Action::CopyVoxel) {
		m()._currentVoxel = getVoxel(m()._result.hitVoxel);
	} else if (m()._result.didHit && m()._action == Action::SelectVoxels) {
		select(m()._result.hitVoxel);
	} else if (m()._result.didHit && m()._action == Action::OverrideVoxel) {
		extract = setVoxel(m()._result.hitVoxel, m()._currentVoxel);
	} else if (m()._result.didHit && m()._action == Action::DeleteVoxel) {
		extract = setVoxel(m()._result.hitVoxel, voxel::createVoxel(voxel::VoxelType::Air));
	} else if (m()._result.validPreviousVoxel && m()._action == Action::PlaceVoxel) {
		extract = setVoxel(m()._result.previousVoxel, m()._currentVoxel);
	} else if (m()._result.didHit && m()._action == Action::PlaceVoxel) {
		extract = setVoxel(m()._result.hitVoxel, m()._currentVoxel);
	}

	if (extract) {
		resetLastTrace();
	}

	if (extract) {
		m()._extract = true;
		m()._extract = true;
	}
}

void EditorScene::resetLastTrace() {
	m()._lastRaytraceX = m()._lastRaytraceY = -1;
}

Action EditorScene::action() const {
	return m()._uiAction;
}

void EditorScene::setKeyAction(Action action) {
	if (action == m()._keyAction) {
		return;
	}
	m()._keyAction = action;
}

void EditorScene::setInternalAction(Action action) {
	if (action == m()._action) {
		return;
	}
	m()._action = action;
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
}

void EditorScene::setAction(Action action) {
	m()._uiAction = action;
}

void EditorScene::setSelectionType(SelectType type) {
	m()._selectionType = type;
}

SelectType EditorScene::selectionType() const {
	return m()._selectionType;
}

bool EditorScene::newModel(bool force) {
	core_trace_scoped(EditorSceneNewModel);
	if (m()._dirty && !force) {
		return false;
	}
	m()._dirty = false;
	newVolume();
	m()._result = voxel::PickResult();
	m()._extract = true;
	resetLastTrace();
	return true;
}

bool EditorScene::saveModel(std::string_view file) {
	core_trace_scoped(EditorSceneSaveModel);
	if (!m()._dirty) {
		// nothing to save yet
		return true;
	}
	if (m()._modelVolume == nullptr) {
		return false;
	}
	const io::FilePtr& filePtr = core::App::getInstance()->filesystem()->open(std::string(file));
	voxel::VoxFormat f;
	if (f.save(m()._modelVolume, filePtr)) {
		m()._dirty = false;
	}
	return !m()._dirty;
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
	const float size = m()._size;
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
	return m()._empty;
}

bool EditorScene::exportModel(std::string_view file) {
	core_trace_scoped(EditorSceneExportModel);
	const io::FilePtr& filePtr = core::App::getInstance()->filesystem()->open(std::string(file));
	if (!(bool)filePtr) {
		return false;
	}

	return voxel::exportMesh(m()._rawVolumeRenderer.mesh(), filePtr->getName().c_str());
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
	for (EditorScene* ref : _references) {
		ref->resetCamera();
	}
	_camera.setAngles(0.0f, 0.0f, 0.0f);
	if (m()._modelVolume == nullptr) {
		return;
	}
	const voxel::Region& region = m()._modelVolume->getEnclosingRegion();
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
	m()._currentVoxel = voxel::createVoxel(type);
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
		Log::info("pointer down");
		if (m()._keyAction != Action::None) {
			setInternalAction(m()._keyAction);
		} else {
			setInternalAction(m()._uiAction);
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
				setKeyAction(Action::CopyVoxel);
			} else if (ev.modifierkeys & tb::TB_SHIFT) {
				setKeyAction(Action::OverrideVoxel);
			} else if (ev.modifierkeys & tb::TB_CTRL) {
				setKeyAction(Action::DeleteVoxel);
			}
			if (_mouseDown) {
				setInternalAction(m()._keyAction);
			}
			return true;
		}
	} else if (ev.type == tb::EVENT_TYPE_KEY_UP) {
		if (ev.modifierkeys && m()._keyAction != Action::None) {
			m()._keyAction = Action::None;
			if (_mouseDown) {
				setInternalAction(m()._uiAction);
			}
			return true;
		}
	} else if (ev.type == tb::EVENT_TYPE_WHEEL && ev.delta_y != 0) {
		const glm::vec3& moveDelta = glm::backward * m()._cameraSpeed * (float)(ev.delta_y * 100);
		_camera.move(moveDelta);
		return true;
	} else if (ev.type == tb::EVENT_TYPE_POINTER_MOVE) {
		const bool relative = isRelativeMouseMode();
		const bool middle = isMiddleMouseButtonPressed();
		const bool alt = m()._action == Action::None && (ev.modifierkeys & tb::TB_ALT);
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
	m()._rawVolumeRenderer.onResize(pos, dim);
	m()._rawVolumeSelectionRenderer.onResize(pos, dim);
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

	m()._rawVolumeRenderer.init();
	m()._rawVolumeSelectionRenderer.init();

	_rotationSpeed = core::Var::get(cfg::ClientMouseRotationSpeed, "0.01");

	resetCamera();
}

void EditorScene::OnProcess() {
	Super::OnProcess();
	core_trace_scoped(EditorSceneOnProcess);
	const long deltaFrame = core::App::getInstance()->deltaFrame();
	const float speed = m()._cameraSpeed * static_cast<float>(deltaFrame);
	const glm::vec3& moveDelta = getMoveDelta(speed, m()._moveMask);
	_camera.move(moveDelta);
	_camera.update(deltaFrame);
	if (m()._modelVolume == nullptr) {
		return;
	}
	m()._angle += deltaFrame * 0.001f;
	const glm::vec3 direction(glm::sin(m()._angle), 0.5f, glm::cos(m()._angle));
	m()._rawVolumeRenderer.setSunDirection(direction);
	if (m()._lastRaytraceX != _mouseX || m()._lastRaytraceY != _mouseY) {
		core_trace_scoped(EditorSceneOnProcessUpdateRay);
		m()._lastRaytraceX = _mouseX;
		m()._lastRaytraceY = _mouseY;

		const int tx = _mouseX;
		const int ty = _mouseY;
		const video::Ray& ray = _camera.mouseRay(glm::ivec2(tx, ty));
		const glm::vec3& dirWithLength = ray.direction * _camera.farPlane();
		const voxel::Voxel& air = voxel::createVoxel(voxel::VoxelType::Air);
		m()._result = voxel::pickVoxel(m()._modelVolume, ray.origin, dirWithLength, air);

		if (m()._result.validPreviousVoxel && (!m()._result.didHit || !actionRequiresExistingVoxel(m()._action))) {
			m()._cursorPositionVolume->clear();
			const glm::ivec3& center = m()._cursorVolume->getEnclosingRegion().getCentre();
			const glm::ivec3& cursorPos = m()._result.previousVoxel - center;
			voxel::mergeRawVolumes(m()._cursorPositionVolume, m()._cursorVolume, cursorPos);
		} else if (m()._result.didHit) {
			m()._cursorPositionVolume->clear();
			const glm::ivec3& center = m()._cursorVolume->getEnclosingRegion().getCentre();
			const glm::ivec3& cursorPos = m()._result.previousVoxel - center;
			voxel::mergeRawVolumes(m()._cursorPositionVolume, m()._cursorVolume, cursorPos);
			m()._cursorPositionVolume->setVoxel(m()._result.hitVoxel, m()._currentVoxel);
		}

		core_trace_scoped(EditorSceneOnProcessMergeRawVolumes);
		voxel::RawVolume* volume = m()._rawVolumeRenderer.volume();
		volume->clear();
		const bool current = isRelativeMouseMode();
		if (!current) {
			voxel::mergeRawVolumesSameDimension(volume, m()._cursorPositionVolume);
		}
		m()._empty = voxel::mergeRawVolumesSameDimension(volume, m()._modelVolume) == 0;
		m()._extract = true;
	}

	if (m()._extract) {
		m()._extract = false;
		m()._rawVolumeRenderer.extract();
	}
	if (m()._selectionExtract) {
		m()._selectionExtract = false;
		m()._rawVolumeSelectionRenderer.extract();
	}

	glClearColor(core::Color::Clear.r, core::Color::Clear.g, core::Color::Clear.b, core::Color::Clear.a);
	core_trace_scoped(EditorSceneRenderFramebuffer);
	_frameBuffer.bind(false);
	render();
	_frameBuffer.unbind();
}

bool EditorScene::renderAABB() const {
	return m()._rawVolumeRenderer.renderAABB();
}

void EditorScene::setRenderAABB(bool renderAABB) {
	m()._rawVolumeRenderer.setRenderAABB(renderAABB);
	for (EditorScene* ref : _references) {
		ref->setRenderAABB(renderAABB);
	}
}

bool EditorScene::renderGrid() const {
	return m()._rawVolumeRenderer.renderGrid();
}

void EditorScene::setRenderGrid(bool renderGrid) {
	m()._rawVolumeRenderer.setRenderGrid(renderGrid);
	for (EditorScene* ref : _references) {
		ref->setRenderGrid(renderGrid);
	}
}

inline long EditorScene::actionExecutionDelay() const {
	return m()._actionExecutionDelay;
}

void EditorScene::setActionExecutionDelay(long actionExecutionDelay) {
	m()._actionExecutionDelay = actionExecutionDelay;
	for (EditorScene* ref : _references) {
		ref->setActionExecutionDelay(actionExecutionDelay);
	}
}

bool EditorScene::renderAxis() const {
	return m()._renderAxis;
}

void EditorScene::setRenderAxis(bool renderAxis) {
	m()._renderAxis = renderAxis;
}

float EditorScene::cameraSpeed() const {
	return m()._cameraSpeed;
}

void EditorScene::setCameraSpeed(float cameraSpeed) {
	m()._cameraSpeed = cameraSpeed;
}

bool EditorScene::isDirty() const {
	return m()._dirty;
}

namespace tb {
TB_WIDGET_FACTORY(EditorScene, TBValue::TYPE_NULL, WIDGET_Z_TOP) {}
}
