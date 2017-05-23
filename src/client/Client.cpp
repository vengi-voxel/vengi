/**
 * @file
 */

#include "Client.h"
#include "ClientMessages_generated.h"
#include "ServerMessages_generated.h"
#include "ui/LoginWindow.h"
#include "ui/DisconnectWindow.h"
#include "ui/AuthFailedWindow.h"
#include "ui/HudWindow.h"
#include "ui/FontUtil.h"
#include "ui/Window.h"
#include "core/command/Command.h"
#include "core/GLM.h"
#include "io/Filesystem.h"
#include "core/Color.h"
#include "network/Network.h"
#include "network/MessageSender.h"
#include "network/IMsgProtocolHandler.h"
#include "ServerMessages_generated.h"
#include "network/AttribUpdateHandler.h"
#include "network/SeedHandler.h"
#include "network/AuthFailedHandler.h"
#include "network/EntityRemoveHandler.h"
#include "network/EntitySpawnHandler.h"
#include "network/EntityUpdateHandler.h"
#include "network/UserSpawnHandler.h"
#include "voxel/MaterialColor.h"

#include <restclient-cpp/restclient.h>
#include <restclient-cpp/connection.h>

#define registerMoveCmd(name, flag) \
	core::Command::registerCommand(name, [&] (const core::CmdArgs& args) { \
		if (args.empty()) { \
			return; \
		} \
		if (args[0] == "true") \
			_moveMask |= network::MoveDirection::flag; \
		else \
			_moveMask &= ~network::MoveDirection::flag; \
	}).setHelp("Character movement");

Client::Client(const video::MeshPoolPtr& meshPool, const network::NetworkPtr& network, const voxel::WorldPtr& world, const network::MessageSenderPtr& messageSender,
		const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, const io::FilesystemPtr& filesystem) :
		Super(filesystem, eventBus, timeProvider, 17816), _camera(), _meshPool(meshPool), _network(network), _world(world), _messageSender(messageSender),
		_worldRenderer(world), _waiting(this) {
	_world->setClientData(true);
	init(ORGANISATION, "client");
}

Client::~Client() {
}

void Client::sendMovement() {
	if (_peer == nullptr) {
		return;
	}

	if (_now - _lastMovement <= 100L) {
		return;
	}

	if (_lastMoveMask == _moveMask) {
		return;
	}
	_lastMovement = _now;
	_lastMoveMask = _moveMask;
	// TODO: we can't use the camera, as we are aiming for a freelook mode, where the players' angles might be different from the camera's
	const float pitch = 0.0f;
	const float yaw = 0.0f;
	_messageSender->sendClientMessage(_peer, _moveFbb, network::ClientMsgType::Move, CreateMove(_moveFbb, _moveMask, pitch, yaw).Union());
}

void Client::onMouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) {
	Super::onMouseMotion(x, y, relX, relY);
	_camera.rotate(glm::vec3(relY, relX, 0.0f) * _rotationSpeed->floatVal());
}

void Client::onEvent(const network::DisconnectEvent& event) {
	removeState(CLIENT_CONNECTING);
	ui::Window* main = new frontend::LoginWindow(this);
	new frontend::DisconnectWindow(main);
}

void Client::onEvent(const network::NewConnectionEvent& event) {
	flatbuffers::FlatBufferBuilder fbb;
	const std::string& email = core::Var::getSafe(cfg::ClientEmail)->strVal();
	const std::string& password = core::Var::getSafe(cfg::ClientPassword)->strVal();
	Log::info("Trying to log into the server with %s", email.c_str());
	_messageSender->sendClientMessage(_peer, fbb, network::ClientMsgType::UserConnect,
			network::CreateUserConnect(fbb, fbb.CreateString(email), fbb.CreateString(password)).Union());
}

void Client::onEvent(const voxel::WorldCreatedEvent& event) {
	Log::info("world created");
	new frontend::HudWindow(this, _dimension);
}

