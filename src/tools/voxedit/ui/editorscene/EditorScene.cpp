/**
 * @file
 */

#include "EditorScene.h"
#include "core/Common.h"
#include "core/Var.h"
#include "core/Color.h"
#include "voxelformat/MeshExporter.h"
#include "ui/turbobadger/UIApp.h"
#include "io/Filesystem.h"
#include "Model.h"

using namespace voxedit;

static inline voxedit::Model& m() {
	static voxedit::Model editorModel;
	return editorModel;
}

#define VOXELIZER_IMPLEMENTATION
#include "voxelizer.h"

EditorScene::EditorScene() :
		Super(),
		_bitmap((tb::UIRendererGL*) tb::g_renderer) {
	//_rawVolumeRenderer.setAmbientColor(glm::vec3(core::Color::White));
	SetIsFocusable(true);
	_axis.init();
	_shapeRenderer.init();
	m().init();
}

EditorScene::~EditorScene() {
	_axis.shutdown();
	_frameBuffer.shutdown();
	_shapeRenderer.shutdown();
	_shapeBuilder.shutdown();
	m().shutdown();
}

void EditorScene::render() {
	core_trace_scoped(EditorSceneRender);
	Model& mdl = m();
	const video::Camera& camera = _controller.camera();
	mdl.render(camera);
	if (mdl.renderAxis()) {
		_axis.render(camera);
	}
	_shapeRenderer.render(_referencePointMesh, camera);
}

void EditorScene::setKeyAction(Action action) {
	m()._keyAction = action;
}

void EditorScene::setInternalAction(Action action) {
	m().setAction(action);
}

void EditorScene::setAction(Action action) {
	m()._uiAction = action;
}

voxedit::Shape EditorScene::cursorShape() const {
	return m().shapeHandler().cursorShape();
}

void EditorScene::setCursorShape(voxedit::Shape type) {
	m().setCursorShape(type);
}

void EditorScene::scaleCursorShape(const glm::vec3& scale) {
	return m().scaleCursorShape(scale);
}

const glm::ivec3& EditorScene::cursorPosition() const {
	return m().cursorPosition();
}

void EditorScene::setCursorPosition(const glm::ivec3& pos, bool force) {
	m().setCursorPosition(pos, force);
}

const glm::ivec3& EditorScene::referencePosition() const {
	return m().referencePosition();
}

void EditorScene::setReferencePosition(const glm::ivec3& pos) {
	m().setReferencePosition(pos);
	_shapeBuilder.clear();
	_shapeBuilder.setColor(core::Color::alpha(core::Color::SteelBlue, 0.8f));
	_shapeBuilder.setPosition(pos);
	_shapeBuilder.sphere(5, 4, 1.0f);
	_shapeRenderer.createOrUpdate(_referencePointMesh, _shapeBuilder);
	camera().setTarget(pos);
}

math::Axis EditorScene::lockedAxis() const {
	return m().lockedAxis();
}

void EditorScene::setLockedAxis(math::Axis axis, bool unlock) {
	m().setLockedAxis(axis, unlock);
}

math::Axis EditorScene::mirrorAxis() const {
	return m().mirrorAxis();
}

void EditorScene::setMirrorAxis(math::Axis axis, const glm::ivec3& pos) {
	m().setMirrorAxis(axis, pos);
}

void EditorScene::setSelectionType(SelectType type) {
	m().setSelectionType(type);
}

SelectType EditorScene::selectionType() const {
	return m().selectionType();
}

void EditorScene::place() {
	m().place();
}

void EditorScene::remove() {
	m().remove();
}

void EditorScene::rotate(int angleX, int angleY, int angleZ) {
	m().rotate(angleX, angleY, angleZ);
}

bool EditorScene::resample(int factor) {
	return m().resample(factor);
}

void EditorScene::move(int x, int y, int z) {
	m().move(x, y, z);
}

bool EditorScene::newModel(bool force) {
	core_trace_scoped(EditorSceneNewModel);
	if (!m().newVolume(force)) {
		return false;
	}
	setReferencePosition(referencePosition());
	resetCamera();
	return true;
}

bool EditorScene::importHeightmap(const std::string& file) {
	core_trace_scoped(EditorSceneImportHeightmap);
	return m().importHeightmap(file);
}

