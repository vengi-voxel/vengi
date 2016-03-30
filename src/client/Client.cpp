#include "Client.h"
#include "voxel/Spiral.h"
#include "network/messages/ClientMessages.h"
#include "frontend/ui/LoginWindow.h"
#include "frontend/ui/DisconnectWindow.h"
#include "frontend/ui/AuthFailedWindow.h"
#include "frontend/ui/HudWindow.h"
#include <PolyVox/CubicSurfaceExtractor.h>
#include <PolyVox/RawVolume.h>
#include "core/Command.h"
#include "sauce/ClientInjector.h"
#include "video/GLDebug.h"
#include "noise/SimplexNoise.h"

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
		UIApp(filesystem, eventBus), _meshPool(meshPool), _network(network), _world(world), _messageSender(messageSender), _timeProvider(
				timeProvider), _worldShader(), _meshShader(new frontend::MeshShader()), _waterShader(new frontend::WaterShader()), _userId(-1), _peer(nullptr), _moveMask(0), _lastMovement(
				0L) {
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

	// TODO: only send if _moveMask differs
	_lastMovement = _now;
	flatbuffers::FlatBufferBuilder fbb;
	const MoveDirection md = (MoveDirection) _moveMask;
	_messageSender->sendClientMessage(_peer, fbb, Type_Move, CreateMove(fbb, md, _camera.pitch(), _camera.yaw()).Union(), 0);
}

void Client::onMouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) {
	UIApp::onMouseMotion(x, y, relX, relY);
	_camera.onMotion(x, y, relX, relY);
}

void Client::onEvent(const network::DisconnectEvent& event) {
	ui::Window* main = new frontend::LoginWindow(this);
	new frontend::DisconnectWindow(main);
}

void Client::onEvent(const network::NewConnectionEvent& event) {
	flatbuffers::FlatBufferBuilder fbb;
	const std::string& email = core::Var::get("cl_email")->strVal();
	const std::string& password = core::Var::get("cl_password")->strVal();
	Log::info("Trying to log into the server with %s", email.c_str());
	_messageSender->sendClientMessage(_peer, fbb, Type_UserConnect,
			CreateUserConnect(fbb, fbb.CreateString(email), fbb.CreateString(password)).Union());
}

void Client::onEvent(const voxel::WorldCreatedEvent& event) {
	Log::info("world created");
	const int size = core::Var::get("cl_chunksize")->intVal();
	glm::ivec2 pos = _lastCameraPosition;
	voxel::Spiral o;
	for (int i = 0; i < 1000; ++i) {
		event.world()->scheduleMeshExtraction(pos);
		o.next();
		pos.x = _lastCameraPosition.x + o.x() * size;
		pos.y = _lastCameraPosition.y + o.y() * size;
	}
	new frontend::HudWindow(this, _width, _height);
}

