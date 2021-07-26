/**
 * @file
 */

#include "MapView.h"

#include "stock/ContainerData.h"
#include "video/Shader.h"
#include "video/Renderer.h"
#include "core/GLM.h"
#include "core/GameConfig.h"
#include "core/Color.h"
#include "metric/Metric.h"
#include "core/TimeProvider.h"
#include "core/EventBus.h"
#include "command/Command.h"
#include "voxel/Voxel.h"
#include "compute/Compute.h"
#include "io/Filesystem.h"
#include "ui/imgui/IMGUI.h"
#include "voxel/MaterialColor.h"
#include "voxelgenerator/Spiral.h"
#include "attrib/Attributes.h"
#include "attrib/ContainerProvider.h"
#include "audio/SoundManager.h"
#include "voxelworldrender/AssetVolumeCache.h"
#include <SDL.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>

MapView::MapView(const metric::MetricPtr& metric, const animation::AnimationCachePtr& animationCache,
		const stock::StockDataProviderPtr& stockDataProvider,
		const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus,
		const core::TimeProviderPtr& timeProvider, const voxelworld::WorldMgrPtr& worldMgr,
		const voxelworld::WorldPagerPtr& worldPager,
		const voxelformat::VolumeCachePtr& volumeCache,
		const voxelformat::MeshCachePtr& meshCache,
		const audio::SoundManagerPtr& soundManager,
		const voxelworldrender::AssetVolumeCachePtr& assetVolumeCache) :
		Super(metric, filesystem, eventBus, timeProvider),
		_animationCache(animationCache), _worldRenderer(assetVolumeCache), _worldMgr(worldMgr),
		_worldPager(worldPager), _movement(soundManager), _stockDataProvider(stockDataProvider),
		_volumeCache(volumeCache), _meshCache(meshCache), _camera(_worldRenderer),
		_soundManager(soundManager), _assetVolumeCache(assetVolumeCache) {
	init(ORGANISATION, "mapview");
}

MapView::~MapView() {
}

app::AppState MapView::onConstruct() {
	app::AppState state = Super::onConstruct();

	_rotationSpeed = core::Var::getSafe(cfg::ClientMouseRotationSpeed);

	_assetVolumeCache->construct();

	_movement.construct();
	_action.construct();
	_camera.construct();
	_meshCache->construct();

	command::Command::registerCommand("bird", [&] (const command::CmdArgs& args) {
		glm::vec3 pos = _entity->position();
		pos.y = core_min(voxel::MAX_HEIGHT, pos.y + 100.0f);
		_entity->setPosition(pos);
	});

	command::Command::registerCommand("+linemode", [&] (const command::CmdArgs& args) {
		if (args.empty()) {
			return;
		}
		_lineModeRendering = args[0] == "true";
	}).setHelp("Toggle line rendering mode");

	_meshSize = core::Var::get(cfg::VoxelMeshSize, "32", core::CV_READONLY);

	_soundManager->construct();
	_volumeCache->construct();
	_worldRenderer.construct();

	return state;
}

app::AppState MapView::onInit() {
	app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}

	compute::init();

	video::enableDebug(video::DebugSeverity::High);

	if (!_meshCache->init()) {
		Log::error("Failed to initialize mesh cache");
		return app::AppState::InitFailure;
	}

	if (!_depthBufferRenderer.init()) {
		Log::warn("Failed to init depth buffer renderer");
	}

	if (!_axis.init()) {
		Log::error("Failed to init axis");
		return app::AppState::InitFailure;
	}

	if (!_volumeCache->init()) {
		Log::error("Failed to init volumeCache");
		return app::AppState::InitFailure;
	}

	if (!_assetVolumeCache->init()) {
		Log::error("Failed to init asset volume cache");
		return app::AppState::InitFailure;
	}

	if (!_movement.init()) {
		Log::error("Failed to init movement");
		return app::AppState::InitFailure;
	}

	if (!_action.init()) {
		Log::error("Failed to init action component");
		return app::AppState::InitFailure;
	}

	if (!_stockDataProvider->init(filesystem()->load("stock.lua"))) {
		Log::error("Failed to init stock data provider: %s", _stockDataProvider->error().c_str());
		return app::AppState::InitFailure;
	}

	if (!_soundManager->init()) {
		Log::warn("Failed to initialize the sound manager");
	}

	if (!voxel::initDefaultMaterialColors()) {
		Log::error("Failed to initialize the palette data");
		return app::AppState::InitFailure;
	}

	if (!_animationCache->init()) {
		Log::error("Failed to init mesh cache");
		return app::AppState::InitFailure;
	}

	if (!_worldMgr->init()) {
		Log::error("Failed to init world mgr");
		return app::AppState::InitFailure;
	}

	if (!_floorResolver.init(_worldMgr)) {
		Log::error("Failed to init world mgr");
		return app::AppState::InitFailure;
	}

	if (!_worldPager->init(_worldMgr->volumeData(), filesystem()->load("worldparams.lua"), filesystem()->load("biomes.lua"))) {
		Log::error("Failed to init world pager");
		return app::AppState::InitFailure;
	}

	_worldMgr->setSeed(1);
	_worldPager->setSeed(1);

	if (!_worldRenderer.init(_worldMgr->volumeData(), glm::ivec2(0), _frameBufferDimension)) {
		Log::error("Failed to init world renderer");
		return app::AppState::InitFailure;
	}

	_camera.init(glm::ivec2(0), frameBufferDimension(), windowDimension());

	const glm::vec3& pos = spawnPos();
	Log::info("Spawn entity at %s", glm::to_string(pos).c_str());

	if (!changeEntityType(pos, _entityType)) {
		return app::AppState::InitFailure;
	}

	core::setBindingContext(core::BindingContext::World);

	_soundManager->playMusic("ambience", true);

	return state;
}

