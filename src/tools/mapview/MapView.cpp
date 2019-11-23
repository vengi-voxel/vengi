/**
 * @file
 */

#include "MapView.h"

#include "../../modules/stock/ContainerData.h"
#include "video/Shader.h"
#include "video/Renderer.h"
#include "core/GLM.h"
#include "core/GameConfig.h"
#include "core/Color.h"
#include "core/command/Command.h"
#include "voxel/Voxel.h"
#include "voxel/Picking.h"
#include "core/io/Filesystem.h"
#include "ui/imgui/IMGUI.h"
#include "frontend/Movement.h"
#include "voxel/MaterialColor.h"
#include "voxelgenerator/Spiral.h"
#include "attrib/Attributes.h"
#include "attrib/ContainerProvider.h"

MapView::MapView(const metric::MetricPtr& metric, const animation::AnimationCachePtr& animationCache,
		const stock::StockDataProviderPtr& stockDataProvider,
		const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus,
		const core::TimeProviderPtr& timeProvider, const voxelworld::WorldMgrPtr& world,
		const voxelformat::VolumeCachePtr& volumeCache) :
		Super(metric, filesystem, eventBus, timeProvider), _camera(),
		_animationCache(animationCache), _worldRenderer(world), _worldMgr(world), _stockDataProvider(stockDataProvider), _volumeCache(volumeCache) {
	init(ORGANISATION, "mapview");
}

MapView::~MapView() {
}

core::AppState MapView::onConstruct() {
	core::AppState state = Super::onConstruct();

	_rotationSpeed = core::Var::getSafe(cfg::ClientMouseRotationSpeed);

	_movement.construct();

	core::Command::registerCommand("bird", [&] (const core::CmdArgs& args) {
		glm::vec3 pos = _entity->position();
		pos.y += 100.0f;
		_entity->setPosition(pos);
	});

	core::Command::registerCommand("+linemode", [&] (const core::CmdArgs& args) {
		if (args.empty()) {
			return;
		}
		_lineModeRendering = args[0] == "true";
	}).setHelp("Toggle line rendering mode");

	_meshSize = core::Var::get(cfg::VoxelMeshSize, "16", core::CV_READONLY);

	_volumeCache->construct();
	_worldRenderer.construct();
	_worldMgr->setPersist(false);

	return state;
}

