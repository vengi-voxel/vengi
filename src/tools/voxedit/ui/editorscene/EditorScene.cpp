#include "EditorScene.h"
#include "core/Common.h"
#include "core/Var.h"
#include "video/GLDebug.h"
#include "video/ScopedViewPort.h"
#include "core/Color.h"
#include "video/ScopedPolygonMode.h"
#include "video/ScopedScissor.h"
#include "video/ScopedFrameBuffer.h"
#include "voxel/model/MeshExporter.h"
#include "ui/UIApp.h"
#include "Model.h"

static inline EditorModel& m() {
	static EditorModel editorModel;
	return editorModel;
}

#define VOXELIZER_IMPLEMENTATION
#include "voxelizer.h"

EditorScene::EditorScene() :
		Super(),
		_bitmap((tb::UIRendererGL*) tb::g_renderer) {
	//_rawVolumeRenderer.setAmbientColor(core::Color::White.xyz());
	SetIsFocusable(true);
}

EditorScene::~EditorScene() {
	_axis.shutdown();
	_frameBuffer.shutdown();
	EditorModel& mdl = m();
	mdl.shutdown();
}

void EditorScene::addReference(EditorScene* ref) {
	_references.push_back(ref);
	ref->_parent = this;
	ref->resetCamera();
}

void EditorScene::render() {
	core_trace_scoped(EditorSceneRender);
	EditorModel& mdl = m();
	const video::Camera& camera = _controller.camera();
	{
		video::ScopedPolygonMode polygonMode(camera.polygonMode());
		mdl.render(camera);
	}
	{
		video::ScopedPolygonMode polygonMode(video::PolygonMode::WireFrame);
		mdl.render(camera);
	}
	if (mdl.renderAxis()) {
		_axis.render(camera);
	}
}

void EditorScene::setKeyAction(Action action) {
	EditorModel& mdl = m();
	if (action == mdl.keyAction()) {
		return;
	}
	mdl._keyAction = action;
}

void EditorScene::setInternalAction(Action action) {
	EditorModel& mdl = m();
	if (action == mdl.action()) {
		return;
	}
	mdl.setAction(action);
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
	EditorModel& mdl = m();
	mdl._uiAction = action;
}

void EditorScene::setSelectionType(SelectType type) {
	EditorModel& mdl = m();
	mdl._selectionType = type;
}

SelectType EditorScene::selectionType() const {
	const EditorModel& mdl = m();
	return mdl._selectionType;
}

bool EditorScene::newModel(bool force) {
	core_trace_scoped(EditorSceneNewModel);
	if (!m().newVolume(force)) {
		return false;
	}
	resetCamera();
	return true;
}

bool EditorScene::saveModel(std::string_view file) {
	core_trace_scoped(EditorSceneSaveModel);
	return m().save(file);
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
	const EditorModel& mdl = m();
	const float size = mdl.size();
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
	const EditorModel& mdl = m();
	return mdl.empty();
}

bool EditorScene::exportModel(std::string_view file) {
	core_trace_scoped(EditorSceneExportModel);
	const io::FilePtr& filePtr = core::App::getInstance()->filesystem()->open(std::string(file));
	if (!(bool)filePtr) {
		return false;
	}

	const EditorModel& mdl = m();
	return voxel::exportMesh(mdl.rawVolumeRenderer().mesh(), filePtr->getName().c_str());
}

bool EditorScene::loadModel(std::string_view file) {
	core_trace_scoped(EditorSceneLoadModel);
	if (!m().load(file)) {
		return false;
	}
	resetCamera();
	return true;
}

void EditorScene::resetCamera() {
	for (EditorScene* ref : _references) {
		ref->resetCamera();
	}
	_controller.resetCamera(m().modelVolume());
}

void EditorScene::setVoxelType(voxel::VoxelType type) {
	EditorModel& mdl = m();
	mdl.setVoxelType(type);
}

