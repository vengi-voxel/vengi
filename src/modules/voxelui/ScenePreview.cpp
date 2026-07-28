/**
 * @file
 */

#include "ScenePreview.h"
#include "app/App.h"
#include "app/I18N.h"
#include "core/Common.h"
#include "core/Log.h"
#include "dearimgui/imgui.h"
#include "io/Filesystem.h"
#include "io/FilesystemArchive.h"
#include "io/FilesystemEntry.h"
#include "io/FormatDescription.h"
#include "ui/IMGUIApp.h"
#include "video/Renderer.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelrender/RenderUtil.h"

namespace voxelui {

ScenePreview::ScenePreview(const core::TimeProviderPtr &timeProvider)
	: _timeProvider(timeProvider), _renderer(timeProvider) {
	core_assert_msg((bool)timeProvider, "ScenePreview requires a TimeProvider");
	_renderContext.sceneGraph = &_sceneGraph;
	_renderContext.renderMode = voxelrender::RenderMode::Scene;
	_renderContext.onlyModels = true;
	_renderContext.enableBloom = false;
}

ScenePreview::~ScenePreview() {
	shutdown();
}

bool ScenePreview::ensureInit() {
	if (_initialized) {
		return true;
	}
	return init();
}

bool ScenePreview::init() {
	if (_initialized) {
		return true;
	}
	_meshState = core::make_shared<voxel::MeshState>();
	_meshState->construct();
	if (!_meshState->init()) {
		Log::error("Failed to init MeshState for ScenePreview");
		return false;
	}
	_renderer.construct();
	if (!_renderer.init(_meshState->hasNormals())) {
		Log::error("Failed to init SceneGraphRenderer for ScenePreview");
		return false;
	}
	if (!_renderContext.init(glm::ivec2(64, 64))) {
		Log::error("Failed to init RenderContext for ScenePreview");
		return false;
	}
	_renderContext.sceneGraph = &_sceneGraph;
	_camera.setMode(video::CameraMode::Perspective);
	_camera.setType(video::CameraType::Free);
	_camera.setRotationType(video::CameraRotationType::Target);
	_camera.setFieldOfView(45.0f);
	_initialized = true;
	return true;
}

void ScenePreview::shutdown() {
	if (!_initialized) {
		return;
	}
	_renderer.clear(_meshState);
	_renderer.shutdown();
	_renderContext.shutdown();
	if (_meshState) {
		(void)_meshState->shutdown();
		_meshState = voxel::MeshStatePtr();
	}
	_sceneGraph.clear();
	_loadedPath.clear();
	_fbSize = glm::ivec2(0);
	_initialized = false;
}

void ScenePreview::clear() {
	if (_initialized) {
		_renderer.clear(_meshState);
	}
	_sceneGraph.clear();
	_loadedPath.clear();
	_cameraDirty = true;
	_renderContext.sceneGraph = &_sceneGraph;
}

bool ScenePreview::empty() const {
	return _sceneGraph.empty();
}

const core::String &ScenePreview::loadedPath() const {
	return _loadedPath;
}

bool ScenePreview::load(const core::String &path) {
	if (!ensureInit()) {
		return false;
	}
	if (path.empty()) {
		clear();
		return false;
	}
	if (path == _loadedPath && !empty()) {
		return true;
	}
	if (!voxelformat::isModelFormat(path)) {
		clear();
		return false;
	}
	if (voxelformat::isMeshFormat(path, false)) {
		// Mesh voxelization is too heavy for interactive previews
		clear();
		_loadedPath = path;
		return false;
	}

	scenegraph::SceneGraph loaded;
	voxelformat::LoadContext loadCtx;
	io::FileDescription fileDesc;
	fileDesc.set(path);
	const io::ArchivePtr &archive = io::openFilesystemArchive(io::filesystem());
	if (!voxelformat::loadFormat(fileDesc, archive, loaded, loadCtx)) {
		clear();
		_loadedPath = path; // avoid reload spam on persistent failure
		return false;
	}
	_renderer.clear(_meshState);
	_sceneGraph = core::move(loaded);
	_renderContext.sceneGraph = &_sceneGraph;
	_loadedPath = path;
	_cameraDirty = true;
	return true;
}

bool ScenePreview::setSceneGraph(scenegraph::SceneGraph &&sceneGraph) {
	if (!ensureInit()) {
		return false;
	}
	_renderer.clear(_meshState);
	_sceneGraph = core::move(sceneGraph);
	_renderContext.sceneGraph = &_sceneGraph;
	_loadedPath.clear();
	_cameraDirty = true;
	return !empty();
}

void ScenePreview::resetCamera() {
	if (empty()) {
		return;
	}
	const voxel::Region &region = _sceneGraph.sceneRegion();
	if (!region.isValid()) {
		return;
	}
	const glm::vec3 angles(glm::radians(-25.0f), glm::radians(45.0f), 0.0f);
	voxelrender::configureCamera(_camera, region, voxelrender::SceneCameraMode::Free, 5000.0f, angles);
	_cameraDirty = false;
}

glm::ivec2 ScenePreview::pixelSizeFor(const glm::vec2 &widgetSize) const {
	glm::vec2 scale(1.0f);
	if (ui::IMGUIApp *app = imguiApp()) {
		const glm::vec2 &windowSize = app->windowDimension();
		const glm::vec2 &fbSize = app->frameBufferDimension();
		if (windowSize.x > 0.0f && windowSize.y > 0.0f) {
			scale = fbSize / windowSize;
		}
	}
	const int w = core_max(1, (int)(widgetSize.x * scale.x));
	const int h = core_max(1, (int)(widgetSize.y * scale.y));
	return glm::ivec2(w, h);
}

void ScenePreview::resize(const glm::ivec2 &pixelSize) {
	if (_fbSize == pixelSize) {
		return;
	}
	_fbSize = pixelSize;
	_camera.setSize(pixelSize);
	_renderContext.resize(pixelSize);
}

void ScenePreview::handleOrbitInput(const glm::vec2 &size) {
	ImGui::InvisibleButton("##scenepreview_orbit", ImVec2(size.x, size.y));
	const bool hovered = ImGui::IsItemHovered();
	if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
		const ImVec2 delta = ImGui::GetIO().MouseDelta;
		_camera.turn(delta.x * 0.01f);
		_camera.setPitch(-delta.y * 0.01f);
	}
	if (hovered) {
		const float wheel = ImGui::GetIO().MouseWheel;
		if (wheel != 0.0f) {
			const float dist = _camera.targetDistance();
			_camera.setTargetDistance(core_max(1.0f, dist - wheel * core_max(1.0f, dist * 0.08f)));
		}
		if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
			_cameraDirty = true;
		}
	}
}