core::AppState MapView::onInit() {
	core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}

	video::enableDebug(video::DebugSeverity::High);

	if (!_axis.init()) {
		Log::error("Failed to init axis");
		return core::AppState::InitFailure;
	}

	if (!_volumeCache->init()) {
		Log::error("Failed to init volumeCache");
		return core::AppState::InitFailure;
	}

	if (!_movement.init()) {
		Log::error("Failed to init movement");
		return core::AppState::InitFailure;
	}

	if (!_stockDataProvider->init(filesystem()->load("stock.lua"))) {
		Log::error("Failed to init stock data provider: %s", _stockDataProvider->error().c_str());
		return core::AppState::InitFailure;
	}

	if (!voxel::initDefaultMaterialColors()) {
		Log::error("Failed to initialize the palette data");
		return core::AppState::InitFailure;
	}

	if (!_animationCache->init()) {
		Log::error("Failed to init mesh cache");
		return core::AppState::InitFailure;
	}

	if (!_worldMgr->init(filesystem()->load("worldparams.lua"), filesystem()->load("biomes.lua"))) {
		Log::error("Failed to init world mgr");
		return core::AppState::InitFailure;
	}

	_worldMgr->setSeed(1);
	if (!_worldRenderer.init(glm::ivec2(0), _frameBufferDimension)) {
		Log::error("Failed to init world renderer");
		return core::AppState::InitFailure;
	}

	_camera.init(glm::ivec2(0), frameBufferDimension(), windowDimension());
	_camera.setFarPlane(10.0f);
	_camera.setRotationType(video::CameraRotationType::Target);
	_camera.setFieldOfView(_fieldOfView);
	_camera.setTargetDistance(_targetDistance);
	_camera.setPosition(_cameraPosition);
	_camera.setTarget(glm::zero<glm::vec3>());
	_camera.setAngles(0.0f, 0.0f, 0.0f);
	_camera.update(0l);

	const int groundPosY = _worldMgr->findWalkableFloor(glm::zero<glm::vec3>());
	const glm::vec3 pos(0.0f, (float)groundPosY, 0.0f);
	Log::info("Spawn entity at %s", glm::to_string(pos).c_str());

	const network::EntityType entityType = network::EntityType::HUMAN_MALE_WORKER;
	const frontend::ClientEntityId entityId = (frontend::ClientEntityId)1;
	_entity = std::make_shared<frontend::ClientEntity>(_stockDataProvider, _animationCache, entityId, entityType, pos, 0.0f);
	attrib::ContainerProvider containerProvider;
	const std::string& attribLua = filesystem()->load("attributes.lua");
	if (!containerProvider.init(attribLua)) {
		Log::error("Failed to init attributes: %s", containerProvider.error().c_str());
		return core::AppState::InitFailure;
	}
	const std::string& entityTypeStr = network::EnumNameEntityType(_entity->type());
	const attrib::ContainerPtr& attribContainer = containerProvider.container(entityTypeStr);
	if (!attribContainer) {
		Log::error("Failed to load attributes for %s", entityTypeStr.c_str());
		return core::AppState::InitFailure;
	}
	attrib::Attributes attributes;
	attributes.add(attribContainer);
	attributes.update(0l);
	const float speed = attributes.max(attrib::Type::SPEED);
	_entity->attrib().setCurrent(attrib::Type::SPEED, speed);
	if (!_worldRenderer.addEntity(_entity)) {
		Log::error("Failed to create entity");
		return core::AppState::InitFailure;
	}
	stock::Stock& stock = _entity->stock();
	stock::Inventory& inv = stock.inventory();
	const stock::ContainerData* containerData = _stockDataProvider->containerData("tool");
	if (containerData == nullptr) {
		Log::error("Could not get container for items");
		return core::AppState::InitFailure;
	}
	const stock::ItemData* itemData = _stockDataProvider->itemData(1);
	if (itemData == nullptr) {
		Log::error("Failed to get item with id 1");
		return core::AppState::InitFailure;
	}
	const stock::ItemPtr& item = _stockDataProvider->createItem(itemData->id());
	if (!inv.add(containerData->id, item, 0, 0)) {
		Log::error("Failed to add item to inventory");
		return core::AppState::InitFailure;
	}

	_worldTimer.init();

	return state;
}

void MapView::beforeUI() {
	Super::beforeUI();
	ScopedProfiler<ProfilerCPU> but(_beforeUiTimer);

	_movement.updatePos(_camera, _deltaFrameSeconds, _entity, [&] (const glm::vec3& pos) {
		const float maxWalkHeight = 3.0f;
		return _worldMgr->findWalkableFloor(pos, maxWalkHeight);
	});
	_camera.update(_deltaFrameMillis);

	// TODO: implement collision in the camera interface
	// you need the WolrdMgr for collision detection. Collision management should be part of the surrounding system
	if (_updateWorld) {
		glm::vec3 targetpos = _camera.target();
		glm::vec3 direction = _camera.direction();
		glm::vec3 hit;

		if (_worldMgr->raycast(targetpos, direction, _targetDistance, [&] (const voxel::PagedVolume::Sampler& sampler) {
				voxel::Voxel voxel = sampler.voxel();
				if (voxel::isBlocked(voxel.getMaterial())) {
					// store position and abort raycast
					hit = glm::vec3(sampler.position());
					return false;
				}
				return true;
			})) {
			_camera.setTargetDistance(glm::distance(targetpos, hit));
		} else {
			_camera.setTargetDistance(_targetDistance);
		}

		_camera.setFarPlane(_worldRenderer.getViewDistance());
		if (!_singlePosExtraction) {
			_worldRenderer.extractMeshes(_camera);
		}
		_worldRenderer.onRunning(_camera, _deltaFrameMillis);
	}
	ScopedProfiler<video::ProfilerGPU> wt(_worldTimer);
	if (_lineModeRendering) {
		video::polygonMode(video::Face::FrontAndBack, video::PolygonMode::WireFrame);
	}
	_drawCallsWorld = _worldRenderer.renderWorld(_camera, &_vertices);
	if (_lineModeRendering) {
		video::polygonMode(video::Face::FrontAndBack, video::PolygonMode::Solid);
	}
}