bool EditorScene::saveModel(const std::string& file) {
	core_trace_scoped(EditorSceneSaveModel);
	return m().save(file);
}

void EditorScene::crop() {
	m().crop();
}

void EditorScene::extend(const glm::ivec3& size) {
	m().extend(size);
}

void EditorScene::scaleHalf() {
	m().scaleHalf();
}

void EditorScene::fill(const glm::ivec3& pos) {
	m().fill(pos);
}

bool EditorScene::voxelizeModel(const video::MeshPtr& meshPtr) {
	const video::Mesh::Vertices& positions = meshPtr->vertices();
	const video::Mesh::Indices& indices = meshPtr->indices();

	if (indices.size() < 8) {
		Log::error("Not enough indices found: %i", (int)indices.size());
		return false;
	}

	vx_mesh_t* mesh = vx_color_mesh_alloc(positions.size(), indices.size());
	if (mesh == nullptr) {
		Log::error("Failed to allocate voxelize mesh");
		return false;
	}

	for (size_t f = 0; f < mesh->nindices; f++) {
		mesh->indices[f] = indices[f];
		mesh->normalindices[f] = indices[f];
	}

	for (size_t v = 0u; v < mesh->nvertices; ++v) {
		const video::Mesh::Vertices::value_type& vertex = positions[v];
		mesh->vertices[v].x = vertex._pos.x;
		mesh->vertices[v].y = vertex._pos.y;
		mesh->vertices[v].z = vertex._pos.z;
		mesh->normals[v].x = vertex._norm.x;
		mesh->normals[v].y = vertex._norm.y;
		mesh->normals[v].z = vertex._norm.z;
		mesh->colors[v].x = vertex._color.x;
		mesh->colors[v].y = vertex._color.y;
		mesh->colors[v].z = vertex._color.z;
	}

	const glm::vec3& meshMins = meshPtr->mins();
	const glm::vec3& meshMaxs = meshPtr->maxs();
	const glm::vec3& meshDimension = meshMaxs - meshMins;

	const voxel::RawVolume* model = m().modelVolume();
	const voxel::Region& region = model->region();
	const glm::vec3 regionDimension(region.getDimensionsInCells());
	const glm::vec3 factor = regionDimension / meshDimension;
	Log::debug("%f:%f:%f", factor.x, factor.y, factor.z);

	const float voxelSize = glm::min(glm::min(factor.x, factor.y), factor.z);
	const float precision = voxelSize / 10.0f;
	vx_point_cloud_t* result = vx_voxelize_pc(mesh, voxelSize, voxelSize, voxelSize, precision);
	Log::debug("Number of vertices: %i", (int)result->nvertices);

	for (size_t i = 0u; i < result->nvertices; ++i) {
		result->vertices[i].x -= meshMins.x;
		result->vertices[i].y -= meshMins.y;
		result->vertices[i].z -= meshMins.z;
	}
	m().pointCloud((const glm::vec3*)result->vertices, (const glm::vec3*)result->colors, result->nvertices);

	vx_point_cloud_free(result);
	vx_mesh_free(mesh);

	return true;
}

bool EditorScene::isEmpty() const {
	return m().empty();
}

void EditorScene::copy() {
	return m().copy();
}

void EditorScene::paste() {
	return m().paste();
}

void EditorScene::cut() {
	return m().cut();
}

void EditorScene::undo() {
	return m().undo();
}

void EditorScene::redo() {
	return m().redo();
}

bool EditorScene::canUndo() const {
	return m().undoHandler().canUndo();
}

bool EditorScene::canRedo() const {
	return m().undoHandler().canRedo();
}

bool EditorScene::exportModel(const std::string& file) {
	core_trace_scoped(EditorSceneExportModel);
	const io::FilePtr& filePtr = core::App::getInstance()->filesystem()->open(std::string(file), io::FileMode::Write);
	if (!(bool)filePtr) {
		return false;
	}
	return voxel::exportMesh(m().volumeRenderer().mesh(), filePtr->name().c_str());
}

bool EditorScene::prefab(const std::string& file) {
	core_trace_scoped(EditorSceneLoadModel);
	if (!m().prefab(file)) {
		return false;
	}
	return true;
}

bool EditorScene::loadModel(const std::string& file) {
	core_trace_scoped(EditorSceneLoadModel);
	if (!m().load(file)) {
		return false;
	}
	return true;
}

