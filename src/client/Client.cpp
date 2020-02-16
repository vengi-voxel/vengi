/**
 * @file
 */

#include "ClientMessages_generated.h"
#include "ServerMessages_generated.h"
#include "Client.h"
#include "voxel/ClientPager.h"
#include "ui/LoginWindow.h"
#include "ui/DisconnectWindow.h"
#include "ui/AuthFailedWindow.h"
#include "ui/HudWindow.h"
#include "ui/turbobadger/FontUtil.h"
#include "ui/turbobadger/Window.h"
#include "core/command/Command.h"
#include "core/GLM.h"
#include "core/io/Filesystem.h"
#include "core/Color.h"
#include "core/Password.h"
#include "network/IMsgProtocolHandler.h"
#include "network/AttribUpdateHandler.h"
#include "network/AuthFailedHandler.h"
#include "network/EntityRemoveHandler.h"
#include "network/EntitySpawnHandler.h"
#include "network/EntityUpdateHandler.h"
#include "network/UserSpawnHandler.h"
#include "network/UserInfoHandler.h"
#include "network/VarUpdateHandler.h"
#include "voxel/MaterialColor.h"
#include "core/metric/Metric.h"
#include "core/TimeProvider.h"
#include <SDL.h>

Client::Client(const metric::MetricPtr& metric, const animation::AnimationCachePtr& animationCache,
		const stock::StockDataProviderPtr& stockDataProvider,
		const network::ClientNetworkPtr& network, const voxelworld::WorldMgrPtr& world,
		const client::ClientPagerPtr& worldPager,
		const network::ClientMessageSenderPtr& messageSender,
		const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider,
		const io::FilesystemPtr& filesystem, const voxelformat::VolumeCachePtr& volumeCache) :
		Super(metric, filesystem, eventBus, timeProvider), _animationCache(animationCache),
		_network(network), _worldMgr(world), _clientPager(worldPager), _messageSender(messageSender),
		_waiting(this), _stockDataProvider(stockDataProvider), _volumeCache(volumeCache),
		_camera(world, _worldRenderer) {
	init(ORGANISATION, "client");
}

Client::~Client() {
}

void Client::sendMovement() {
	const network::MoveDirection moveMask = _movement.moveMask();

	// TODO: we can't use the camera, as we are aiming for a freelook mode, where the players' angles might be different from the camera's
	const video::Camera& camera = _camera.camera();
	const float pitch = camera.pitch();
	const float yaw = camera.yaw();
	glm::vec2 moveAngles(pitch, yaw);

	if (_lastMoveMask != moveMask || !glm::all(glm::epsilonEqual(moveAngles, _lastMoveAngles, 0.0001f))) {
		_lastMoveMask = moveMask;
		_lastMoveAngles = moveAngles;
		_messageSender->sendClientMessage(_moveFbb, network::ClientMsgType::Move, CreateMove(_moveFbb, moveMask, moveAngles.x, moveAngles.y).Union());
	}
}

void Client::onEvent(const network::DisconnectEvent& event) {
	_network->destroy();
	ui::turbobadger::Window* main = new frontend::LoginWindow(this);
	new frontend::DisconnectWindow(main);
}

void Client::onEvent(const network::NewConnectionEvent& event) {
	flatbuffers::FlatBufferBuilder fbb;
	const core::String& email = core::Var::getSafe(cfg::ClientEmail)->strVal();
	const core::String& password = core::Var::getSafe(cfg::ClientPassword)->strVal();
	Log::info("Trying to log into the server with %s", email.c_str());
	const core::String& pwhash = core::pwhash(password, "TODO");
	_messageSender->sendClientMessage(fbb, network::ClientMsgType::UserConnect,
			network::CreateUserConnect(fbb, fbb.CreateString(email.c_str(), email.size()),
			fbb.CreateString(pwhash.c_str(), pwhash.size())).Union());
}

void Client::onEvent(const voxelworld::WorldCreatedEvent& event) {
	Log::info("world created");
	new frontend::HudWindow(this, frameBufferDimension());
}

