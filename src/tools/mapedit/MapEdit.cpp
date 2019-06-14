/**
 * @file
 */

#include "MapEdit.h"

#include "video/Shader.h"
#include "video/Renderer.h"
#include "core/GLM.h"
#include "core/GameConfig.h"
#include "core/Color.h"
#include "core/command/Command.h"
#include "voxel/polyvox/Voxel.h"
#include "voxel/polyvox/Picking.h"
#include "io/Filesystem.h"
#include "ui/imgui/IMGUI.h"
#include "frontend/Movement.h"
#include "voxel/MaterialColor.h"

MapEdit::MapEdit(const metric::MetricPtr& metric, const video::MeshPoolPtr& meshPool, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, const voxel::WorldMgrPtr& world) :
		Super(metric, filesystem, eventBus, timeProvider), _camera(), _meshPool(meshPool), _worldRenderer(world), _worldMgr(world) {
	init(ORGANISATION, "mapedit");
	_worldMgr->setClientData(true);
}

MapEdit::~MapEdit() {
}

core::AppState MapEdit::onConstruct() {
	core::AppState state = Super::onConstruct();

	_speed = core::Var::get(cfg::ClientMouseSpeed, "0.1");
	_rotationSpeed = core::Var::getSafe(cfg::ClientMouseRotationSpeed);

	_movement.construct();

	core::Command::registerCommand("+linemode", [&] (const core::CmdArgs& args) { \
		if (args.empty()) {
			return;
		}
		_lineModeRendering = args[0] == "true";
	}).setHelp("Toggle line rendering mode");

	core::Var::get(cfg::VoxelMeshSize, "16", core::CV_READONLY);

	core::Command::registerCommand("freelook", [this] (const core::CmdArgs& args) {
		this->_freelook ^= true;
	}).setHelp("Toggle free look");

	_worldRenderer.construct();
	_worldMgr->setPersist(false);

	return state;
}

core::AppState MapEdit::onInit() {
	core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}

	video::enableDebug(video::DebugSeverity::High);

	if (!_axis.init()) {
		return core::AppState::InitFailure;
	}

	if (!_movement.init()) {
		return core::AppState::InitFailure;
	}

	if (!voxel::initDefaultMaterialColors()) {
		Log::error("Failed to initialize the palette data");
		return core::AppState::InitFailure;
	}

	if (!_worldMgr->init(filesystem()->load("worldparams.lua"), filesystem()->load("biomes.lua"))) {
		return core::AppState::InitFailure;
	}

	_worldMgr->setSeed(1);
	if (!_worldRenderer.init(glm::ivec2(0), _pixelDimension)) {
		return core::AppState::InitFailure;
	}
	_camera.init(glm::ivec2(0), pixelDimension(), screenDimension());
	_camera.setFieldOfView(45.0f);
	_camera.setPosition(glm::vec3(50.0f, 100.0f, 50.0f));
	_camera.lookAt(glm::vec3(0.0f, 0.0f, 0.0f));

	_worldRenderer.extractMeshes(_camera);

	_meshPool->init();

	const char *meshName = "chr_skelett";
	const video::MeshPtr& mesh = _meshPool->getMesh(meshName);
	if (!mesh) {
		Log::error("Failed to load the mesh '%s'", meshName);
		return core::AppState::InitFailure;
	}
	_entity = std::make_shared<frontend::ClientEntity>(1, network::EntityType::NONE, _camera.position(), 0.0f, mesh);
	if (!_worldRenderer.addEntity(_entity)) {
		Log::error("Failed to create entity");
		return core::AppState::InitFailure;
	}

	glm::vec3 targetPos = _camera.position();
	targetPos.x += 1000.0f;
	targetPos.z += 1000.0f;
	_entity->lerpPosition(targetPos, _entity->orientation());

	_worldTimer.init();

	return state;
}