// TODO: generate bigger buffers and use glBufferSubData
video::GLMeshData Client::createMesh(voxel::DecodedMesh& surfaceMesh, const glm::ivec2& translation, float scale) {
	// Convenient access to the vertices and indices
	const uint32_t* vecIndices = surfaceMesh.getRawIndexData();
	const uint32_t numIndices = surfaceMesh.getNoOfIndices();
	const voxel::VoxelVertexDecoded* vecVertices = surfaceMesh.getRawVertexData();
	const uint32_t numVertices = surfaceMesh.getNoOfVertices();

	// This struct holds the OpenGL properties (buffer handles, etc) which will be used
	// to render our mesh. We copy the data from the PolyVox mesh into this structure.
	video::GLMeshData meshData;

	// Create the VAO for the mesh
	glGenVertexArrays(1, &meshData.vertexArrayObject);
	core_assert(meshData.vertexArrayObject > 0);
	glBindVertexArray(meshData.vertexArrayObject);

	// The GL_ARRAY_BUFFER will contain the list of vertex positions
	glGenBuffers(1, &meshData.vertexBuffer);
	core_assert(meshData.vertexBuffer > 0);
	glBindBuffer(GL_ARRAY_BUFFER, meshData.vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(voxel::VoxelVertexDecoded), vecVertices, GL_STATIC_DRAW);

	// and GL_ELEMENT_ARRAY_BUFFER will contain the indices
	glGenBuffers(1, &meshData.indexBuffer);
	core_assert(meshData.indexBuffer > 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshData.indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(uint32_t), vecIndices, GL_STATIC_DRAW);

	const int posLoc = _worldShader.enableVertexAttribute("a_pos");
	glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(voxel::VoxelVertexDecoded),
			GL_OFFSET(offsetof(voxel::VoxelVertexDecoded, position)));

	const int matLoc = _worldShader.enableVertexAttribute("a_materialdensity");
	// our material and density is encoded as 8 bits material and 8 bits density
	core_assert(sizeof(voxel::Voxel) == sizeof(uint16_t));
	glVertexAttribIPointer(matLoc, sizeof(voxel::Voxel), GL_UNSIGNED_BYTE, sizeof(voxel::VoxelVertexDecoded),
			GL_OFFSET(offsetof(voxel::VoxelVertexDecoded, data)));

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	Log::trace("mesh information:\n- mesh indices: %i, vertices: %i\n- position: %i:%i", numIndices, numVertices, translation.x,
			translation.y);

	meshData.noOfIndices = numIndices;
	meshData.translation = translation;
	meshData.scale = scale;
	meshData.indexType = GL_UNSIGNED_INT;
	return meshData;
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

	core::Var::get("cl_name", "noname");
	core::Var::get("cl_password", "nopassword");

	if (!_worldShader.init()) {
		return core::Cleanup;
	}
	if (!_meshShader->init()) {
		return core::Cleanup;
	}
	if (!_waterShader->init()) {
		return core::Cleanup;
	}

	_waterTexture = video::TexturePtr(new video::Texture("texture/water.png"));
	_waterTexture->load();
	const glm::vec3 vertices[4] = { glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f,
			1.0f, 0.0f) };
	const glm::ivec3 indices[2] = { glm::ivec3( 0, 1, 2 ), glm::ivec3( 1, 3, 2 ) };
	glGenVertexArrays(1, &_waterData.vertexArrayObject);
	core_assert(_waterData.vertexArrayObject > 0);
	glBindVertexArray(_waterData.vertexArrayObject);

	glGenBuffers(1, &_waterData.vertexBuffer);
	core_assert(_waterData.vertexBuffer > 0);
	glBindBuffer(GL_ARRAY_BUFFER, _waterData.vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(glm::vec3), vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &_waterData.indexBuffer);
	core_assert(_waterData.indexBuffer > 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _waterData.indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 2 * sizeof(glm::ivec2), indices, GL_STATIC_DRAW);

	const int posLoc = _waterShader->enableVertexAttribute("a_pos");
	glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(voxel::VoxelVertexDecoded),
			GL_OFFSET(offsetof(voxel::VoxelVertexDecoded, position)));

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	_waterData.noOfIndices = 2;
	_waterData.scale = 1.0f;
	_waterData.indexType = GL_UNSIGNED_INT;

	GL_checkError();

	_camera.init(_width, _height);

	registerMoveCmd("+move_right", MOVERIGHT);
	registerMoveCmd("+move_left", MOVELEFT);
	registerMoveCmd("+move_forward", MOVEFORWARD);
	registerMoveCmd("+move_backward", MOVEBACKWARD);

	const int ColorTextureSize = 256;
	uint8_t colorTexture[ColorTextureSize * ColorTextureSize * 3];
	noise::Simplex::SeamlessNoise2DRGB(colorTexture, ColorTextureSize, 3, 0.3f, 0.7f);
	_colorTexture = video::TexturePtr(new video::Texture(colorTexture, ColorTextureSize, ColorTextureSize, 3));

	_clearColor = glm::vec3(0.0, 0.6, 0.796);

	_root.SetSkinBg(TBIDC("background"));
	new frontend::LoginWindow(this);

	SDL_GL_SetSwapInterval(core::Var::get("cl_vsync", "true")->boolVal());

	return state;
}