core::AppState Client::onConstruct() {
	core::AppState state = Super::onConstruct();

	registerMoveCmd("+move_right", MOVERIGHT);
	registerMoveCmd("+move_left", MOVELEFT);
	registerMoveCmd("+move_forward", MOVEFORWARD);
	registerMoveCmd("+move_backward", MOVEBACKWARD);

	core::Var::get(cfg::ClientPort, SERVER_PORT);
	core::Var::get(cfg::ClientHost, SERVER_HOST);
	core::Var::get(cfg::ClientAutoLogin, "false");
	core::Var::get(cfg::ClientName, "noname");
	core::Var::get(cfg::ClientPassword, "");
	core::Var::get(cfg::HTTPBaseURL, "https://localhost/");
	_rotationSpeed = core::Var::getSafe(cfg::ClientMouseRotationSpeed);
	_maxTargetDistance = core::Var::get(cfg::ClientCameraMaxTargetDistance, "250.0");
	core::Var::get(cfg::VoxelMeshSize, "16", core::CV_READONLY);
	_worldRenderer.onConstruct();

	return state;
}

core::AppState Client::onInit() {
	eventBus()->subscribe<network::NewConnectionEvent>(*this);
	eventBus()->subscribe<network::DisconnectEvent>(*this);
	eventBus()->subscribe<voxel::WorldCreatedEvent>(*this);

	const network::ProtocolHandlerRegistryPtr& r = _network->registry();
	r->registerHandler(network::EnumNameServerMsgType(network::ServerMsgType::AttribUpdate), std::make_shared<AttribUpdateHandler>());
	r->registerHandler(network::EnumNameServerMsgType(network::ServerMsgType::EntitySpawn), std::make_shared<EntitySpawnHandler>());
	r->registerHandler(network::EnumNameServerMsgType(network::ServerMsgType::EntityRemove), std::make_shared<EntityRemoveHandler>());
	r->registerHandler(network::EnumNameServerMsgType(network::ServerMsgType::EntityUpdate), std::make_shared<EntityUpdateHandler>());
	r->registerHandler(network::EnumNameServerMsgType(network::ServerMsgType::UserSpawn), std::make_shared<UserSpawnHandler>());
	r->registerHandler(network::EnumNameServerMsgType(network::ServerMsgType::AuthFailed), std::make_shared<AuthFailedHandler>());
	r->registerHandler(network::EnumNameServerMsgType(network::ServerMsgType::Seed), std::make_shared<SeedHandler>(_world));

	core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}

	video::enableDebug(video::DebugSeverity::Medium);

	if (!_network->init()) {
		return core::AppState::Cleanup;
	}

	_camera.init(glm::ivec2(), dimension());
	_camera.setRotationType(video::CameraRotationType::Target);
	_camera.setTargetDistance(_maxTargetDistance->floatVal());
	_waiting.init();

	_meshPool->init();

	if (!voxel::initDefaultMaterialColors()) {
		Log::error("Failed to initialize the palette data");
		return core::AppState::Cleanup;
	}

	if (!_world->init(filesystem()->load("world.lua"), filesystem()->load("biomes.lua"))) {
		return core::AppState::Cleanup;
	}

	if (!_worldRenderer.init(glm::ivec2(), _dimension)) {
		return core::AppState::Cleanup;
	}

	RestClient::init();

	_root.SetSkinBg(TBIDC("background"));
	_voxelFont.init("font.ttf", 14, 1, true, " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~€‚ƒ„…†‡ˆ‰Š‹ŒŽ‘’“”•–—˜™š›œžŸ¡¢£¤¥¦§¨©ª«¬®¯°±²³´µ¶·¸¹º»¼½¾¿ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþÿ");

	handleLogin();

	return state;
}