bool MapView::changeEntityType(const glm::vec3& pos, const network::EntityType entityType) {
	const frontend::ClientEntityId entityId = (frontend::ClientEntityId)1;
	_entity = core::make_shared<frontend::ClientEntity>(_stockDataProvider, _animationCache, entityId, entityType, pos, 0.0f);
	attrib::ContainerProvider containerProvider;
	const core::String& attribLua = filesystem()->load("attributes.lua");
	if (!containerProvider.init(attribLua)) {
		Log::error("Failed to init attributes: %s", containerProvider.error().c_str());
		return false;
	}
	const attrib::ContainerPtr& attribContainer = containerProvider.container("GENERIC");
	if (!attribContainer) {
		Log::error("Failed to load attributes for attribute 'entity'");
		return false;
	}
	attrib::Attributes attributes;
	attributes.add(attribContainer);
	attributes.update(0l);
	const double speed = attributes.max(attrib::Type::SPEED);
	_entity->attrib().setCurrent(attrib::Type::SPEED, speed);
	_worldRenderer.entityMgr().removeEntity(_entity->id());
	if (!_worldRenderer.entityMgr().addEntity(_entity)) {
		Log::error("Failed to create entity");
		return false;
	}
	stock::Stock& stock = _entity->stock();
	stock::Inventory& inv = stock.inventory();
	const stock::ContainerData* containerData = _stockDataProvider->containerData("tool");
	if (containerData == nullptr) {
		Log::error("Could not get container for items");
		return false;
	}
	const stock::ItemData* itemData = _stockDataProvider->itemData(1);
	if (itemData == nullptr) {
		Log::error("Failed to get item with id 1");
		return false;
	}
	const stock::ItemPtr& item = _stockDataProvider->createItem(itemData->id());
	if (!inv.add(containerData->id, item, 0, 0)) {
		Log::error("Failed to add item to inventory");
		return false;
	}
	return true;
}

glm::vec3 MapView::spawnPos() const {
	const int radius = 30;
	for (int x = -radius; x < radius; ++x) {
		for (int z = -radius; z < radius; ++z) {
			const voxelutil::FloorTraceResult& trace = _worldMgr->findWalkableFloor(glm::ivec3(x, voxel::MAX_HEIGHT / 2, z));
			if (trace.heightLevel <= voxel::MAX_WATER_HEIGHT) {
				continue;
			}
			return glm::vec3(x, (float)trace.heightLevel, z);
		}
	}
	return glm::vec3(0.0f, 1.0f, 0.0f);
}

void MapView::beforeUI() {
	Super::beforeUI();

	const video::Camera& camera = _camera.camera();
	_movement.update(_deltaFrameSeconds, camera.horizontalYaw(), _entity, [&] (const glm::ivec3& pos, int maxWalkHeight) {
		return _floorResolver.findWalkableFloor(pos, maxWalkHeight);
	});
	_action.update(nowSeconds(), _entity);
	const double speed = _entity->attrib().current(attrib::Type::SPEED);
	_camera.update(_entity->position(), _nowSeconds, _deltaFrameSeconds, speed);

	if (_updateWorld) {
		core_trace_scoped(UpdateWorld);
		if (!_singlePosExtraction) {
			_worldRenderer.extractMeshes(camera);
		}
		_worldRenderer.update(camera, _deltaFrameSeconds);
	}
	if (_lineModeRendering) {
		video::polygonMode(video::Face::FrontAndBack, video::PolygonMode::WireFrame);
	}
	_drawCallsWorld = _worldRenderer.renderWorld(camera);
	if (_lineModeRendering) {
		video::polygonMode(video::Face::FrontAndBack, video::PolygonMode::Solid);
	}
}