void Client::renderBackground() {
	_camera.setAngles(-M_PI_2, M_PI);
	_camera.setPosition(glm::vec3(0.0f, 100.0f, 0.0f));
	_camera.update();
	const glm::mat4& view = _camera.getViewMatrix();
	const glm::mat4& projection = glm::perspective(45.0f, _aspect, 0.1f, 1000.0f);

	_meshShader->activate();
	_meshShader->setUniformMatrix("u_view", view, false);
	_meshShader->setUniformMatrix("u_projection", projection, false);
	_meshShader->setUniformVec3("u_lightpos", _lightPos);
	_meshShader->setUniformVec3("u_diffusecolor", _diffuseColor);
	_meshShader->setUniformVec3("u_specularcolor", _specularColor);
	_meshShader->setUniformi("u_texture", 0);
	const video::MeshPtr& mesh = _meshPool->getMesh("animal_rabbit");
	if (mesh->initMesh(_meshShader)) {
		const glm::mat4& translate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 20.0f));
		const glm::mat4& scale = glm::scale(translate, glm::vec3(0.01f));
		const glm::mat4& model = glm::rotate(scale, (float) glm::sin(_now / 100L), glm::vec3(0.0, 1.0, 0.0));
		_meshShader->setUniformMatrix("u_model", model, false);
		mesh->render();
	}
	_meshShader->deactivate();
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	GL_checkError();
}

void Client::renderMap() {
	voxel::DecodedMeshData mesh;
	while (_world->pop(mesh)) {
		// Now add the mesh to the list of meshes to render.
		addMeshData(createMesh(mesh.mesh, mesh.translation, 1.0f));
	}

	// TODO: use polyvox VolumeResampler to create a minimap of your volume

	const bool left = _moveMask & MoveDirection_MOVELEFT;
	const bool right = _moveMask & MoveDirection_MOVERIGHT;
	const bool forward = _moveMask & MoveDirection_MOVEFORWARD;
	const bool backward = _moveMask & MoveDirection_MOVEBACKWARD;
	_camera.updatePosition(_deltaFrame, left, right, forward, backward);
	_camera.updateViewMatrix();
	glClear(GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);

	GL_checkError();

	const glm::mat4& view = _camera.getViewMatrix();
	const glm::mat4& projection = glm::perspective(45.0f, _aspect, 0.1f, 1000.0f);

	_waterShader->activate();
	_waterShader->setUniformMatrix("u_model", glm::mat4(1.0f), false);
	_waterShader->setUniformMatrix("u_view", view, false);
	_waterShader->setUniformMatrix("u_projection", projection, false);
	_waterShader->setUniformVec3("u_lightpos", _lightPos);
	_waterShader->setUniformVec3("u_diffusecolor", _diffuseColor);
	_waterShader->setUniformVec3("u_specularcolor", _specularColor);
	_waterShader->setUniformi("u_texture", 0);
	_waterShader->setUniformf("u_wavetime", 0.5f);
	_waterShader->setUniformf("u_wavewidth", 0.6f);
	_waterShader->setUniformf("u_waveheight", 1.0f);
	_waterTexture->bind();
	glBindVertexArray(_waterData.vertexArrayObject);
	glDrawElements(GL_TRIANGLES, _waterData.noOfIndices, _waterData.indexType, 0);
	glBindVertexArray(0);
	_waterTexture->unbind();
	_waterShader->deactivate();
	GL_checkError();

	_worldShader.activate();
	_worldShader.setUniformMatrix("u_view", view, false);
	_worldShader.setUniformMatrix("u_projection", projection, false);
	_worldShader.setUniformVec3("u_lightpos", _lightPos);
	_worldShader.setUniformVec3("u_diffusecolor", _diffuseColor);
	_worldShader.setUniformVec3("u_specularcolor", _specularColor);
	// TODO: add culling
	for (const video::GLMeshData& meshData : _meshData) {
		const glm::mat4& model = glm::translate(glm::mat4(1.0f), glm::vec3(meshData.translation.x, 0, meshData.translation.y));
		_worldShader.setUniformMatrix("u_model", model, false);
		glBindVertexArray(meshData.vertexArrayObject);
		glDrawElements(GL_TRIANGLES, meshData.noOfIndices, meshData.indexType, 0);
	}
	_worldShader.deactivate();
	GL_checkError();

	_meshShader->activate();
	_meshShader->setUniformMatrix("u_view", view, false);
	_meshShader->setUniformMatrix("u_projection", projection, false);
	_meshShader->setUniformVec3("u_lightpos", _lightPos);
	_meshShader->setUniformVec3("u_diffusecolor", _diffuseColor);
	_meshShader->setUniformVec3("u_specularcolor", _specularColor);
	_meshShader->setUniformi("u_texture", 0);
	for (const auto& e : _entities) {
		const frontend::ClientEntityPtr& ent = e.second;
		ent->update(_now);
		const video::MeshPtr& mesh = ent->mesh();
		if (!mesh->initMesh(_meshShader))
			continue;
		const glm::mat4& translate = glm::translate(glm::mat4(1.0f), ent->position());
		const glm::mat4& scale = glm::scale(translate, glm::vec3(0.01f));
		const glm::mat4& model = glm::rotate(scale, ent->orientation(), glm::vec3(0.0, 1.0, 0.0));
		_meshShader->setUniformMatrix("u_model", model, false);
		mesh->render();
	}
	_meshShader->deactivate();
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	GL_checkError();

	// TODO: iterate over meshdata again and remove too far away chunks

	const glm::ivec2& camXZ = _world->getGridPos(_camera.getPosition());
	const glm::vec2 diff = _lastCameraPosition - camXZ;
	if (glm::length(diff.x) >= 1 || glm::length(diff.y) >= 1) {
		_lastCameraPosition = camXZ;
		_world->scheduleMeshExtraction(camXZ);
	}

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glDisable(GL_DEPTH_TEST);
	//glDisable(GL_CULL_FACE);

	GL_checkError();
}