void MapView::onRenderUI() {
	if (ImGui::CollapsingHeader("Stats")) {
		const glm::vec3& pos = _camera.position();
		const glm::vec3& targetpos = _camera.target();
		const float distance = _camera.targetDistance();
		const float pitch = _camera.pitch();
		const float yaw = _camera.yaw();
		voxelrender::WorldRenderer::Stats stats;
		_worldRenderer.stats(stats);
		ImGui::Text("%s: %f, max: %f", _frameTimer.name().c_str(), _frameTimer.avg(), _frameTimer.maximum());
		ImGui::Text("%s: %f, max: %f", _beforeUiTimer.name().c_str(), _beforeUiTimer.avg(), _beforeUiTimer.maximum());
		ImGui::Text("%s: %f, max: %f", _worldTimer.name().c_str(), _worldTimer.avg(), _worldTimer.maximum());
		ImGui::Text("Drawcalls: %i (verts: %i)", _drawCallsWorld, _vertices);
		ImGui::Text("Target Pos: %.2f:%.2f:%.2f ", targetpos.x, targetpos.y, targetpos.z);
		ImGui::Text("Pos: %.2f:%.2f:%.2f, Distance:%.2f", pos.x, pos.y, pos.z, distance);
		ImGui::Text("Yaw: %.2f Pitch: %.2f Roll: %.2f", yaw, pitch, _camera.roll());
		ImGui::Text("Pending: %i, meshes: %i, extracted: %i, uploaded: %i, visible: %i, octreesize: %i, octreeactive: %i, occluded: %i",
				stats.pending, stats.meshes, stats.extracted, stats.active, stats.visible, stats.octreeSize, stats.octreeActive, stats.occluded);
	}
	const bool current = isRelativeMouseMode();
	ImGui::Text("World mouse mode: %s", (current ? "true" : "false"));

	ImGui::InputVarFloat("Rotation Speed", _rotationSpeed);
	ImGui::CheckboxVar("Occlusion Query", cfg::OcclusionQuery);
	ImGui::CheckboxVar("Render Occlusion Queries", cfg::RenderOccluded);
	ImGui::CheckboxVar("Render AABB", cfg::RenderAABB);

	if (ImGui::CollapsingHeader("Debug")) {
		ImGui::Checkbox("Single position", &_singlePosExtraction);
		if (ImGui::Button("Use current position")) {
			_singleExtractionPoint = _camera.target();
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.2f);
		ImGui::InputInt3("Extract position", glm::value_ptr(_singleExtractionPoint), 0);
		if (ImGui::Button("Reset")) {
			_worldRenderer.reset();
			_worldRenderer.addEntity(_entity);
		}
		if (ImGui::Button("Extract")) {
			const glm::vec3 entPos(_singleExtractionPoint.x, voxel::MAX_TERRAIN_HEIGHT, _singleExtractionPoint.z);
			_entity->setPosition(entPos);
			_worldRenderer.extractMesh(_singleExtractionPoint);
		}
		ImGui::SameLine();
		if (ImGui::Button("Extract around position")) {
			voxelgenerator::Spiral o;
			const glm::ivec3 ms(_meshSize->intVal());
			for (int i = 0; i < 9; ++i) {
				glm::ivec3 meshPos = _singleExtractionPoint;
				meshPos.x += o.x() * ms.x;
				meshPos.z += o.z() * ms.z;
				_worldRenderer.extractMesh(meshPos);
				o.next();
			}
		}
	}

	if (ImGui::CollapsingHeader("Camera")) {
		if (ImGui::InputFloat("FOV", &_fieldOfView)) {
			_camera.setFieldOfView(glm::clamp(_fieldOfView, 1.0f, 360.0f));
		}
		if (ImGui::InputFloat("Distance", &_targetDistance)) {
			_camera.setTargetDistance(glm::clamp(_targetDistance, 1.0f, 200.0f));
		}
		if (ImGui::InputFloat3("Relative position", glm::value_ptr(_cameraPosition))) {
			_camera.setPosition(_camera.target() + _cameraPosition);
		}
	}

	if (ImGui::CollapsingHeader("Shadow")) {
		ImGui::CheckboxVar("Shadowmap render", cfg::ClientShadowMapShow);
		ImGui::CheckboxVar("Shadowmap cascades", cfg::ClientDebugShadowMapCascade);
		ImGui::CheckboxVar("Shadowmap debug", cfg::ClientDebugShadow);

		render::ShadowParameters& sp = _worldRenderer.shadow().parameters();
		ImGui::InputFloat("Shadow bias", &sp.shadowBias);
		ImGui::InputFloat("Shadow bias slope", &sp.shadowBiasSlope);
		ImGui::InputFloat("Shadow slice weight", &sp.sliceWeight);
	}

	ImGui::Checkbox("Line mode rendering", &_lineModeRendering);
	ImGui::Checkbox("Update World", &_updateWorld);

	bool temp = _renderTracing;
	if (ImGui::Checkbox("Toggle profiler", &temp)) {
		_renderTracing = toggleTrace();
	}
}

