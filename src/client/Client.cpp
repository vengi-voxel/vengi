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
#include "core/Command.h"
#include "core/GLM.h"
#include "core/Color.h"
#include "video/GLDebug.h"
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
	});

Client::Client(video::MeshPoolPtr meshPool, network::NetworkPtr network, voxel::WorldPtr world, network::MessageSenderPtr messageSender,
		core::EventBusPtr eventBus, core::TimeProviderPtr timeProvider, io::FilesystemPtr filesystem) :
		Super(filesystem, eventBus, 17816), _camera(), _meshPool(meshPool), _network(network), _world(world), _messageSender(messageSender),
		_timeProvider(timeProvider), _worldRenderer(world), _waiting(this) {
	_world->setClientData(true);
	init("engine", "client");
}

Client::~Client() {
	core::Command::unregisterCommand("+move_right");
	core::Command::unregisterCommand("+move_left");
	core::Command::unregisterCommand("+move_upt");
	core::Command::unregisterCommand("+move_down");
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
	const std::string& email = core::Var::get(cfg::ClientEmail)->strVal();
	const std::string& password = core::Var::get(cfg::ClientPassword)->strVal();
	Log::info("Trying to log into the server with %s", email.c_str());
	_messageSender->sendClientMessage(_peer, fbb, network::ClientMsgType::UserConnect,
			network::CreateUserConnect(fbb, fbb.CreateString(email), fbb.CreateString(password)).Union());
}

void Client::onEvent(const voxel::WorldCreatedEvent& event) {
	Log::info("world created");
	new frontend::HudWindow(this, _dimension);
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
	if (state != core::Running) {
		return state;
	}

	GLDebug::enable(GLDebug::Medium);

	if (!_network->init()) {
		return core::Cleanup;
	}

	core::Var::get(cfg::ClientName, "noname");
	core::Var::get(cfg::ClientPassword, "nopassword");
	core::Var::get(cfg::HTTPBaseURL, "https://localhost/");
	_rotationSpeed = core::Var::get(cfg::ClientMouseRotationSpeed, "0.1");

	GL_checkError();

	_maxTargetDistance = core::Var::get(cfg::ClientCameraMaxTargetDistance, "250.0");
	_camera.init(dimension());
	_camera.setRotationType(video::CameraRotationType::Target);
	_camera.setTargetDistance(_maxTargetDistance->floatVal());
	_waiting.init();

	registerMoveCmd("+move_right", MOVERIGHT);
	registerMoveCmd("+move_left", MOVELEFT);
	registerMoveCmd("+move_forward", MOVEFORWARD);
	registerMoveCmd("+move_backward", MOVEBACKWARD);

	if (!_worldRenderer.onInit(_dimension)) {
		return core::Cleanup;
	}

	RestClient::init();

	_root.SetSkinBg(TBIDC("background"));

	handleLogin();

	return state;
}

