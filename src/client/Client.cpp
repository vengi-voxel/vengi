/**
 * @file
 */

#include "ClientMessages_generated.h"
#include "ServerMessages_generated.h"
#include "Client.h"
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
#include "network/SeedHandler.h"
#include "network/AuthFailedHandler.h"
#include "network/EntityRemoveHandler.h"
#include "network/EntitySpawnHandler.h"
#include "network/EntityUpdateHandler.h"
#include "network/UserSpawnHandler.h"
#include "voxel/MaterialColor.h"
#include "core/Rest.h"

Client::Client(const metric::MetricPtr& metric, const animation::AnimationCachePtr& animationCache,
		const stock::StockDataProviderPtr& stockDataProvider,
		const network::ClientNetworkPtr& network, const voxelworld::WorldMgrPtr& world,
		const network::ClientMessageSenderPtr& messageSender,
		const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider,
		const io::FilesystemPtr& filesystem, const voxelformat::VolumeCachePtr& volumeCache) :
		Super(metric, filesystem, eventBus, timeProvider), _animationCache(animationCache),
		_network(network), _worldMgr(world), _messageSender(messageSender),
		_worldRenderer(world), _waiting(this), _stockDataProvider(stockDataProvider), _volumeCache(volumeCache),
		_camera(world, _worldRenderer) {
	init(ORGANISATION, "client");
}

Client::~Client() {
}

void Client::sendMovement() {
	network::MoveDirection moveMask = network::MoveDirection::NONE;
	if (_movement.left()) {
		moveMask |= network::MoveDirection::MOVELEFT;
	} else if (_movement.right()) {
		moveMask |= network::MoveDirection::MOVERIGHT;
	}
	if (_movement.forward()) {
		moveMask |= network::MoveDirection::MOVEFORWARD;
	} else if (_movement.backward()) {
		moveMask |= network::MoveDirection::MOVEBACKWARD;
	}

	if (_movement.jump()) {
		moveMask |= network::MoveDirection::JUMP;
	}

	// TODO: orientation must match, too
	if (_lastMoveMask == moveMask) {
		return;
	}
	_lastMoveMask = moveMask;
	// TODO: we can't use the camera, as we are aiming for a freelook mode, where the players' angles might be different from the camera's
	const float pitch = 0.0f;
	const float yaw = 0.0f;
	_messageSender->sendClientMessage(_moveFbb, network::ClientMsgType::Move, CreateMove(_moveFbb, moveMask, pitch, yaw).Union());
}

void Client::onEvent(const network::DisconnectEvent& event) {
	_network->destroy();
	ui::turbobadger::Window* main = new frontend::LoginWindow(this);
	new frontend::DisconnectWindow(main);
}

void Client::onEvent(const network::NewConnectionEvent& event) {
	flatbuffers::FlatBufferBuilder fbb;
	const std::string& email = core::Var::getSafe(cfg::ClientEmail)->strVal();
	const std::string& password = core::Var::getSafe(cfg::ClientPassword)->strVal();
	Log::info("Trying to log into the server with %s", email.c_str());
	_messageSender->sendClientMessage(fbb, network::ClientMsgType::UserConnect,
			network::CreateUserConnect(fbb, fbb.CreateString(email), fbb.CreateString(core::pwhash(password, "TODO"))).Union());
}

void Client::onEvent(const voxelworld::WorldCreatedEvent& event) {
	Log::info("world created");
	new frontend::HudWindow(this, frameBufferDimension());
}

core::AppState Client::onConstruct() {
	core::AppState state = Super::onConstruct();

	_volumeCache->construct();
	_movement.construct();

	core::Var::get(cfg::ClientPort, SERVER_PORT);
	core::Var::get(cfg::ClientHost, SERVER_HOST);
	core::Var::get(cfg::ClientAutoLogin, "false");
	core::Var::get(cfg::ClientName, "noname");
	core::Var::get(cfg::ClientPassword, "");
	core::Var::get(cfg::HTTPBaseURL, BASE_URL);
	_rotationSpeed = core::Var::getSafe(cfg::ClientMouseRotationSpeed);
	_maxTargetDistance = core::Var::get(cfg::ClientCameraMaxTargetDistance, "20.0");
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
	regHandler(network::ServerMsgType::Seed, SeedHandler, _worldMgr, _eventBus);

	core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}

	video::enableDebug(video::DebugSeverity::Medium);

	if (!_network->init()) {
		return core::AppState::InitFailure;
	}

	if (!_movement.init()) {
		return core::AppState::InitFailure;
	}

	if (!_stockDataProvider->init(filesystem()->load("stock.lua"))) {
		Log::error("Failed to init stock data provider: %s", _stockDataProvider->error().c_str());
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
		return core::AppState::InitFailure;
	}

	if (!_worldMgr->init(filesystem()->load("worldparams.lua"), filesystem()->load("biomes.lua"))) {
		return core::AppState::InitFailure;
	}

	if (!_worldRenderer.init(glm::ivec2(0), frameBufferDimension())) {
		return core::AppState::InitFailure;
	}

	RestClient::init();

	_voxelFont.init("font.ttf", 14, 1, voxel::VoxelFont::MergeQuads, " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~€‚ƒ„…†‡ˆ‰Š‹ŒŽ‘’“”•–—˜™š›œžŸ¡¢£¤¥¦§¨©ª«¬®¯°±²³´µ¶·¸¹º»¼½¾¿ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþÿ");

	handleLogin();

	return state;
}

