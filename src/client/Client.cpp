#include "Client.h"
#include "network/messages/ClientMessages.h"
#include "ui/LoginWindow.h"
#include "ui/DisconnectWindow.h"
#include "ui/AuthFailedWindow.h"
#include "ui/HudWindow.h"
#include "core/Command.h"
#include "sauce/ClientInjector.h"
#include "video/Color.h"
#include "video/GLDebug.h"

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
		UIApp(filesystem, eventBus), _meshPool(meshPool), _network(network), _world(world), _messageSender(messageSender),
		_timeProvider(timeProvider), _worldShader(), _meshShader(new frontend::MeshShader()),
		_worldRenderer(world) {
	_world->setClientData(true);
	init("engine", "client");
	_rotationSpeed = core::Var::get(cfg::ClientMouseRotationSpeed, "0.01");
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

	// TODO: only send if _moveMask differs
	_lastMovement = _now;
	flatbuffers::FlatBufferBuilder fbb;
	const MoveDirection md = (MoveDirection) _moveMask;
	_messageSender->sendClientMessage(_peer, fbb, Type_Move, CreateMove(fbb, md, _camera.pitch(), _camera.yaw()).Union(), 0);
}

void Client::onMouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) {
	UIApp::onMouseMotion(x, y, relX, relY);
	_camera.onMotion(x, y, relX, relY, _rotationSpeed->floatVal());
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
	new frontend::HudWindow(this, _width, _height);
}

core::AppState Client::onInit() {
	eventBus()->subscribe<network::NewConnectionEvent>(*this);
	eventBus()->subscribe<network::DisconnectEvent>(*this);
	eventBus()->subscribe<voxel::WorldCreatedEvent>(*this);

	GLDebug::enable(GLDebug::Medium);

	core::AppState state = UIApp::onInit();
	if (state != core::Running)
		return state;

	if (!_network->start())
		return core::Cleanup;

	core::Var::get(cfg::ClientName, "noname");
	core::Var::get(cfg::ClientPassword, "nopassword");

	if (!_worldShader.init()) {
		return core::Cleanup;
	}
	if (!_meshShader->init()) {
		return core::Cleanup;
	}

	GL_checkError();

	_camera.init(_width, _height);

	registerMoveCmd("+move_right", MOVERIGHT);
	registerMoveCmd("+move_left", MOVELEFT);
	registerMoveCmd("+move_forward", MOVEFORWARD);
	registerMoveCmd("+move_backward", MOVEBACKWARD);

	_worldRenderer.onInit();
	_clearColor = video::Color::LightBlue;

	_root.SetSkinBg(TBIDC("background"));
	new frontend::LoginWindow(this);

	SDL_GL_SetSwapInterval(core::Var::get(cfg::ClientVSync, "false")->boolVal());

	return state;
}

void Client::renderBackground() {
	_camera.setAngles(-M_PI_2, M_PI);
	_camera.setPosition(glm::vec3(0.0f, 100.0f, 0.0f));
	_camera.update();
}

void Client::beforeUI() {
	UIApp::beforeUI();

	if (_world->isCreated()) {
		const bool left = _moveMask & MoveDirection_MOVELEFT;
		const bool right = _moveMask & MoveDirection_MOVERIGHT;
		const bool forward = _moveMask & MoveDirection_MOVEFORWARD;
		const bool backward = _moveMask & MoveDirection_MOVEBACKWARD;
		_camera.updatePosition(_deltaFrame, left, right, forward, backward);
		_camera.updateViewMatrix();

		const glm::mat4& view = _camera.getViewMatrix();
		_drawCallsWorld = _worldRenderer.renderWorld(_worldShader, view, _aspect);
		_drawCallsEntities = _worldRenderer.renderEntities(_meshShader, view, _aspect);
		_worldRenderer.extractNewMeshes(_camera.getPosition());
	} else {
		_drawCallsWorld = 0;
		_drawCallsEntities = 0;
		renderBackground();
	}
}

void Client::afterUI() {
	UIApp::afterUI();
	tb::TBStr drawCallsWorld;
	drawCallsWorld.SetFormatted("drawcalls world: %i", _drawCallsWorld);
	tb::TBStr drawCallsEntity;
	drawCallsEntity.SetFormatted("drawcalls entities: %i", _drawCallsEntities);
	_root.GetFont()->DrawString(5, 20, tb::TBColor(255, 255, 255), drawCallsEntity);
	_root.GetFont()->DrawString(5, 35, tb::TBColor(255, 255, 255), drawCallsWorld);
}

core::AppState Client::onCleanup() {
	_worldRenderer.onCleanup();
	core::AppState state = UIApp::onCleanup();
	_world->destroy();
	return state;
}

core::AppState Client::onRunning() {
	_timeProvider->update(_now);
	core::AppState state = UIApp::onRunning();
	sendMovement();
	if (state == core::AppState::Running) {
		_posLerp.update(_now);
		const glm::vec3& pos = _posLerp.position();
		_camera.setPosition(pos);
		_network->update();
		_world->onFrame(_deltaFrame);
		if (_world->isCreated()) {
			_worldRenderer.onRunning(_now);
		}
	}

	return state;
}

void Client::authFailed() {
	ui::Window* main = new frontend::LoginWindow(this);
	new frontend::AuthFailedWindow(main);
}

void Client::disconnect() {
	flatbuffers::FlatBufferBuilder fbb;
	_messageSender->sendClientMessage(_peer, fbb, Type_UserDisconnect, CreateUserDisconnect(fbb).Union());
}

void Client::npcUpdate(frontend::ClientEntityId id, const glm::vec3& pos, float orientation) {
	const frontend::ClientEntityPtr& entity = _worldRenderer.getEntity(id);
	if (!entity) {
		return;
	}
	entity->lerpPosition(_now, pos, orientation);
}

void Client::npcSpawn(frontend::ClientEntityId id, network::messages::NpcType type, const glm::vec3& pos) {
	Log::info("NPC %li spawned at pos %f:%f:%f (type %i)", id, pos.x, pos.y, pos.z, type);
	const std::string& meshName = core::string::toLower(network::messages::EnumNameNpcType(type));
	_worldRenderer.addEntity(frontend::ClientEntityPtr(new frontend::ClientEntity(id, type, _now, pos, 0.0f, _meshPool->getMesh(meshName))));
}

void Client::userUpdate(const glm::vec3& position) {
	_posLerp.lerpPosition(_now, position);
}

void Client::npcRemove(frontend::ClientEntityId id) {
	_worldRenderer.removeEntity(id);
}

void Client::spawn(frontend::ClientEntityId id, const char *name, const glm::vec3& pos) {
	Log::info("User %li (%s) logged in at pos %f:%f:%f", id, name, pos.x, pos.y, pos.z);
	_userId = id;
	_posLerp.setPosition(_now, pos);
	_camera.setPosition(pos);
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
	getInjector()->get<Client>()->startMainLoop(argc, argv);
	return EXIT_SUCCESS;
}