core::AppState Client::onConstruct() {
	core::AppState state = Super::onConstruct();

	_volumeCache->construct();
	_movement.construct();
	_camera.construct();

	core::Var::get(cfg::ClientPort, SERVER_PORT);
	core::Var::get(cfg::ClientHost, SERVER_HOST);
	core::Var::get(cfg::ClientAutoLogin, "false");
	core::Var::get(cfg::ClientName, "noname", core::CV_BROADCAST);
	core::Var::get(cfg::ClientPassword, "");
	_chunkUrl = core::Var::get(cfg::ServerChunkBaseUrl, "");
	_seed = core::Var::get(cfg::ServerSeed, "");
	core::Var::get(cfg::HTTPBaseURL, BASE_URL);
	_rotationSpeed = core::Var::getSafe(cfg::ClientMouseRotationSpeed);
	core::Var::get(cfg::VoxelMeshSize, "16", core::CV_READONLY);
	_worldRenderer.construct();

	return state;
}

#define regHandler(type, handler, ...) \
	r->registerHandler(network::EnumNameServerMsgType(type), std::make_shared<handler>(__VA_ARGS__));

core::AppState Client::onInit() {
	eventBus()->subscribe<network::NewConnectionEvent>(*this);
	eventBus()->subscribe<network::DisconnectEvent>(*this);
	eventBus()->subscribe<voxelworld::WorldCreatedEvent>(*this);

	const network::ProtocolHandlerRegistryPtr& r = _network->registry();
	regHandler(network::ServerMsgType::AttribUpdate, AttribUpdateHandler);
	regHandler(network::ServerMsgType::EntitySpawn, EntitySpawnHandler);
	regHandler(network::ServerMsgType::EntityRemove, EntityRemoveHandler);
	regHandler(network::ServerMsgType::EntityUpdate, EntityUpdateHandler);
	regHandler(network::ServerMsgType::UserSpawn, UserSpawnHandler);
	regHandler(network::ServerMsgType::AuthFailed, AuthFailedHandler);
	regHandler(network::ServerMsgType::VarUpdate, VarUpdateHandler);
	regHandler(network::ServerMsgType::UserInfo, UserInfoHandler);

	core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}

	video::enableDebug(video::DebugSeverity::Medium);

	if (!_network->init()) {
		Log::error("Failed to initialize network layer");
		return core::AppState::InitFailure;
	}

	if (!_movement.init()) {
		Log::error("Failed to initialize movement controller");
		return core::AppState::InitFailure;
	}

	if (!_stockDataProvider->init(filesystem()->load("stock.lua"))) {
		Log::error("Failed to initialize stock data provider: %s", _stockDataProvider->error().c_str());
		return core::AppState::InitFailure;
	}

	_camera.init(glm::ivec2(0), frameBufferDimension(), windowDimension());
	_waiting.init();

	if (!_animationCache->init()) {
		Log::error("Failed to initialize character cache");
		return core::AppState::InitFailure;
	}

	if (!voxel::initDefaultMaterialColors()) {
		Log::error("Failed to initialize the palette data");
		return core::AppState::InitFailure;
	}

	if (!_volumeCache->init()) {
		Log::error("Failed to initialize volume cache");
		return core::AppState::InitFailure;
	}

	if (!_worldMgr->init()) {
		Log::error("Failed to initialize world manager");
		return core::AppState::InitFailure;
	}

	if (!_clientPager->init(_chunkUrl->strVal())) {
		Log::error("Failed to initialize client pager");
		return core::AppState::InitFailure;
	}

	if (!_worldRenderer.init(_worldMgr->volumeData(), glm::ivec2(0), frameBufferDimension())) {
		Log::error("Failed to initialize world renderer");
		return core::AppState::InitFailure;
	}

	handleLogin();

	return state;
}

void Client::handleLogin() {
	const core::VarPtr& autoLoginVar = core::Var::getSafe(cfg::ClientAutoLogin);
	if (autoLoginVar->boolVal()) {
		const int port = core::Var::getSafe(cfg::ClientPort)->intVal();
		const core::String& host = core::Var::getSafe(cfg::ClientHost)->strVal();
		if (!connect(port, host)) {
			autoLoginVar->setVal(false);
		}
	}

	if (!autoLoginVar->boolVal()) {
		new frontend::LoginWindow(this);
	}
}