void Client::handleLogin() {
	const core::VarPtr& autoLoginVar = core::Var::getSafe(cfg::ClientAutoLogin);
	if (autoLoginVar->boolVal()) {
		const int port = core::Var::getSafe(cfg::ClientPort)->intVal();
		const std::string& host = core::Var::getSafe(cfg::ClientHost)->strVal();
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

	if (_player) {
		const video::Camera& camera = _camera.camera();
		_camera.update(_player->position(), _deltaFrameMillis);
		_movement.update(_deltaFrameSeconds, camera.yaw(), _player, [&] (const glm::vec3& pos, float maxWalkHeight) {
			return _worldMgr->findWalkableFloor(pos, maxWalkHeight);
		});
		_worldRenderer.extractMeshes(camera);
		_worldRenderer.update(camera, _deltaFrameMillis);
		_worldRenderer.renderWorld(camera);
	}
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
	_voxelFont.shutdown();
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
	Log::info("shutting down the volume cache");
	_volumeCache->shutdown();

	RestClient::disable();

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
	core::Var::visitBroadcast([] (const core::VarPtr& var) {
		Log::info("TODO: %s needs broadcast", var->name().c_str());
	});
	if (_network->isConnected()) {
		_camera.camera().rotate(glm::vec3(_mouseRelativePos.y, _mouseRelativePos.x, 0.0f) * _rotationSpeed->floatVal());
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

void Client::signup(const std::string& email, const std::string& password) {
	const core::rest::Response& r = core::rest::post("signup",
			core::json { { "email", email }, { "password", core::pwhash(password, "TODO") } });
	if (r.code != core::rest::StatusCode::OK) {
		Log::error("Failed to signup with %s (%i)", email.c_str(), r.code);
	}
}

void Client::lostPassword(const std::string& email) {
	const core::rest::Response& r = core::rest::post("lostpassword",
			core::json { { "email", email } });
	if (r.code != core::rest::StatusCode::OK) {
		Log::error("Failed to request the password reset for %s (%i)", email.c_str(), r.code);
	}
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

void Client::entityUpdate(frontend::ClientEntityId id, const glm::vec3& pos, float orientation, animation::Animation animation) {
	const frontend::ClientEntityPtr& entity = _worldRenderer.getEntity(id);
	if (!entity) {
		Log::warn("Could not get entity with id %li", id);
		return;
	}
	entity->setPosition(pos);
	entity->setOrientation(orientation);
	entity->setAnimation(animation);
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

bool Client::connect(uint16_t port, const std::string& hostname) {
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
	const voxelworld::WorldMgrPtr& world = std::make_shared<voxelworld::WorldMgr>(volumeCache);
	const core::TimeProviderPtr& timeProvider = std::make_shared<core::TimeProvider>();
	const io::FilesystemPtr& filesystem = std::make_shared<io::Filesystem>();
	const network::ProtocolHandlerRegistryPtr& protocolHandlerRegistry = std::make_shared<network::ProtocolHandlerRegistry>();
	const network::ClientNetworkPtr& network = std::make_shared<network::ClientNetwork>(protocolHandlerRegistry, eventBus);
	const network::ClientMessageSenderPtr& messageSender = std::make_shared<network::ClientMessageSender>(network);
	const metric::MetricPtr& metric = std::make_shared<metric::Metric>();
	const stock::StockDataProviderPtr& stockDataProvider = std::make_shared<stock::StockDataProvider>();
	Client app(metric, animationCache, stockDataProvider, network, world, messageSender, eventBus, timeProvider, filesystem, volumeCache);
	return app.startMainLoop(argc, argv);
}