void MapEdit::beforeUI() {
	Super::beforeUI();
	ScopedProfiler<ProfilerCPU> but(_beforeUiTimer);

	const glm::vec3& moveDelta = _movement.moveDelta(_speed->floatVal());
	_camera.move(moveDelta);
	if (!_freelook) {
		const glm::vec3& groundPosition = _worldRenderer.groundPosition(_camera.position());
		_camera.setPosition(groundPosition);
	}
	_camera.setFarPlane(_worldRenderer.getViewDistance());
	_camera.update(_deltaFrameMillis);

	if (_updateWorld) {
		_worldRenderer.extractMeshes(_camera);
		_worldRenderer.onRunning(_camera, _deltaFrameMillis);
	}
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

void MapEdit::onRenderUI() {
	const glm::vec3& pos = _camera.position();
	voxelrender::WorldRenderer::Stats stats;
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
	ImGui::CheckboxVar("Occlusion Query", cfg::OcclusionQuery);
	ImGui::CheckboxVar("Render Occlusion Queries", cfg::RenderOccluded);
	ImGui::CheckboxVar("Render AABB", cfg::RenderAABB);
	ImGui::CheckboxVar("Shadowmap render", cfg::ClientShadowMapShow);
	ImGui::CheckboxVar("Shadowmap cascades", cfg::ClientDebugShadowMapCascade);
	ImGui::CheckboxVar("Shadowmap debug", cfg::ClientDebugShadow);

	ImGui::Checkbox("Line mode rendering", &_lineModeRendering);
	ImGui::Checkbox("Freelook", &_freelook);
	ImGui::Checkbox("Update World", &_updateWorld);

	bool temp = _renderTracing;
	if (ImGui::Checkbox("Toggle profiler", &temp)) {
		_renderTracing = toggleTrace();
	}

	ImGui::Text("+/-: change move speed");
	ImGui::Text("l: line mode rendering");
}

core::AppState MapEdit::onRunning() {
	core_trace_scoped(MapEditOnRunning);
	ScopedProfiler<ProfilerCPU> wt(_frameTimer);
	const core::AppState state = Super::onRunning();

	_movement.update(_deltaFrameMillis);

	const bool current = isRelativeMouseMode();
	if (current) {
		_camera.rotate(glm::vec3(_mouseRelativePos.y, _mouseRelativePos.x, 0.0f) * _rotationSpeed->floatVal());
	}

	_axis.render(_camera);
	//glm::vec3 entPos = _entity->position();
	//entPos.y = _world->findFloor(entPos.x, entPos.z, voxel::isFloor);
	_entity->update(_deltaFrameMillis);
	return state;
}

core::AppState MapEdit::onCleanup() {
	_meshPool->shutdown();
	_worldRenderer.shutdown();
	_worldTimer.shutdown();
	_axis.shutdown();
	_movement.shutdown();
	_entity = frontend::ClientEntityPtr();
	const core::AppState state = Super::onCleanup();
	_worldMgr->shutdown();
	return state;
}

void MapEdit::onWindowResize() {
	Super::onWindowResize();
	_camera.init(glm::ivec2(0), pixelDimension(), screenDimension());
}

bool MapEdit::onKeyPress(int32_t key, int16_t modifier) {
	if (key == SDLK_ESCAPE) {
		toggleRelativeMouseMode();
	}
	return Super::onKeyPress(key, modifier);
}

void MapEdit::onMouseButtonPress(int32_t x, int32_t y, uint8_t button, uint8_t clicks) {
	Super::onMouseButtonPress(x, y, button, clicks);
	const video::Ray& ray = _camera.mouseRay(glm::ivec2(_mousePos.x, _mousePos.y));
	const glm::vec3& dirWithLength = ray.direction * _camera.farPlane();
	// TODO: select relevant procgen entity by querying the (not yet existant) octree.
	const voxel::PickResult& result = _worldMgr->pickVoxel(ray.origin, dirWithLength);
	if (result.didHit && button == SDL_BUTTON_RIGHT) {
		// TODO: show popup on existing procgen entity to change its properties or delete it
		_worldMgr->setVoxel(result.hitVoxel, voxel::createVoxel(voxel::VoxelType::Air, 0));
	} else if (result.validPreviousPosition && button == SDL_BUTTON_LEFT) {
		// TODO: place a new procgen entity
		_worldMgr->setVoxel(result.previousPosition, voxel::createRandomColorVoxel(voxel::VoxelType::Grass));
	}
}

int main(int argc, char *argv[]) {
	const video::MeshPoolPtr& meshPool = std::make_shared<video::MeshPool>();
	const core::EventBusPtr& eventBus = std::make_shared<core::EventBus>();
	const voxel::WorldMgrPtr& world = std::make_shared<voxel::WorldMgr>();
	const io::FilesystemPtr& filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr& timeProvider = std::make_shared<core::TimeProvider>();
	const metric::MetricPtr& metric = std::make_shared<metric::Metric>();
	MapEdit app(metric, meshPool, filesystem, eventBus, timeProvider, world);
	return app.startMainLoop(argc, argv);
}