void Client::handleLogin() {
	const core::VarPtr& autoLoginVar = core::Var::getSafe(cfg::ClientAutoLogin);
	if (autoLoginVar->boolVal()) {
		const int port = core::Var::getSafe(cfg::ClientPort)->intVal();
		const std::string& host = core::Var::getSafe(cfg::ClientHost)->strVal();
		Log::info("Trying to connect to server %s:%i", host.c_str(), port);
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

	if (_world->created()) {
		if (_player) {
			const glm::vec3& pos = _player->position();
			_camera.setTarget(pos);
		}
		_camera.setFarPlane(_worldRenderer.getViewDistance());
		_camera.init(glm::ivec2(), dimension());
		_camera.update(_deltaFrame);

		_drawCallsWorld = _worldRenderer.renderWorld(_camera);
		_drawCallsEntities = _worldRenderer.renderEntities(_camera);
		_worldRenderer.extractMeshes(_camera.position());
	} else {
		_drawCallsWorld = 0;
		_drawCallsEntities = 0;
	}
}

void Client::afterRootWidget() {
	const glm::vec3& pos = _camera.position();
	const glm::vec3& target = _camera.target();
	frontend::WorldRenderer::Stats stats;
	_worldRenderer.stats(stats);
	const int x = 5;
	enqueueShowStr(x, core::Color::White, "drawcalls world: %i", _drawCallsWorld);
	enqueueShowStr(x, core::Color::White, "drawcalls entities: %i", _drawCallsEntities);
	enqueueShowStr(x, core::Color::White, "pos: %.2f:%.2f:%.2f", pos.x, pos.y, pos.z);
	enqueueShowStr(x, core::Color::White, "pending: %i, meshes: %i, extracted: %i, uploaded: %i, visible: %i, octreesize: %i, octreeactive: %i, occluded: %i",
			stats.pending, stats.meshes, stats.extracted, stats.active, stats.visible, stats.octreeSize, stats.octreeActive, stats.occluded);
	enqueueShowStr(x, core::Color::White, "pos: %.2f:%.2f:%.2f (target: %.2f:%.2f:%.2f)", pos.x, pos.y, pos.z, target.x, target.y, target.z);

	if (hasState(CLIENT_CONNECTING)) {
		_waiting.render();
	}

	Super::afterRootWidget();
}

core::AppState Client::onCleanup() {
	eventBus()->unsubscribe<network::NewConnectionEvent>(*this);
	eventBus()->unsubscribe<network::DisconnectEvent>(*this);
	eventBus()->unsubscribe<voxel::WorldCreatedEvent>(*this);

	Log::info("shutting down the client");
	disconnect();
	_voxelFont.shutdown();
	_meshPool->shutdown();
	_worldRenderer.shutdown();
	core::AppState state = Super::onCleanup();
	_world->shutdown();
	_player = frontend::ClientEntityPtr();
	_network->shutdown();
	_waiting.shutdown();

	RestClient::disable();

	return state;
}

void Client::onMouseWheel(int32_t x, int32_t y) {
	Super::onMouseWheel(x, y);
	const float targetDistance = glm::clamp(_camera.targetDistance() - y, 0.0f, _maxTargetDistance->floatVal());
	_camera.setTargetDistance(targetDistance);
}

bool Client::onKeyPress(int32_t key, int16_t modifier) {
	if (Super::onKeyPress(key, modifier)) {
		return true;
	}

	if (key == SDLK_ESCAPE) {
		if (hasState(CLIENT_CONNECTING)) {
			removeState(CLIENT_CONNECTING);
			disconnect();
			_network->disconnect();
		}
	}

	return false;
}

core::AppState Client::onRunning() {
	_waiting.update(_deltaFrame);
	core::AppState state = Super::onRunning();
	core::Var::visitBroadcast([] (const core::VarPtr& var) {
		Log::info("TODO: %s needs broadcast", var->name().c_str());
	});
	sendMovement();
	if (state == core::AppState::Running) {
		_network->update();
		_world->onFrame(_deltaFrame);
		if (_world->created()) {
			_worldRenderer.onRunning(_camera, _deltaFrame);
		}
	}

	return state;
}

void Client::onWindowResize() {
	Super::onWindowResize();
	_camera.init(glm::ivec2(), dimension());
}

void Client::signup(const std::string& email, const std::string& password) {
	RestClient::Connection conn(core::Var::getSafe(cfg::HTTPBaseURL)->strVal());
	conn.AppendHeader("Content-Type", "text/json");
	const RestClient::Response r = conn.post("signup",
		"{"
			"\"email\": \"" + email + "\", "
			"\"password\": \"" + password + "\""
		"}");
	if (r.code != 200) {
		Log::error("Failed to signup with %s (%i)", email.c_str(), r.code);
	}
}

void Client::lostPassword(const std::string& email) {
	RestClient::Connection conn(core::Var::getSafe(cfg::HTTPBaseURL)->strVal());
	conn.AppendHeader("Content-Type", "text/json");
	const RestClient::Response r = conn.post("lostpassword", "{\"email\": \"" + email + "\"}");
	if (r.code != 200) {
		Log::error("Failed to request the password reset for %s (%i)", email.c_str(), r.code);
	}
}

void Client::authFailed() {
	removeState(CLIENT_CONNECTING);
	core::Var::getSafe(cfg::ClientAutoLogin)->setVal(false);
	// TODO: stack (push/pop in UIApp) window support
	ui::Window* main = new frontend::LoginWindow(this);
	new frontend::AuthFailedWindow(main);
}

void Client::disconnect() {
	flatbuffers::FlatBufferBuilder fbb;
	_messageSender->sendClientMessage(_peer, fbb, network::ClientMsgType::UserDisconnect, network::CreateUserDisconnect(fbb).Union());
}

void Client::entityUpdate(frontend::ClientEntityId id, const glm::vec3& pos, float orientation) {
	const frontend::ClientEntityPtr& entity = _worldRenderer.getEntity(id);
	if (!entity) {
		Log::warn("Could not get entity with id %li", id);
		return;
	}
	entity->lerpPosition(pos, orientation);
}

void Client::entitySpawn(frontend::ClientEntityId id, network::EntityType type, float orientation, const glm::vec3& pos) {
	Log::info("Entity %li spawned at pos %f:%f:%f (type %i)", id, pos.x, pos.y, pos.z, (int)type);
	const std::string_view& meshName = "chr_skelett2_bake"; // core::string::toLower(network::EnumNameEntityType(type));
	const video::MeshPtr& mesh = _meshPool->getMesh(meshName);
	_worldRenderer.addEntity(std::make_shared<frontend::ClientEntity>(id, type, pos, orientation, mesh));
}

void Client::entityRemove(frontend::ClientEntityId id) {
	_worldRenderer.removeEntity(id);
}

void Client::spawn(frontend::ClientEntityId id, const char *name, const glm::vec3& pos, float orientation) {
	removeState(CLIENT_CONNECTING);
	Log::info("User %li (%s) logged in at pos %f:%f:%f with orientation: %f", id, name, pos.x, pos.y, pos.z, orientation);
	_camera.setTarget(pos);
	const video::MeshPtr& mesh = _meshPool->getMesh("chr_skelett2_bake");
	const network::EntityType type = network::EntityType::PLAYER;
	_player = std::make_shared<frontend::ClientEntity>(id, type, pos, orientation, mesh);
	_worldRenderer.addEntity(_player);
	_worldRenderer.extractMeshes(pos);

	flatbuffers::FlatBufferBuilder fbb;
	_messageSender->sendClientMessage(_peer, fbb, network::ClientMsgType::UserConnected,
			network::CreateUserConnected(fbb).Union());
}

bool Client::connect(uint16_t port, const std::string& hostname) {
	setState(CLIENT_CONNECTING);
	ENetPeer* peer = _network->connect(port, hostname);
	if (!peer) {
		removeState(CLIENT_CONNECTING);
		Log::error("Failed to connect to server %s:%i", hostname.c_str(), port);
		return false;
	}

	peer->data = this;

	_peer = peer;
	Log::info("Connected to server %s:%i", hostname.c_str(), port);
	_waiting.setTextId("stateconnecting");
	return true;
}

int main(int argc, char *argv[]) {
	const video::MeshPoolPtr& meshPool = std::make_shared<video::MeshPool>();
	const core::EventBusPtr& eventBus = std::make_shared<core::EventBus>();
	const voxel::WorldPtr& world = std::make_shared<voxel::World>();
	const core::TimeProviderPtr& timeProvider = std::make_shared<core::TimeProvider>();
	const io::FilesystemPtr& filesystem = std::make_shared<io::Filesystem>();
	const network::ProtocolHandlerRegistryPtr& protocolHandlerRegistry = std::make_shared<network::ProtocolHandlerRegistry>();
	const network::NetworkPtr& network = std::make_shared<network::Network>(protocolHandlerRegistry, eventBus);
	const network::MessageSenderPtr& messageSender = std::make_shared<network::MessageSender>(network);
	Client app(meshPool, network, world, messageSender, eventBus, timeProvider, filesystem);
	return app.startMainLoop(argc, argv);
}