void EditorScene::noise(int octaves, float lacunarity, float frequency, float gain, voxel::noisegen::NoiseType type) {
	m().noise(octaves, lacunarity, frequency, gain, type);
}

void EditorScene::spaceColonization() {
	m().spaceColonization();
}

void EditorScene::lsystem(const voxel::lsystem::LSystemContext& ctx) {
	m().lsystem(ctx);
}

void EditorScene::createTree(const voxel::TreeContext& ctx) {
	m().createTree(ctx);
}

void EditorScene::createBuilding(voxel::BuildingType type, const voxel::BuildingContext& ctx) {
	m().createBuilding(type, ctx);
}

void EditorScene::createPlant(voxel::PlantType type) {
	m().createPlant(type);
}

void EditorScene::createCloud() {
	m().createCloud();
}

void EditorScene::createCactus() {
	m().createCactus();
}

void EditorScene::resetCamera() {
	_controller.resetCamera(m().modelVolume());
}

void EditorScene::setCursorVoxel(const voxel::Voxel& voxel) {
	m().setCursorVoxel(voxel);
}

void EditorScene::unselectAll() {
	m().unselectAll();
}

void EditorScene::select(const glm::ivec3& pos) {
	m().select(pos);
}

void EditorScene::bezier(const glm::ivec3& start, const glm::ivec3& end, const glm::ivec3& control) {
	m().bezier(start, end, control);
}

bool EditorScene::renderAABB() const {
	return m().gridRenderer().renderAABB();
}

void EditorScene::setRenderAABB(bool renderAABB) {
	Model& mdl = m();
	mdl.gridRenderer().setRenderAABB(renderAABB);
}

bool EditorScene::renderGrid() const {
	return m().gridRenderer().renderGrid();
}

void EditorScene::setRenderGrid(bool renderGrid) {
	m().gridRenderer().setRenderGrid(renderGrid);
}

inline long EditorScene::actionExecutionDelay() const {
	return m()._actionExecutionDelay;
}

void EditorScene::setActionExecutionDelay(long actionExecutionDelay) {
	m()._actionExecutionDelay = actionExecutionDelay;
}

bool EditorScene::renderAxis() const {
	return m().renderAxis();
}

void EditorScene::setRenderAxis(bool renderAxis) {
	m()._renderAxis = renderAxis;
}

bool EditorScene::renderLockAxis() const {
	return m()._renderLockAxis;
}

void EditorScene::setRenderLockAxis(bool renderLockAxis) {
	m()._renderLockAxis = renderLockAxis;
}

bool EditorScene::isDirty() const {
	return m().dirty();
}