void Client::handleLogin() {
	const core::VarPtr& autoLoginVar = core::Var::get(cfg::ClientAutoLogin);
	if (autoLoginVar->boolVal()) {
		const int port = core::Var::get(cfg::ClientPort)->intVal();
		const std::string& host = core::Var::get(cfg::ClientHost)->strVal();
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

	if (_world->isCreated()) {
		if (_player) {
			const glm::vec3& pos = _player->position();
			_camera.setTarget(pos);
		}
		_camera.setFarPlane(_worldRenderer.getViewDistance());
		_camera.setAspectRatio(_aspect);
		_camera.update(_deltaFrame);

		_drawCallsWorld = _worldRenderer.renderWorld(_camera);
		_drawCallsEntities = _worldRenderer.renderEntities(_camera);
		_worldRenderer.extractNewMeshes(_camera.position());
	} else {
		_drawCallsWorld = 0;
		_drawCallsEntities = 0;
	}
}

void Client::afterUI() {
	tb::TBStr drawCallsWorld;
	drawCallsWorld.SetFormatted("drawcalls world: %i", _drawCallsWorld);
	tb::TBStr drawCallsEntity;
	drawCallsEntity.SetFormatted("drawcalls entities: %i", _drawCallsEntities);
	tb::TBFontFace *font = _root.GetFont();
	font->DrawString(5, 20, tb::TBColor(255, 255, 255), drawCallsEntity);
	font->DrawString(5, 35, tb::TBColor(255, 255, 255), drawCallsWorld);
	const glm::vec3& pos = _camera.position();
	const glm::vec3& target = _camera.target();
	tb::TBStr position;
	position.SetFormatted("pos: %.2f:%.2f:%.2f (target: %.2f:%.2f:%.2f)", pos.x, pos.y, pos.z, target.x, target.y, target.z);
	font->DrawString(5, 50, tb::TBColor(255, 255, 255), position);

	if (hasState(CLIENT_CONNECTING)) {
		_waiting.render();
	}

	Super::afterUI();
}

core::AppState Client::onCleanup() {
	eventBus()->unsubscribe<network::NewConnectionEvent>(*this);
	eventBus()->unsubscribe<network::DisconnectEvent>(*this);
	eventBus()->unsubscribe<voxel::WorldCreatedEvent>(*this);

	Log::info("shutting down the client");
	disconnect();
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
	_timeProvider->update(_now);
	_waiting.update(_deltaFrame);
	core::AppState state = Super::onRunning();
	sendMovement();
	if (state == core::AppState::Running) {
		_network->update();
		_world->onFrame(_deltaFrame);
		if (_world->isCreated()) {
			_worldRenderer.onRunning(_deltaFrame);
		}
	}

	return state;
}

void Client::onWindowResize() {
	Super::onWindowResize();
	_camera.init(dimension());
}

void Client::signup(const std::string& email, const std::string& password) {
	RestClient::Connection conn(core::Var::get(cfg::HTTPBaseURL)->strVal());
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
	RestClient::Connection conn(core::Var::get(cfg::HTTPBaseURL)->strVal());
	conn.AppendHeader("Content-Type", "text/json");
	const RestClient::Response r = conn.post("lostpassword", "{\"email\": \"" + email + "\"}");
	if (r.code != 200) {
		Log::error("Failed to request the password reset for %s (%i)", email.c_str(), r.code);
	}
}

void Client::authFailed() {
	removeState(CLIENT_CONNECTING);
	core::Var::get(cfg::ClientAutoLogin)->setVal(false);
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
	const std::string& meshName = "chr_skelett2_bake"; // core::string::toLower(network::EnumNameEntityType(type));
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
	//const glm::vec3 lookAtPos(pos.x + 1, pos.y, pos.z);
	// TODO: take orientation into account
	//_camera.lookAt(lookAtPos);
	// broken: _camera.setAngles(0.0f, orientation);
	const video::MeshPtr& mesh = _meshPool->getMesh("chr_skelett2_bake");
	const network::EntityType type = network::EntityType::PLAYER;
	_player = std::make_shared<frontend::ClientEntity>(id, type, pos, orientation, mesh);
	_worldRenderer.addEntity(_player);
	_worldRenderer.onSpawn(pos);

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
	const video::MeshPoolPtr meshPool = std::make_shared<video::MeshPool>();
	const core::EventBusPtr eventBus = std::make_shared<core::EventBus>();
	const voxel::WorldPtr world = std::make_shared<voxel::World>();
	const core::TimeProviderPtr timeProvider = std::make_shared<core::TimeProvider>();
	const io::FilesystemPtr filesystem = std::make_shared<io::Filesystem>();
	const network::ProtocolHandlerRegistryPtr protocolHandlerRegistry = std::make_shared<network::ProtocolHandlerRegistry>();
	const network::NetworkPtr network = std::make_shared<network::Network>(protocolHandlerRegistry, eventBus);
	const network::MessageSenderPtr messageSender = std::make_shared<network::MessageSender>(network);
	Client app(meshPool, network, world, messageSender, eventBus, timeProvider, filesystem);
	return app.startMainLoop(argc, argv);
}