bool EditorScene::OnEvent(const tb::TBWidgetEvent &ev) {
	core_trace_scoped(EditorSceneOnEvent);
	const int x = ev.target_x;
	const int y = ev.target_y;
	ui::UIRect rect = GetRect();
	ConvertToRoot(rect.x, rect.y);
	const int tx = x + rect.x;
	const int ty = y + rect.y;
	const long now = core::App::getInstance()->currentMillis();
	EditorModel& mdl = m();
	bool& mouseDown = _controller._mouseDown;
	if (ev.type == tb::EVENT_TYPE_POINTER_DOWN) {
		mouseDown = true;
		if (mdl.keyAction() != Action::None) {
			setInternalAction(mdl.keyAction());
		} else {
			setInternalAction(mdl.uiAction());
		}
		mdl.executeAction(tx, ty, mouseDown, now);
		return true;
	} else if (ev.type == tb::EVENT_TYPE_POINTER_UP) {
		mouseDown = false;
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
			if (mouseDown) {
				setInternalAction(mdl.keyAction());
			}
			return true;
		}
	} else if (ev.type == tb::EVENT_TYPE_KEY_UP) {
		if (ev.modifierkeys && mdl.keyAction() != Action::None) {
			setKeyAction(Action::None);
			if (mouseDown) {
				setInternalAction(mdl.uiAction());
			}
			return true;
		}
	} else if (ev.type == tb::EVENT_TYPE_WHEEL && ev.delta_y != 0) {
		_controller.zoom((float)(ev.delta_y * 100));
		return true;
	} else if (ev.type == tb::EVENT_TYPE_POINTER_MOVE) {
		const bool relative = isRelativeMouseMode();
		const bool middle = isMiddleMouseButtonPressed();
		const bool alt = mdl.action() == Action::None && (ev.modifierkeys & tb::TB_ALT);
		if (_controller.move(relative || middle || alt, x, y)) {
			mdl.executeAction(tx, ty, mouseDown, now);
		}
		return true;
	}
	return Super::OnEvent(ev);
}

void EditorScene::select(const glm::ivec3& pos) {
	m().select(pos);
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
	_controller.onResize(pos, dim);
	_frameBuffer.shutdown();
	_frameBuffer.init(dim);
	_bitmap.Init(dim.x, dim.y, _frameBuffer.texture());
	EditorModel& mdl = m();
	mdl.onResize(pos, dim);
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
	EditorModel& mdl = m();
	mdl.init();

	Controller::SceneCameraMode mode = Controller::SceneCameraMode::Free;
	const char *cameraMode = info.node->GetValueString("camera", "free");
	if (!strcmp(cameraMode, "top")) {
		mode = Controller::SceneCameraMode::Top;
	} else if (!strcmp(cameraMode, "front")) {
		mode = Controller::SceneCameraMode::Front;
	} else if (!strcmp(cameraMode, "left")) {
		mode = Controller::SceneCameraMode::Left;
	}
	_controller.init(mode);
}

void EditorScene::OnProcess() {
	Super::OnProcess();
	core_trace_scoped(EditorSceneOnProcess);

	const long deltaFrame = core::App::getInstance()->deltaFrame();
	_controller.update(deltaFrame);

	EditorModel& mdl = m();
	if (mdl.modelVolume() == nullptr) {
		return;
	}

	const bool skipCursor = isRelativeMouseMode();
	mdl.trace(_controller._mouseX, _controller._mouseY, skipCursor, _controller.camera());

	glClearColor(core::Color::Clear.r, core::Color::Clear.g, core::Color::Clear.b, core::Color::Clear.a);
	core_trace_scoped(EditorSceneRenderFramebuffer);
	_frameBuffer.bind(false);
	render();
	_frameBuffer.unbind();
}

bool EditorScene::renderAABB() const {
	const EditorModel& mdl = m();
	return mdl.rawVolumeRenderer().renderAABB();
}

void EditorScene::setRenderAABB(bool renderAABB) {
	EditorModel& mdl = m();
	mdl.rawVolumeRenderer().setRenderAABB(renderAABB);
}

bool EditorScene::renderGrid() const {
	const EditorModel& mdl = m();
	return mdl.rawVolumeRenderer().renderGrid();
}

void EditorScene::setRenderGrid(bool renderGrid) {
	EditorModel& mdl = m();
	mdl.rawVolumeRenderer().setRenderGrid(renderGrid);
}

inline long EditorScene::actionExecutionDelay() const {
	const EditorModel& mdl = m();
	return mdl._actionExecutionDelay;
}

void EditorScene::setActionExecutionDelay(long actionExecutionDelay) {
	EditorModel& mdl = m();
	mdl._actionExecutionDelay = actionExecutionDelay;
}

bool EditorScene::renderAxis() const {
	const EditorModel& mdl = m();
	return mdl._renderAxis;
}

void EditorScene::setRenderAxis(bool renderAxis) {
	EditorModel& mdl = m();
	mdl._renderAxis = renderAxis;
}

float EditorScene::cameraSpeed() const {
	return _controller._cameraSpeed;
}

void EditorScene::setCameraSpeed(float cameraSpeed) {
	_controller._cameraSpeed = cameraSpeed;
}

bool EditorScene::isDirty() const {
	const EditorModel& mdl = m();
	return mdl.dirty();
}

namespace tb {
TB_WIDGET_FACTORY(EditorScene, TBValue::TYPE_NULL, WIDGET_Z_TOP) {}
}
