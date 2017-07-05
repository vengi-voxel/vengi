/**
 * @file
 */

#include "WorldRendererTool.h"
#include "video/Shader.h"
#include "video/Renderer.h"
#include "core/GLM.h"
#include "core/GameConfig.h"
#include "core/Color.h"
#include "voxel/polyvox/Voxel.h"
#include "voxel/polyvox/Picking.h"
#include "io/Filesystem.h"
#include "imgui/IMGUI.h"
#include "frontend/Movement.h"
#include "voxel/MaterialColor.h"

WorldRendererTool::WorldRendererTool(const video::MeshPoolPtr& meshPool, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, const voxel::WorldPtr& world) :
		Super(filesystem, eventBus, timeProvider), _camera(), _meshPool(meshPool), _worldRenderer(world), _world(world) {
	init(ORGANISATION, "worldrenderertool");
	_world->setClientData(true);
}

WorldRendererTool::~WorldRendererTool() {
}

core::AppState WorldRendererTool::onConstruct() {
	core::AppState state = Super::onConstruct();

	_speed = core::Var::get(cfg::ClientMouseSpeed, "0.1");
	_rotationSpeed = core::Var::getSafe(cfg::ClientMouseRotationSpeed);

	core::Command::registerCommand("+linemode", [&] (const core::CmdArgs& args) { \
		if (args.empty()) {
			return;
		}
		_lineModeRendering = args[0] == "true";
	}).setHelp("Toggle line rendering mode");

	registerMoveCmd("+move_right", MOVERIGHT);
	registerMoveCmd("+move_left", MOVELEFT);
	registerMoveCmd("+move_forward", MOVEFORWARD);
	registerMoveCmd("+move_backward", MOVEBACKWARD);
	core::Var::get(cfg::VoxelMeshSize, "16", core::CV_READONLY);

	core::Command::registerCommand("freelook", [this] (const core::CmdArgs& args) {
		this->_freelook ^= true;
	}).setHelp("Toggle free look");

	_worldRenderer.onConstruct();
	_world->setPersist(false);

	return state;
}

core::AppState WorldRendererTool::onInit() {
	core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}

	video::enableDebug(video::DebugSeverity::High);

	if (!_axis.init()) {
		return core::AppState::Cleanup;
	}

	if (!voxel::initDefaultMaterialColors()) {
		Log::error("Failed to initialize the palette data");
		return core::AppState::Cleanup;
	}

	if (!_world->init(filesystem()->load("world.lua"), filesystem()->load("biomes.lua"))) {
		return core::AppState::Cleanup;
	}

	_world->setSeed(1);
	if (!_worldRenderer.init(glm::ivec2(), _dimension)) {
		return core::AppState::Cleanup;
	}
	_camera.init(glm::ivec2(), dimension());
	_camera.setFieldOfView(45.0f);
	_camera.setPosition(glm::vec3(50.0f, 100.0f, 50.0f));
	_camera.lookAt(glm::vec3(0.0f, 0.0f, 0.0f));

	_worldRenderer.extractMeshes(_camera);

	_meshPool->init();

	const char *meshName = "chr_skelett2_bake";
	const video::MeshPtr& mesh = _meshPool->getMesh(meshName);
	if (!mesh) {
		Log::error("Failed to load the mesh '%s'", meshName);
		return core::AppState::Cleanup;
	}
	_entity = std::make_shared<frontend::ClientEntity>(1, network::EntityType::NONE, _camera.position(), 0.0f, mesh);
	if (!_worldRenderer.addEntity(_entity)) {
		Log::error("Failed to create entity");
		return core::AppState::Cleanup;
	}

	glm::vec3 targetPos = _camera.position();
	targetPos.x += 1000.0f;
	targetPos.z += 1000.0f;
	_entity->lerpPosition(targetPos, _entity->orientation());

	_worldTimer.init();

	return state;
}

void WorldRendererTool::beforeUI() {
	Super::beforeUI();
	ScopedProfiler<ProfilerCPU> but(_beforeUiTimer);
	_world->onFrame(_deltaFrame);

	if (_resetTriggered && !_world->isReset()) {
		_world->setContext(_ctx);
		_worldRenderer.extractMeshes(_camera);
		_resetTriggered = false;
	}

	const float speed = _speed->floatVal() * static_cast<float>(_deltaFrame);
	const glm::vec3& moveDelta = getMoveDelta(speed, _moveMask);
	_camera.move(moveDelta);
	if (!_freelook) {
		const glm::vec3& groundPosition = _worldRenderer.groundPosition(_camera.position());
		_camera.setPosition(groundPosition);
	}
	_camera.setFarPlane(_worldRenderer.getViewDistance());
	_camera.update(_deltaFrame);

	_worldRenderer.extractMeshes(_camera);
	_worldRenderer.onRunning(_camera, _deltaFrame);
	ScopedProfiler<video::ProfilerGPU> wt(_worldTimer);
	if (_lineModeRendering) {
		video::polygonMode(video::Face::FrontAndBack, video::PolygonMode::WireFrame);
	}
	_drawCallsWorld = _worldRenderer.renderWorld(_camera, &_vertices);
	_drawCallsEntities = _worldRenderer.renderEntities(_camera);
	if (_lineModeRendering) {
		video::polygonMode(video::Face::FrontAndBack, video::PolygonMode::Solid);
	}
}