void Client::beforeUI() {
	Super::beforeUI();

	if (_chunkUrl->isDirty()) {
		_clientPager->init(_chunkUrl->strVal());
		_chunkUrl->markClean();
	}
	if (_seed->isDirty()) {
		_seed->markClean();
		const unsigned int seed = _seed->intVal();
		Log::info("Initialize for seed %u", seed);
		_clientPager->setSeed(seed);
		_eventBus->publish(voxelworld::WorldCreatedEvent());
	}
	if (_player) {
		const video::Camera& camera = _camera.camera();
		_movement.update(_deltaFrameSeconds, camera.horizontalYaw(), _player, [&] (const glm::vec3& pos, float maxWalkHeight) {
			return _worldMgr->findWalkableFloor(pos, maxWalkHeight);
		});
		_camera.update(_player->position(), _deltaFrameMillis, _now);
		_worldRenderer.extractMeshes(camera);
		_worldRenderer.update(camera, _deltaFrameMillis);
		_worldRenderer.renderWorld(camera);
	}

	sendVars();
}

void Client::sendVars() const {
	std::vector<core::VarPtr> vars;
	core::Var::visitDirtyBroadcast([&vars] (const core::VarPtr& var) {
		vars.push_back(var);
	});
	if (vars.empty()) {
		return;
	}
	static flatbuffers::FlatBufferBuilder fbb;
	auto fbbVars = fbb.CreateVector<flatbuffers::Offset<network::Var>>(vars.size(),
		[&] (size_t i) {
			const core::String& sname = vars[i]->name();
			const core::String& svalue = vars[i]->strVal();
			auto name = fbb.CreateString(sname.c_str(), sname.size());
			auto value = fbb.CreateString(svalue.c_str(), svalue.size());
			return network::CreateVar(fbb, name, value);
		});
	_messageSender->sendClientMessage(fbb, network::ClientMsgType::VarUpdate,
			network::CreateVarUpdate(fbb, fbbVars).Union());
}

void Client::afterRootWidget() {
	if (_network->isConnecting()) {
		_waiting.render();
	}
	Super::afterRootWidget();
}

core::AppState Client::onCleanup() {
	Log::info("shutting down the client");
	eventBus()->unsubscribe<network::NewConnectionEvent>(*this);
	eventBus()->unsubscribe<network::DisconnectEvent>(*this);
	eventBus()->unsubscribe<voxelworld::WorldCreatedEvent>(*this);

	Log::info("disconnect");
	disconnect();

	Log::info("shutting down the client components");
	_stockDataProvider->shutdown();
	Log::info("shutting down the character cache");
	_animationCache->shutdown();
	Log::info("shutting down the world renderer");
	_worldRenderer.shutdown();
	core::AppState state = Super::onCleanup();
	Log::info("shutting down the world");
	_worldMgr->shutdown();
	_player = frontend::ClientEntityPtr();
	Log::info("shutting down the network");
	_network->shutdown();
	_waiting.shutdown();
	_movement.shutdown();
	_camera.shutdown();
	Log::info("shutting down the volume cache");
	_volumeCache->shutdown();

	Log::info("everything was shut down");

	return state;
}

bool Client::onKeyPress(int32_t key, int16_t modifier) {
	if (Super::onKeyPress(key, modifier)) {
		return true;
	}

	if (key == SDLK_ESCAPE) {
		if (_network->isConnecting() || _network->isConnected()) {
			disconnect();
		}
	}

	return false;
}

core::AppState Client::onRunning() {
	_waiting.update(_deltaFrameMillis);
	const core::AppState state = Super::onRunning();
	if (_network->isConnected()) {
		const float pitch = _mouseRelativePos.y;
		const float turn = _mouseRelativePos.x;
		_camera.rotate(pitch, turn, _rotationSpeed->floatVal());
	}
	sendMovement();
	if (state == core::AppState::Running) {
		_network->update();
	}

	return state;
}

void Client::onWindowResize(int windowWidth, int windowHeight) {
	Super::onWindowResize(windowWidth, windowHeight);
	_camera.init(glm::ivec2(0), frameBufferDimension(), windowDimension());
}

void Client::signup(const core::String& email, const core::String& password) {
}

void Client::lostPassword(const core::String& email) {
}

