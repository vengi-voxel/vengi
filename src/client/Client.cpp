/**
 * @file
 */

#include "Client.h"
#include "messages/ClientMessages_generated.h"
#include "ui/LoginWindow.h"
#include "ui/DisconnectWindow.h"
#include "ui/AuthFailedWindow.h"
#include "ui/HudWindow.h"
#include "core/Command.h"
#include "core/GLM.h"
#include "core/Color.h"
#include "video/GLDebug.h"
#include "ClientModule.h"
#include "network/ClientNetworkModule.h"

#define registerMoveCmd(name, flag) \
	core::Command::registerCommand(name, [&] (const core::CmdArgs& args) { \
		if (args.empty()) { \
			return; \
		} \
		if (args[0] == "true") \
			_moveMask |= MoveDirection_##flag; \
		else \
			_moveMask &= ~MoveDirection_##flag; \
	});

Client::Client(video::MeshPoolPtr meshPool, network::NetworkPtr network, voxel::WorldPtr world, network::MessageSenderPtr messageSender,
		core::EventBusPtr eventBus, core::TimeProviderPtr timeProvider, io::FilesystemPtr filesystem) :
		UIApp(filesystem, eventBus, 17816), _camera(), _meshPool(meshPool), _network(network), _world(world), _messageSender(messageSender),
		_timeProvider(timeProvider), _worldRenderer(world) {
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
	if (_peer == nullptr)
		return;

	if (_now - _lastMovement <= 100L)
		return;

	if (_lastMoveMask == _moveMask)
		return;
	_lastMovement = _now;
	_lastMoveMask = _moveMask;
	flatbuffers::FlatBufferBuilder fbb;
	const MoveDirection md = (MoveDirection) _moveMask;
	_messageSender->sendClientMessage(_peer, fbb, Type_Move, CreateMove(fbb, md, _camera.pitch(), _camera.yaw()).Union(), 0);
}

void Client::onMouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) {
	UIApp::onMouseMotion(x, y, relX, relY);
	_camera.rotate(glm::vec3(relY, relX, 0.0f) * _rotationSpeed->floatVal());
}

void Client::onEvent(const network::DisconnectEvent& event) {
	ui::Window* main = new frontend::LoginWindow(this);
	new frontend::DisconnectWindow(main);
}

void Client::onEvent(const network::NewConnectionEvent& event) {
	flatbuffers::FlatBufferBuilder fbb;
	const std::string& email = core::Var::get(cfg::ClientEmail)->strVal();
	const std::string& password = core::Var::get(cfg::ClientPassword)->strVal();
	Log::info("Trying to log into the server with %s", email.c_str());
	_messageSender->sendClientMessage(_peer, fbb, Type_UserConnect,
			CreateUserConnect(fbb, fbb.CreateString(email), fbb.CreateString(password)).Union());
}

void Client::onEvent(const voxel::WorldCreatedEvent& event) {
	Log::info("world created");
	new frontend::HudWindow(this, _dimension);
}

core::AppState Client::onInit() {
	eventBus()->subscribe<network::NewConnectionEvent>(*this);
	eventBus()->subscribe<network::DisconnectEvent>(*this);
	eventBus()->subscribe<voxel::WorldCreatedEvent>(*this);

	core::AppState state = UIApp::onInit();
	if (state != core::Running) {
		return state;
	}

	GLDebug::enable(GLDebug::Medium);

	if (!_network->init()) {
		return core::Cleanup;
	}

	core::Var::get(cfg::ClientName, "noname");
	core::Var::get(cfg::ClientPassword, "nopassword");
	_rotationSpeed = core::Var::get(cfg::ClientMouseRotationSpeed, "0.1");

	GL_checkError();

	_camera.init(dimension());

	registerMoveCmd("+move_right", MOVERIGHT);
	registerMoveCmd("+move_left", MOVELEFT);
	registerMoveCmd("+move_forward", MOVEFORWARD);
	registerMoveCmd("+move_backward", MOVEBACKWARD);

	if (!_worldRenderer.onInit(_dimension)) {
		return core::Cleanup;
	}

	_root.SetSkinBg(TBIDC("background"));
	new frontend::LoginWindow(this);

	return state;
}

void Client::renderBackground() {
	_camera.setAngles(-glm::half_pi<float>(), glm::pi<float>());
	_camera.setPosition(glm::vec3(0.0f, 100.0f, 0.0f));
	_camera.update(0L);
}