void MapView::onRenderUI() {
	if (ImGui::CollapsingHeader("Stats")) {
		const video::Camera& camera = _camera.camera();
		const glm::vec3& pos = camera.position();
		const glm::vec3& targetpos = camera.target();
		const float distance = camera.targetDistance();
		const float pitch = camera.pitch();
		const float yaw = camera.horizontalYaw();
		ImGui::Text("Fps: %f", fps());
		ImGui::Text("Drawcalls: %i", _drawCallsWorld);
		ImGui::Text("Target Pos: %.2f:%.2f:%.2f ", targetpos.x, targetpos.y, targetpos.z);
		ImGui::Text("Pos: %.2f:%.2f:%.2f, Distance:%.2f", pos.x, pos.y, pos.z, distance);
		ImGui::Text("Yaw: %.2f Pitch: %.2f Roll: %.2f", yaw, pitch, camera.roll());
	}
	const bool current = isRelativeMouseMode();
	ImGui::Text("World mouse mode: %s", (current ? "true" : "false"));

	ImGui::InputDouble("Time scale", &_timeScaleFactor, 0.1, 1.0);
	ImGui::InputDouble("World time", &_worldTime);

	_worldTime += _deltaFrameSeconds * _timeScaleFactor;
	_worldRenderer.setSeconds(_worldTime);

	ImGui::InputVarFloat("Rotation Speed", _rotationSpeed);

	if (ImGui::BeginCombo("Entity", network::EnumNameEntityType(_entityType), ImGuiComboFlags_None)) {
		for (int i = ((int)network::EntityType::BEGIN_ANIMAL) + 1; i < (int)network::EntityType::MAX_CHARACTERS; ++i) {
			if (i >= (int)network::EntityType::MAX_ANIMAL && i <= (int)network::EntityType::BEGIN_CHARACTERS) {
				continue;
			}
			const network::EntityType type = (network::EntityType)i;
			bool selected = type == _entityType;
			if (ImGui::Selectable(network::EnumNameEntityType(type), selected)) {
				if (!changeEntityType(_entity->position(), type)) {
					changeEntityType(_entity->position(), _entityType);
				} else {
					_entityType = type;
					selected = true;
				}
			}
			if (selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	if (ImGui::CollapsingHeader("Textures/Buffers")) {
		static bool renderColorMap = false;
		ImGui::Checkbox("Colormap render", &renderColorMap);
		if (renderColorMap) {
			static glm::ivec2 colorMapSize(256, 256);
			ImGui::InputVec2("size", colorMapSize);
			ImGui::Image(_worldRenderer.colorTexture().handle(), colorMapSize);
		}
		static glm::ivec2 frameBufferSize(256, 256);
		ImGui::InputVec2("size", frameBufferSize);
		ImGui::Text("Framebuffer");
		ImGui::Image(_worldRenderer.frameBuffer().texture()->handle(), frameBufferSize);
		ImGui::Text("Reflection");
		ImGui::Image(_worldRenderer.reflectionBuffer().texture()->handle(), frameBufferSize);
		ImGui::Text("Refraction");
		ImGui::Image(_worldRenderer.refractionBuffer().texture()->handle(), frameBufferSize);
		const video::Camera& camera = _camera.camera();
		const video::FrameBufferAttachment attachment = video::FrameBufferAttachment::Color0;
		_depthBufferRenderer.renderDepthBufferToTexture(camera, _worldRenderer.entitiesBuffer(), attachment);
		ImGui::Image(_depthBufferRenderer.renderToTextureFbo().texture(attachment)->handle(), frameBufferSize);
	}

	if (ImGui::CollapsingHeader("Mesh extraction")) {
		ImGui::Checkbox("Single position", &_singlePosExtraction);
		if (ImGui::Button("Use current position")) {
			_singleExtractionPoint = _camera.camera().target();
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.2f);
		ImGui::InputInt3("Extract position", glm::value_ptr(_singleExtractionPoint), 0);
		if (ImGui::Button("Reset")) {
			_worldRenderer.reset();
			_worldRenderer.entityMgr().addEntity(_entity);
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
		const video::Camera& camera = _camera.camera();
		float fieldOfView = camera.fieldOfView();
		if (ImGui::InputFloat("FOV", &fieldOfView)) {
			_camera.setFieldOfView(glm::clamp(fieldOfView, 1.0f, 360.0f));
		}
		const float targetDistance = camera.targetDistance();
		ImGui::Text("Distance: %.0f", targetDistance);
	}

	if (ImGui::CollapsingHeader("Shadow")) {
		render::ShadowParameters& sp = _worldRenderer.shadow().parameters();
		static bool renderShadowMap = false;
		ImGui::Checkbox("Shadowmap render", &renderShadowMap);
		if (renderShadowMap) {
			const video::Camera& camera = _camera.camera();
			static glm::ivec2 frameBufferSize(256, 256);
			ImGui::InputVec2("size", frameBufferSize);
			int index = (int)video::FrameBufferAttachment::Color1;
			for (int i = 0; i < sp.maxDepthBuffers; ++i) {
				const video::FrameBufferAttachment attachment = (video::FrameBufferAttachment)(index + i);
				_depthBufferRenderer.renderShadowMapToTexture(camera, _worldRenderer.shadow().depthBuffer(), i, attachment);
				ImGui::Text("Cascade %i", i + 1);
				ImGui::Image(_depthBufferRenderer.renderToTextureFbo().texture(attachment)->handle(), frameBufferSize);
			}
		}
		ImGui::CheckboxVar("Shadowmap cascades", cfg::ClientDebugShadowMapCascade);
		ImGui::CheckboxVar("Shadowmap debug", cfg::ClientDebugShadow);

		ImGui::InputFloat("Shadow bias", &sp.shadowBias);
		ImGui::InputFloat("Shadow bias slope", &sp.shadowBiasSlope);
		ImGui::InputFloat("Shadow slice weight", &sp.sliceWeight);
	}

	ImGui::Checkbox("Line mode rendering", &_lineModeRendering);
	ImGui::Checkbox("Update World", &_updateWorld);
}

app::AppState MapView::onRunning() {
	core_trace_scoped(MapViewOnRunning);
	const app::AppState state = Super::onRunning();

	const bool current = isRelativeMouseMode();
	if (current) {
		const float pitch = _mouseRelativePos.y;
		const float turn = _mouseRelativePos.x;
		_camera.rotate(pitch, turn, _rotationSpeed->floatVal());
	}

	_soundManager->setListenerPosition(_camera.camera().position());
	_soundManager->update();
	_axis.render(_camera.camera());
	compute::finish();
	return state;
}

app::AppState MapView::onCleanup() {
	_soundManager->shutdown();
	_stockDataProvider->shutdown();
	_animationCache->shutdown();
	_worldRenderer.shutdown();
	_assetVolumeCache->shutdown();
	_volumeCache->shutdown();
	_depthBufferRenderer.shutdown();
	_axis.shutdown();
	_movement.shutdown();
	_action.shutdown();
	_camera.shutdown();
	_entity = frontend::ClientEntityPtr();
	const app::AppState state = Super::onCleanup();
	_worldPager->shutdown();
	_worldMgr->shutdown();
	_floorResolver.shutdown();
	_meshCache->shutdown();
	compute::shutdown();
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
	const voxelformat::MeshCachePtr& meshCache = std::make_shared<voxelformat::MeshCache>();
	const animation::AnimationCachePtr& animationCache = core::make_shared<animation::AnimationCache>(meshCache);
	const core::EventBusPtr& eventBus = std::make_shared<core::EventBus>();
	const voxelformat::VolumeCachePtr& volumeCache = std::make_shared<voxelformat::VolumeCache>();
	const voxelworld::ChunkPersisterPtr& chunkPersister = std::make_shared<voxelworld::ChunkPersister>();
	const voxelworld::WorldPagerPtr& worldPager = core::make_shared<voxelworld::WorldPager>(volumeCache, chunkPersister);
	const voxelworld::WorldMgrPtr& worldMgr = std::make_shared<voxelworld::WorldMgr>(worldPager);
	const io::FilesystemPtr& filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr& timeProvider = std::make_shared<core::TimeProvider>();
	const metric::MetricPtr& metric = std::make_shared<metric::Metric>();
	const stock::StockDataProviderPtr& stockDataProvider = std::make_shared<stock::StockDataProvider>();
	const audio::SoundManagerPtr& soundMgr = core::make_shared<audio::SoundManager>(filesystem);
	const voxelworldrender::AssetVolumeCachePtr& assetVolumeCache = core::make_shared<voxelworldrender::AssetVolumeCache>(volumeCache);
	MapView app(metric, animationCache, stockDataProvider, filesystem, eventBus, timeProvider,
			worldMgr, worldPager, volumeCache, meshCache, soundMgr, assetVolumeCache);
	return app.startMainLoop(argc, argv);
}