void Client::beforeUI() {
	UIApp::beforeUI();

	if (_world->isCreated())
		renderMap();
	else
		renderBackground();
}

core::AppState Client::onCleanup() {
	core::AppState state = UIApp::onCleanup();
	// TODO: destroy the gl buffers
	_world->destroy();
	return state;
}

core::AppState Client::onRunning() {
	_timeProvider->update(_now);
	core::AppState state = UIApp::onRunning();
	sendMovement();
	if (state == core::AppState::Running) {
		_posLerp.update(_now);
		glm::vec3 pos = _posLerp.position();
		pos.z += 10.0f;
		_camera.setPosition(pos);
		_network->update();
		_world->onFrame(_deltaFrame);
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
	auto i = _entities.find(id);
	if (i == _entities.end()) {
		Log::error("could not find entity with id %li", id);
		return;
	}
	Log::trace("NPC %li updated at pos %f:%f:%f with orientation %f", id, pos.x, pos.y, pos.z, orientation);
	i->second->lerpPosition(_now, pos, orientation);
}

void Client::npcSpawn(frontend::ClientEntityId id, network::messages::NpcType type, const glm::vec3& pos) {
	auto i = _entities.find(id);
	if (i != _entities.end()) {
		Log::error("NPC %li already spawned", id);
		return;
	}
	Log::info("NPC %li spawned at pos %f:%f:%f (type %i)", id, pos.x, pos.y, pos.z, type);
	const std::string& meshName = core::string::toLower(network::messages::EnumNameNpcType(type));
	_entities[id] = frontend::ClientEntityPtr(new frontend::ClientEntity(id, type, _now, pos, 0.0f, _meshPool->getMesh(meshName)));
}

void Client::userUpdate(const glm::vec3& position) {
	_posLerp.lerpPosition(_now, position);
}

void Client::npcRemove(frontend::ClientEntityId id) {
	auto i = _entities.find(id);
	if (i == _entities.end()) {
		Log::error("NPC %li not found", id);
		return;
	}
	Log::info("NPC %li removed", id);
	_entities.erase(i);
}

void Client::spawn(frontend::ClientEntityId id, const char *name, const glm::vec3& pos) {
	Log::info("User %li (%s) logged in at pos %f:%f:%f", id, name, pos.x, pos.y, pos.z);
	_userId = id;
	_posLerp.setPosition(_now, pos);
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