void Client::beforeUI() {
	UIApp::beforeUI();

	if (_world->isCreated()) {
		glm::vec3 moveDelta = glm::vec3();
		const float speed = 0.01f * static_cast<float>(_deltaFrame);
		if (_moveMask & MoveDirection_MOVELEFT) {
			moveDelta += glm::left * speed;
		}
		if (_moveMask & MoveDirection_MOVERIGHT) {
			moveDelta += glm::right * speed;
		}
		if (_moveMask & MoveDirection_MOVEFORWARD) {
			moveDelta += glm::forward * speed;
		}
		if (_moveMask & MoveDirection_MOVEBACKWARD) {
			moveDelta += glm::backward * speed;
		}
		_camera.move(moveDelta);

		_camera.setFarPlane(_worldRenderer.getViewDistance());
		_camera.setAspectRatio(_aspect);
		_camera.update(_deltaFrame);

		_drawCallsWorld = _worldRenderer.renderWorld(_camera);
		_drawCallsEntities = _worldRenderer.renderEntities(_camera);
		_worldRenderer.extractNewMeshes(_camera.position());
	} else {
		_drawCallsWorld = 0;
		_drawCallsEntities = 0;
		renderBackground();
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
	UIApp::afterUI();
}

core::AppState Client::onCleanup() {
	_meshPool->shutdown();
	_worldRenderer.shutdown();
	core::AppState state = UIApp::onCleanup();
	_world->shutdown();
	_player = frontend::ClientEntityPtr();
	_network->shutdown();
	return state;
}

core::AppState Client::onRunning() {
	_timeProvider->update(_now);
	core::AppState state = UIApp::onRunning();
	sendMovement();
	if (state == core::AppState::Running) {
		if (_player) {
			const glm::vec3& pos = _player->position();
			_camera.setPosition(pos);
		}
		_network->update();
		_world->onFrame(_deltaFrame);
		if (_world->isCreated()) {
			_worldRenderer.onRunning(_deltaFrame);
		}
	}

	return state;
}

void Client::onWindowResize() {
	UIApp::onWindowResize();
	_camera.init(dimension());
}

void Client::authFailed() {
	ui::Window* main = new frontend::LoginWindow(this);
	new frontend::AuthFailedWindow(main);
}

void Client::disconnect() {
	flatbuffers::FlatBufferBuilder fbb;
	_messageSender->sendClientMessage(_peer, fbb, Type_UserDisconnect, CreateUserDisconnect(fbb).Union());
}

void Client::entityUpdate(frontend::ClientEntityId id, const glm::vec3& pos, float orientation) {
	const frontend::ClientEntityPtr& entity = _worldRenderer.getEntity(id);
	if (!entity) {
		return;
	}
	entity->lerpPosition(_now, pos, orientation);
}

void Client::npcSpawn(frontend::ClientEntityId id, network::messages::NpcType type, const glm::vec3& pos) {
	Log::info("NPC %li spawned at pos %f:%f:%f (type %i)", id, pos.x, pos.y, pos.z, type);
	const std::string& meshName = core::string::toLower(network::messages::EnumNameNpcType(type));
	_worldRenderer.addEntity(std::make_shared<frontend::ClientEntity>(id, type, _now, pos, 0.0f, _meshPool->getMesh(meshName)));
}

void Client::entityRemove(frontend::ClientEntityId id) {
	_worldRenderer.removeEntity(id);
}

void Client::spawn(frontend::ClientEntityId id, const char *name, const glm::vec3& pos) {
	Log::info("User %li (%s) logged in at pos %f:%f:%f", id, name, pos.x, pos.y, pos.z);
	_camera.setPosition(pos);
	_player = std::make_shared<frontend::ClientEntity>(id, -1, _now, pos, 0.0f, _meshPool->getMesh("chr_fatkid"));
	_worldRenderer.addEntity(_player);
	_worldRenderer.onSpawn(pos);
}

bool Client::connect(uint16_t port, const std::string& hostname) {
	ENetPeer* peer = _network->connect(port, hostname);
	if (!peer) {
		Log::error("Failed to connect to server %s:%i", hostname.c_str(), port);
		return false;
	}

	peer->data = this;

	_peer = peer;
	Log::info("Connected to server %s:%i", hostname.c_str(), port);
	return true;
}

int main(int argc, char *argv[]) {
	return core::getAppWithModules<Client>(ClientModule(), ClientNetworkModule())->startMainLoop(argc, argv);
}
