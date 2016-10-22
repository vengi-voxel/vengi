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
#include "voxel/polyvox/CubicSurfaceExtractor.h"
#include "voxel/IsQuadNeeded.h"

VoxEdit::VoxEdit(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		ui::UIApp(filesystem, eventBus, timeProvider), _rawVolume(nullptr), _mesh(nullptr), _colorShader(shader::ColorShader::getInstance()) {
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
	if (_rawVolume != nullptr) {
		delete _rawVolume;
	}
	const voxel::Region region(glm::ivec3(-512), glm::ivec3(512));
	_rawVolume = new voxel::RawVolume(region);
	// TODO
	return false;
}

void VoxEdit::onWindowResize() {
	Super::onWindowResize();
	_camera.init(dimension());
	_camera.setAspectRatio(_aspect);
}

core::AppState VoxEdit::onInit() {
	const core::AppState state = Super::onInit();
	_logLevel->setVal(std::to_string(SDL_LOG_PRIORITY_DEBUG));
	Log::init();
	if (state == core::AppState::Cleanup) {
		return state;
	}

	GLDebug::enable(GLDebug::Medium);

	if (!_axis.init()) {
		return core::AppState::Cleanup;
	}

	if (!_plane.init(glm::zero<glm::vec3>())) {
		return core::AppState::Cleanup;
	}

	if (!_colorShader.setup()) {
		Log::error("Failed to initialize the color shader");
		return core::AppState::Cleanup;
	}

	_vertexBufferIndex = _vertexBuffer.create();
	if (_vertexBufferIndex == -1) {
		Log::error("Could not create the vertex buffer object");
		return core::AppState::Cleanup;
	}

	_indexBufferIndex = _vertexBuffer.create(nullptr, 0, GL_ELEMENT_ARRAY_BUFFER);
	if (_indexBufferIndex == -1) {
		Log::error("Could not create the vertex buffer object for the indices");
		return core::AppState::Cleanup;
	}

	_colorBufferIndex = _vertexBuffer.create();
	if (_colorBufferIndex == -1) {
		Log::error("Could not create the vertex buffer object for the colors");
		return core::AppState::Cleanup;
	}

	// configure shader attributes
	core_assert_always(_vertexBuffer.addAttribute(_colorShader.getLocationPos(), _vertexBufferIndex, _colorShader.getComponentsPos()));
	core_assert_always(_vertexBuffer.addAttribute(_colorShader.getLocationColor(), _colorBufferIndex, _colorShader.getComponentsColor()));

	_lastDirectory = core::Var::get("ve_lastdirectory", "");
	_rotationSpeed = core::Var::get(cfg::ClientMouseRotationSpeed, "0.01");

	Log::info("Set window dimensions: %ix%i (aspect: %f)", _dimension.x, _dimension.y, _aspect);
	_camera.init(dimension());
	_camera.setAspectRatio(_aspect);
	_camera.setPosition(glm::vec3(0.0f, 50.0f, 100.0f));
	_camera.lookAt(glm::vec3(0.0001f));

	registerMoveCmd("+move_right", MOVERIGHT);
	registerMoveCmd("+move_left", MOVELEFT);
	registerMoveCmd("+move_forward", MOVEFORWARD);
	registerMoveCmd("+move_backward", MOVEBACKWARD);

	_mesh = new voxel::Mesh(128, 128, true);

	registerWindows(this);
	registerActions(this);

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

	if (_extract) {
		_extract = false;
		voxel::extractCubicMesh(_rawVolume, _rawVolume->getEnclosingRegion(), _mesh, voxel::IsQuadNeeded(false), true, true);
		const voxel::IndexType* meshIndices = _mesh->getRawIndexData();
		const voxel::Vertex* meshVertices = _mesh->getRawVertexData();
		const size_t meshNumberIndices = _mesh->getNoOfIndices();
		if (meshNumberIndices == 0) {
			_pos.clear();
			_indices.clear();
			_colors.clear();
		} else {
			const size_t meshNumberVertices = _mesh->getNoOfVertices();
			_pos.reserve(meshNumberVertices);
			_indices.reserve(meshNumberIndices);
			for (size_t i = 0; i < meshNumberVertices; ++i) {
				_pos.emplace_back(meshVertices[i].position, 1.0f);
			}
			for (size_t i = 0; i < meshNumberIndices; ++i) {
				_indices.push_back(meshIndices[i]);
			}
			if (!_vertexBuffer.update(_vertexBufferIndex, _pos)) {
				Log::error("Failed to update the vertex buffer");
				requestQuit();
				return;
			}
			if (!_vertexBuffer.update(_indexBufferIndex, _indices)) {
				Log::error("Failed to update the index buffer");
				requestQuit();
				return;
			}
			if (!_vertexBuffer.update(_colorBufferIndex, _colors)) {
				Log::error("Failed to update the color buffer");
				requestQuit();
				return;
			}
		}
	}

	if  (_renderPlane) {
		_plane.render(_camera);
	}
	doRender();
	if (_renderAxis) {
		_axis.render(_camera);
	}
}

void VoxEdit::doRender() {
	if (_pos.empty()) {
		return;
	}
	video::ScopedShader scoped(_colorShader);
	core_assert_always(_colorShader.setView(_camera.viewMatrix()));
	core_assert_always(_colorShader.setProjection(_camera.projectionMatrix()));

	core_assert_always(_vertexBuffer.bind());
	const GLuint nIndices = _vertexBuffer.elements(_indexBufferIndex, 1, sizeof(uint32_t));
	core_assert(nIndices > 0);
	glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, nullptr);
	_vertexBuffer.unbind();
	GL_checkError();
}

void VoxEdit::afterUI() {
	Super::afterUI();
	enqueueShowStr(5, core::Color::Gray, "ESC: toggle camera free look");
}

core::AppState VoxEdit::onCleanup() {
	_axis.shutdown();
	_plane.shutdown();
	_vertexBuffer.shutdown();
	_colorShader.shutdown();
	_vertexBufferIndex = -1;
	_indexBufferIndex = -1;
	_colorBufferIndex = -1;

	core::Command::unregisterCommand("+move_right");
	core::Command::unregisterCommand("+move_left");
	core::Command::unregisterCommand("+move_upt");
	core::Command::unregisterCommand("+move_down");

	// TODO: saveFile(tmpFilename);
	if (_rawVolume != nullptr) {
		delete _rawVolume;
	}
	if (_mesh != nullptr) {
		delete _mesh;
	}
	_mesh = nullptr;
	_rawVolume = nullptr;
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
	const glm::vec3 v((float)x / (float)width(), (float)y / (float)height(), 0.0f);
	glm::vec3 worldPos = _camera.screenToWorld(v);
	Log::info("clicked to screen at %i:%i (%f:%f:%f)", x, y, worldPos.x, worldPos.y, worldPos.z);
	worldPos.y = 0.0f;
	_rawVolume->setVoxel(worldPos, voxel::createVoxel(voxel::Grass1));
	_dirty = true;
	_extract = true;
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