void WorldRendererTool::onRenderUI() {
	const glm::vec3& pos = _camera.position();
	frontend::WorldRenderer::Stats stats;
	_worldRenderer.stats(stats);
	ImGui::Text("%s: %f, max: %f", _frameTimer.name().c_str(), _frameTimer.avg(), _frameTimer.maximum());
	ImGui::Text("%s: %f, max: %f", _beforeUiTimer.name().c_str(), _beforeUiTimer.avg(), _beforeUiTimer.maximum());
	ImGui::Text("%s: %f, max: %f", _worldTimer.name().c_str(), _worldTimer.avg(), _worldTimer.maximum());
	ImGui::Text("drawcalls world: %i (verts: %i)", _drawCallsWorld, _vertices);
	ImGui::Text("drawcalls entities: %i", _drawCallsEntities);
	ImGui::Text("pos: %.2f:%.2f:%.2f", pos.x, pos.y, pos.z);
	ImGui::Text("pending: %i, meshes: %i, extracted: %i, uploaded: %i, visible: %i, octreesize: %i, octreeactive: %i, occluded: %i",
			stats.pending, stats.meshes, stats.extracted, stats.active, stats.visible, stats.octreeSize, stats.octreeActive, stats.occluded);
	const bool current = isRelativeMouseMode();
	ImGui::Text("world mouse mode: %s", (current ? "true" : "false"));

	ImGui::InputVarFloat("speed", _speed);
	ImGui::InputVarFloat("rotationSpeed", _rotationSpeed);

	ImGui::Checkbox("Line mode rendering", &_lineModeRendering);
	ImGui::Checkbox("Freelook", &_freelook);

	ImGui::Text("+/-: change move speed");
	ImGui::Text("l: line mode rendering");
}

core::AppState WorldRendererTool::onRunning() {
	ScopedProfiler<ProfilerCPU> wt(_frameTimer);
	const core::AppState state = Super::onRunning();

	const bool current = isRelativeMouseMode();
	if (current) {
		_camera.rotate(glm::vec3(_mouseRelativePos.y, _mouseRelativePos.x, 0.0f) * _rotationSpeed->floatVal());
	}

	_axis.render(_camera);
	//glm::vec3 entPos = _entity->position();
	//entPos.y = _world->findFloor(entPos.x, entPos.z, voxel::isFloor);
	_entity->update(_deltaFrame);
	return state;
}

core::AppState WorldRendererTool::onCleanup() {
	_meshPool->shutdown();
	_worldRenderer.shutdown();
	_worldTimer.shutdown();
	_axis.shutdown();
	_entity = frontend::ClientEntityPtr();
	const core::AppState state = Super::onCleanup();
	_world->shutdown();
	return state;
}

void WorldRendererTool::onWindowResize() {
	Super::onWindowResize();
	_camera.init(glm::ivec2(), dimension());
}

bool WorldRendererTool::onKeyPress(int32_t key, int16_t modifier) {
	if (key == SDLK_ESCAPE) {
		toggleRelativeMouseMode();
	} else if (key == SDLK_PLUS || key == SDLK_KP_PLUS) {
		const float speed = _speed->floatVal() + 0.1f;
		_speed->setVal(speed);
	} else if (key == SDLK_MINUS || key == SDLK_KP_MINUS) {
		const float speed = std::max(0.1f, _speed->floatVal() - 0.1f);
		_speed->setVal(speed);
	}
	return Super::onKeyPress(key, modifier);
}

void WorldRendererTool::onMouseButtonPress(int32_t x, int32_t y, uint8_t button, uint8_t clicks) {
	Super::onMouseButtonPress(x, y, button, clicks);
	const video::Ray& ray = _camera.mouseRay(glm::ivec2(_mousePos.x, _mousePos.y));
	const glm::vec3& dirWithLength = ray.direction * _camera.farPlane();
	const voxel::PickResult& result = _world->pickVoxel(ray.origin, dirWithLength);
	if (result.didHit && button == SDL_BUTTON_RIGHT) {
		_world->setVoxel(result.hitVoxel, voxel::createVoxel(voxel::VoxelType::Air, 0));
	} else if (result.validPreviousPosition && button == SDL_BUTTON_LEFT) {
		_world->setVoxel(result.previousPosition, voxel::createRandomColorVoxel(voxel::VoxelType::Grass));
	}
}

void WorldRendererTool::reset(const voxel::WorldContext& ctx) {
	_ctx = ctx;
	_worldRenderer.reset();
	_world->reset();
	_resetTriggered = true;
}

int main(int argc, char *argv[]) {
	const video::MeshPoolPtr meshPool = std::make_shared<video::MeshPool>();
	const core::EventBusPtr eventBus = std::make_shared<core::EventBus>();
	const voxel::WorldPtr world = std::make_shared<voxel::World>();
	const io::FilesystemPtr filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr timeProvider = std::make_shared<core::TimeProvider>();
	WorldRendererTool app(meshPool, filesystem, eventBus, timeProvider, world);
	return app.startMainLoop(argc, argv);
}