void Client::authFailed() {
	core::Var::getSafe(cfg::ClientAutoLogin)->setVal(false);
	// TODO: stack (push/pop in UIApp) window support
	ui::turbobadger::Window* main = new frontend::LoginWindow(this);
	new frontend::AuthFailedWindow(main);
}

void Client::disconnect() {
	_player = frontend::ClientEntityPtr();
	flatbuffers::FlatBufferBuilder fbb;
	_messageSender->sendClientMessage(fbb, network::ClientMsgType::UserDisconnect, network::CreateUserDisconnect(fbb).Union());
	_network->disconnect();
}

void Client::entitySpawn(frontend::ClientEntityId id, network::EntityType type, float orientation, const glm::vec3& pos, animation::Animation animation) {
	Log::info("Entity %li spawned at pos %f:%f:%f (type %i)", id, pos.x, pos.y, pos.z, (int)type);
	const frontend::ClientEntityPtr& entity = std::make_shared<frontend::ClientEntity>(_stockDataProvider, _animationCache, id, type, pos, orientation);
	entity->setAnimation(animation);
	_worldRenderer.addEntity(entity);
}

void Client::entityRemove(frontend::ClientEntityId id) {
	_worldRenderer.removeEntity(id);
}

void Client::spawn(frontend::ClientEntityId id, const char *name, const glm::vec3& pos, float orientation) {
	Log::info("User %li (%s) logged in at pos %f:%f:%f with orientation: %f", id, name, pos.x, pos.y, pos.z, orientation);
	_camera.setTarget(pos);

	// TODO:get rid of this
	//_camera.camera().setTargetDistance(_maxTargetDistance->floatVal());
	//_camera.camera().setPosition(pos + _cameraPosition);

	const network::EntityType type = network::EntityType::PLAYER;
	_player = std::make_shared<frontend::ClientEntity>(_stockDataProvider, _animationCache, id, type, pos, orientation);
	_worldRenderer.addEntity(_player);
	_worldRenderer.extractMeshes(_camera.camera());

	flatbuffers::FlatBufferBuilder fbb;
	_messageSender->sendClientMessage(fbb, network::ClientMsgType::UserConnected,
			network::CreateUserConnected(fbb).Union());
}

bool Client::connect(uint16_t port, const core::String& hostname) {
	if (hostname.empty()) {
		Log::error("No hostname given");
		return false;
	}
	ENetPeer* peer = _network->connect(port, hostname);
	if (peer == nullptr) {
		Log::error("Failed to connect to server %s:%i", hostname.c_str(), port);
		return false;
	}

	peer->data = this;
	Log::info("Connecting to server %s:%i", hostname.c_str(), port);
	_waiting.setTextId("stateconnecting");
	return true;
}

int main(int argc, char *argv[]) {
	const animation::AnimationCachePtr& animationCache = std::make_shared<animation::AnimationCache>();
	const core::EventBusPtr& eventBus = std::make_shared<core::EventBus>();
	const voxelformat::VolumeCachePtr& volumeCache = std::make_shared<voxelformat::VolumeCache>();
	const core::TimeProviderPtr& timeProvider = std::make_shared<core::TimeProvider>();
	const io::FilesystemPtr& filesystem = std::make_shared<io::Filesystem>();
	const network::ProtocolHandlerRegistryPtr& protocolHandlerRegistry = std::make_shared<network::ProtocolHandlerRegistry>();
	const network::ClientNetworkPtr& network = std::make_shared<network::ClientNetwork>(protocolHandlerRegistry, eventBus);
	const network::ClientMessageSenderPtr& messageSender = std::make_shared<network::ClientMessageSender>(network);
	const client::ClientPagerPtr& pager = std::make_shared<client::ClientPager>();
	const voxelworld::WorldMgrPtr& world = std::make_shared<voxelworld::WorldMgr>(pager);
	const metric::MetricPtr& metric = std::make_shared<metric::Metric>();
	const stock::StockDataProviderPtr& stockDataProvider = std::make_shared<stock::StockDataProvider>();
	Client app(metric, animationCache, stockDataProvider, network, world, pager, messageSender, eventBus, timeProvider, filesystem, volumeCache);
	return app.startMainLoop(argc, argv);
}