core::AppState MapView::onRunning() {
	core_trace_scoped(MapViewOnRunning);
	ScopedProfiler<ProfilerCPU> wt(_frameTimer);
	const core::AppState state = Super::onRunning();

	_movement.update(_deltaFrameMillis);

	const bool current = isRelativeMouseMode();
	if (current) {
		_camera.rotate(glm::vec3(_mouseRelativePos.y, _mouseRelativePos.x, 0.0f) * _rotationSpeed->floatVal());
		_entity->setOrientation(-1.0 * _camera.yaw());
	}
	_axis.render(_camera);
	_entity->update(_deltaFrameMillis);
	return state;
}

core::AppState MapView::onCleanup() {
	_stockDataProvider->shutdown();
	_animationCache->shutdown();
	_volumeCache->shutdown();
	_worldRenderer.shutdown();
	_worldTimer.shutdown();
	_axis.shutdown();
	_movement.shutdown();
	_entity = frontend::ClientEntityPtr();
	const core::AppState state = Super::onCleanup();
	_worldMgr->shutdown();
	return state;
}

void MapView::onWindowResize(int windowWidth, int windowHeight) {
	Super::onWindowResize(windowWidth, windowHeight);
	_camera.init(glm::ivec2(0), frameBufferDimension(), windowDimension());
}

bool MapView::onKeyPress(int32_t key, int16_t modifier) {
	if (key == SDLK_ESCAPE) {
		toggleRelativeMouseMode();
	}
	return Super::onKeyPress(key, modifier);
}

int main(int argc, char *argv[]) {
	const animation::AnimationCachePtr& animationCache = std::make_shared<animation::AnimationCache>();
	const core::EventBusPtr& eventBus = std::make_shared<core::EventBus>();
	const voxelformat::VolumeCachePtr& volumeCache = std::make_shared<voxelformat::VolumeCache>();
	const voxelworld::WorldMgrPtr& world = std::make_shared<voxelworld::WorldMgr>(volumeCache);
	const io::FilesystemPtr& filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr& timeProvider = std::make_shared<core::TimeProvider>();
	const metric::MetricPtr& metric = std::make_shared<metric::Metric>();
	const stock::StockDataProviderPtr& stockDataProvider = std::make_shared<stock::StockDataProvider>();
	MapView app(metric, animationCache, stockDataProvider, filesystem, eventBus, timeProvider, world, volumeCache);
	return app.startMainLoop(argc, argv);
}