void ScenePreview::renderFrame(double deltaSeconds) {
	if (_cameraDirty) {
		resetCamera();
	}
	_camera.update(deltaSeconds);
	video::clearColor(glm::vec4(0.15f, 0.15f, 0.15f, 1.0f));
	video::enable(video::State::DepthTest);
	video::depthFunc(video::CompareFunc::LessEqual);
	video::enable(video::State::CullFace);
	video::enable(video::State::DepthMask);
	video::enable(video::State::Blend);
	video::blendFunc(video::BlendMode::SourceAlpha, video::BlendMode::OneMinusSourceAlpha);

	_renderContext.frameBuffer.bind(true);
	_renderer.render(_meshState, _renderContext, _camera, true, true);
	_renderContext.frameBuffer.unbind();
}

void ScenePreview::drawImage(const glm::vec2 &size) const {
	const video::FrameBuffer &fb = _renderContext.enableMultisampling ? _renderContext.resolveFrameBuffer
																	  : _renderContext.frameBuffer;
	const glm::vec4 &uv = fb.uv();
	const video::TexturePtr &texture = fb.texture(video::FrameBufferAttachment::Color0);
	ImGui::Image(texture->handle(), ImVec2(size.x, size.y), ImVec2(uv.x, uv.y), ImVec2(uv.z, uv.w));
}

void ScenePreview::updateAndRender(double deltaSeconds, const glm::vec2 &size, bool interactive) {
	if (!ensureInit()) {
		ImGui::TextUnformatted(_("Preview unavailable"));
		return;
	}
	if (size.x < 1.0f || size.y < 1.0f) {
		return;
	}
	if (empty()) {
		ImGui::BeginChild("##scenepreview_empty", ImVec2(size.x, size.y), ImGuiChildFlags_Borders);
		ImGui::TextWrapped("%s", _("No model"));
		ImGui::EndChild();
		return;
	}

	const ImVec2 cursor = ImGui::GetCursorScreenPos();
	resize(pixelSizeFor(size));
	renderFrame(deltaSeconds);
	drawImage(size);
	if (interactive) {
		ImGui::SetCursorScreenPos(cursor);
		handleOrbitInput(size);
		ImGui::SetItemTooltip("%s", _("Drag to orbit, wheel to zoom, double-click to reset"));
	}
}

void ScenePreview::renderForFileDialog(const io::FilesystemEntry &entry, video::OpenFileMode mode,
									   const io::FormatDescription *desc) {
	(void)desc;
	const float size = 20.0f * ImGui::GetTextLineHeightWithSpacing();
	ImGui::BeginChild("##filedialog_scenepreview", ImVec2(size, size), ImGuiChildFlags_Borders,
					  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	if (mode == video::OpenFileMode::Directory || entry.name.empty() || entry.isDirectory()) {
		ImGui::TextWrapped("%s", _("Select a model"));
		ImGui::EndChild();
		return;
	}

	core::String path = entry.fullPath;
	if (path.empty()) {
		path = entry.name;
	}
	if (voxelformat::isMeshFormat(path, false)) {
		ImGui::TextWrapped("%s", _("Mesh preview not available"));
		ImGui::EndChild();
		return;
	}
	if (!voxelformat::isModelFormat(path)) {
		ImGui::TextWrapped("%s", _("No preview"));
		ImGui::EndChild();
		return;
	}

	if (path != _loadedPath) {
		if (!load(path)) {
			ImGui::TextWrapped("%s", _("Failed to load preview"));
			ImGui::EndChild();
			return;
		}
	}

	const ImVec2 avail = ImGui::GetContentRegionAvail();
	const double delta = app::App::getInstance()->deltaFrameSeconds();
	updateAndRender(delta, glm::vec2(avail.x, avail.y), true);
	ImGui::EndChild();
}

} // namespace voxelui