bool EditorScene::OnEvent(const tb::TBWidgetEvent &ev) {
	core_trace_scoped(EditorSceneOnEvent);
	const uint64_t now = core::App::getInstance()->systemMillis();
	Model& mdl = m();
	bool& mouseDown = _controller._mouseDown;
	if (ev.type == tb::EVENT_TYPE_POINTER_DOWN) {
		if (ev.button_type == tb::TB_LEFT || ev.button_type == tb::TB_RIGHT) {
			mouseDown = true;
			mdl.executeAction(now, true);
			setInternalAction(mdl.keyAction());
			return true;
		}
	} else if (ev.type == tb::EVENT_TYPE_POINTER_UP) {
		if (ev.button_type == tb::TB_LEFT) {
			mouseDown = false;
			mdl.executeAction(now, false);
			setInternalAction(Action::None);
			return true;
		} else if (ev.button_type == tb::TB_RIGHT) {
			mouseDown = false;
			mdl.executeAction(now, false);
			setInternalAction(Action::None);
			return true;
		}
	} else if (ev.type == tb::EVENT_TYPE_KEY_UP) {
		if (ev.special_key == tb::TB_KEY_SHIFT) {
			const Action action = mdl.evalAction();
			const bool deleteVoxels = action == Action::DeleteVoxel;
			const bool overwrite = deleteVoxels ? true : action == Action::OverrideVoxel;
			if (mdl.aabbEnd(deleteVoxels, overwrite)) {
				return true;
			}
		}
	} else if (ev.type == tb::EVENT_TYPE_KEY_DOWN) {
		if (ev.special_key == tb::TB_KEY_SHIFT && mdl.aabbStart()) {
			return true;
		} else if (ev.modifierkeys) {
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
		if (ev.modifierkeys & tb::TB_SHIFT) {
			scaleCursorShape(glm::vec3(ev.delta_y * 2));
		} else {
			_controller.zoom((float)(ev.delta_y * 100));
		}
		return true;
	} else if (ev.type == tb::EVENT_TYPE_POINTER_MOVE) {
		const bool relative = isRelativeMouseMode();
		const bool middle = isMiddleMouseButtonPressed();
		const bool alt = mdl.action() == Action::None && (ev.modifierkeys & tb::TB_ALT);
		if (_controller.move(relative || middle || alt, ev.target_x, ev.target_y)) {
			if (mouseDown) {
				// execute a defined action while the mouse is moving
				mdl.executeAction(now, true);
			}
		}
		mdl.setMousePos(ev.target_x, ev.target_y);
		return true;
	}
	return Super::OnEvent(ev);
}

void EditorScene::OnResized(int oldw, int oldh) {
	core_trace_scoped(EditorSceneOnResized);
	Super::OnResized(oldw, oldh);
	const tb::TBRect& rect = GetRect();
	const glm::ivec2 dim(rect.w, rect.h);
	_controller.onResize(dim);
	_frameBuffer.shutdown();
	video::TextureConfig textureCfg;
	textureCfg.wrap(video::TextureWrap::ClampToEdge);
	video::FrameBufferConfig cfg;
	cfg.dimension(dim).depthBuffer(true).depthBufferFormat(video::TextureFormat::D24).addTextureAttachment(textureCfg);
	_frameBuffer.init(cfg);
	const video::TexturePtr& fboTexture = _frameBuffer.texture(video::FrameBufferAttachment::Color0);
	_bitmap.Init(dim.x, dim.y, fboTexture->handle());
	m().onResize(dim);
}

void EditorScene::OnPaint(const PaintProps &paintProps) {
	core_trace_scoped(EditorSceneOnPaint);
	Super::OnPaint(paintProps);
	const glm::ivec2& dimension = _frameBuffer.dimension();
	ui::turbobadger::UIRect rect = GetRect();
	rect.x = 0;
	rect.y = 0;
	const glm::vec4& uv = _frameBuffer.uv();
	const glm::vec2 uva(uv.x, uv.y);
	const glm::vec2 uvc(uv.z, uv.w);

	const float x = uva.x * dimension.x;
	const float y = uva.y * dimension.y;

	const float w = (uvc.x - uva.x) * dimension.x;
	const float h = (uvc.y - uva.y) * dimension.y;

	const tb::TBRect srcRect(x, y, w, h);
	tb::g_renderer->DrawBitmap(rect, srcRect, &_bitmap);
	tb::TBFontFace* font = GetFont();
	font->DrawString(0, 0, tb::TBColor(255.0f, 255.0f, 255.0f, 255.0f), _cameraMode.c_str());
}

void EditorScene::OnInflate(const tb::INFLATE_INFO &info) {
	Super::OnInflate(info);

	Controller::SceneCameraMode mode = Controller::SceneCameraMode::Free;
	const char *cameraMode = info.node->GetValueString("camera", "free");
	if (!strcmp(cameraMode, "top")) {
		mode = Controller::SceneCameraMode::Top;
	} else if (!strcmp(cameraMode, "front")) {
		mode = Controller::SceneCameraMode::Front;
	} else if (!strcmp(cameraMode, "left")) {
		mode = Controller::SceneCameraMode::Left;
	}
	_cameraMode = cameraMode;
	_controller.init(mode);
}

void EditorScene::update() {
	m().update();
}

void EditorScene::OnProcess() {
	Super::OnProcess();
	if (!GetVisibilityCombined()) {
		return;
	}
	core_trace_scoped(EditorSceneOnProcess);

	const long deltaFrame = core::App::getInstance()->deltaFrame();
	_controller.update(deltaFrame);

	if (tb::TBWidget::hovered_widget == this) {
		m().trace(_controller.camera());
	}

	video::clearColor(core::Color::Clear);
	core_trace_scoped(EditorSceneRenderFramebuffer);
	_frameBuffer.bind(true);
	render();
	_frameBuffer.unbind();
}

namespace tb {
TB_WIDGET_FACTORY(EditorScene, TBValue::TYPE_NULL, WIDGET_Z_TOP) {}
}
