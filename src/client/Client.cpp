#include "Client.h"
#include "voxel/Spiral.h"
#include "network/messages/ClientMessages.h"
#include "ui/LoginWindow.h"
#include "ui/DisconnectWindow.h"
#include "ui/AuthFailedWindow.h"
#include "ui/HudWindow.h"
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
		UIApp(filesystem, eventBus), _meshPool(meshPool), _network(network), _world(world), _messageSender(messageSender),
		_timeProvider(timeProvider), _worldShader(), _meshShader(new frontend::MeshShader()),
		_userId(-1), _peer(nullptr), _moveMask(0), _lastMovement(0L), _fogRange(0.0f), _viewDistance(0.0f) {
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
	const std::string& email = core::Var::get(cfg::ClientEmail)->strVal();
	const std::string& password = core::Var::get(cfg::ClientPassword)->strVal();
	Log::info("Trying to log into the server with %s", email.c_str());
	_messageSender->sendClientMessage(_peer, fbb, Type_UserConnect,
			CreateUserConnect(fbb, fbb.CreateString(email), fbb.CreateString(password)).Union());
}

void Client::extractMeshAroundCamera(int amount) {
	const int size = _world->getChunkSize();
	glm::ivec2 pos = _lastCameraPosition;
	voxel::Spiral o;
	for (int i = 0; i < amount; ++i) {
		if (!isCulled(pos)) {
			_world->scheduleMeshExtraction(pos);
		}
		o.next();
		pos.x = _lastCameraPosition.x + o.x() * size;
		pos.y = _lastCameraPosition.y + o.y() * size;
	}
}

void Client::onEvent(const voxel::WorldCreatedEvent& event) {
	Log::info("world created");
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
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(typename voxel::DecodedMesh::IndexType), vecIndices, GL_STATIC_DRAW);

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

	_noiseFuture.push_back(_threadPool.enqueue([] () {
		const int ColorTextureSize = 256;
		uint8_t *colorTexture = new uint8_t[ColorTextureSize * ColorTextureSize * 3];
		noise::Simplex::SeamlessNoise2DRGB(colorTexture, ColorTextureSize, 3, 0.3f, 0.7f);
		return NoiseGenerationTask(colorTexture, ColorTextureSize, ColorTextureSize, 3);
	}));
	_colorTexture = video::createTexture("**colortexture**");

	_clearColor = glm::vec3(0.0, 0.6, 0.796);

	_root.SetSkinBg(TBIDC("background"));
	new frontend::LoginWindow(this);

	SDL_GL_SetSwapInterval(core::Var::get(cfg::ClientVSync, "false")->boolVal());

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
	_meshShader->setUniformf("u_fogrange", _fogRange);
	_meshShader->setUniformf("u_viewdistance", _viewDistance);
	_meshShader->setUniformi("u_texture", 0);
	const video::MeshPtr& mesh = _meshPool->getMesh("animal_rabbit");
	if (mesh->initMesh(_meshShader)) {
		const glm::mat4& translate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 20.0f));
		const glm::mat4& scale = glm::scale(translate, glm::vec3(0.01f));
		const glm::mat4& model = glm::rotate(scale, (float) glm::sin(_now / 100L), glm::vec3(0.0, 1.0, 0.0));
		_meshShader->setUniformMatrix("u_model", model, false);
		mesh->render();
		_drawCallsEntities = 0;
	}
	_meshShader->deactivate();
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	GL_checkError();
}

void Client::destroyMeshData(const video::GLMeshData& meshData) {
	_world->allowReExtraction(meshData.translation);
}

bool Client::isCulled(const glm::ivec2& pos) const {
	const glm::ivec2 dist = pos - _lastCameraPosition;
	const int distance = glm::sqrt(dist.x * dist.x + dist.y * dist.y);
	const float cullingThreshold = 10.0f;
	const int maxAllowedDistance = _viewDistance + cullingThreshold;
	if (distance >= maxAllowedDistance) {
		return true;
	}
	return false;
}

void Client::renderMap() {
	voxel::DecodedMeshData mesh;
	if (_world->pop(mesh)) {
		// Now add the mesh to the list of meshes to render.
		addMeshData(createMesh(mesh.mesh, mesh.translation, 1.0f));
	}

	_drawCallsWorld = 0;
	_drawCallsEntities = 0;

	// TODO: use polyvox VolumeResampler to create a minimap of your volume
	// RawVolume<uint8_t> volDataLowLOD(PolyVox::Region(Vector3DInt32(0, 0, 0), Vector3DInt32(15, 31, 31)));
	// VolumeResampler< RawVolume<uint8_t>, RawVolume<uint8_t> > volumeResampler(&volData, PolyVox::Region(Vector3DInt32(0, 0, 0), Vector3DInt32(31, 63, 63)), &volDataLowLOD, volDataLowLOD.getEnclosingRegion());
	// volumeResampler.execute();
	// auto meshLowLOD = extractMarchingCubesMesh(&volDataLowLOD, volDataLowLOD.getEnclosingRegion());
	// auto decodedMeshLowLOD = decodeMesh(meshLowLOD);

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

	_worldShader.activate();
	_worldShader.setUniformMatrix("u_view", view, false);
	_worldShader.setUniformMatrix("u_projection", projection, false);
	_worldShader.setUniformf("u_fogrange", _fogRange);
	_worldShader.setUniformf("u_viewdistance", _viewDistance);
	_worldShader.setUniformi("u_texture", 0);
	_worldShader.setUniformVec3("u_lightpos", _lightPos);
	_colorTexture->bind();
	for (auto i = _meshData.begin(); i != _meshData.end();) {
		const video::GLMeshData& meshData = *i;
		if (isCulled(meshData.translation)) {
			destroyMeshData(meshData);
			i = _meshData.erase(i);
			continue;
		}
		const glm::mat4& model = glm::translate(glm::mat4(1.0f), glm::vec3(meshData.translation.x, 0, meshData.translation.y));
		_worldShader.setUniformMatrix("u_model", model, false);
		glBindVertexArray(meshData.vertexArrayObject);
		glDrawElements(GL_TRIANGLES, meshData.noOfIndices, meshData.indexType, 0);
		GL_checkError();
		++_drawCallsWorld;
		++i;
	}
	GL_checkError();
	_worldShader.deactivate();
	GL_checkError();

	_meshShader->activate();
	_meshShader->setUniformMatrix("u_view", view, false);
	_meshShader->setUniformMatrix("u_projection", projection, false);
	_meshShader->setUniformVec3("u_lightpos", _lightPos);
	_meshShader->setUniformf("u_fogrange", _fogRange);
	_meshShader->setUniformf("u_viewdistance", _viewDistance);
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
		GL_checkError();
		++_drawCallsEntities;
	}
	_meshShader->deactivate();
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	GL_checkError();

	const glm::ivec2& camXZ = _world->getGridPos(_camera.getPosition());
	const glm::vec2 diff = _lastCameraPosition - camXZ;
	if (glm::length(diff.x) >= 1 || glm::length(diff.y) >= 1) {
		_lastCameraPosition = camXZ;
		extractMeshAroundCamera(40);
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

	_drawCallsWorld = 0;
	_drawCallsEntities = 0;

	if (_world->isCreated())
		renderMap();
	else
		renderBackground();
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
	core::AppState state = UIApp::onCleanup();
	// TODO: destroy the gl buffers
	_world->destroy();
	return state;
}

core::AppState Client::onRunning() {
	_timeProvider->update(_now);
	if (!_noiseFuture.empty()) {
		NoiseFuture& future = _noiseFuture.back();
		if (future.valid()) {
			Log::info("Noise texture ready - upload it");
			NoiseGenerationTask c = future.get();
			_colorTexture->upload(c.buffer, c.width, c.height, c.depth);
			_colorTexture->unbind();
			delete[] c.buffer;
			_noiseFuture.erase(_noiseFuture.end());
		}
	}
	core::AppState state = UIApp::onRunning();
	sendMovement();
	if (state == core::AppState::Running) {
		_posLerp.update(_now);
		const glm::vec3& pos = _posLerp.position();
		_camera.setPosition(pos);
		_network->update();
		_world->onFrame(_deltaFrame);
		if (_world->isCreated()) {
			// TODO: properly lerp this
			if (_viewDistance < 500) {
				const int advance = _world->getChunkSize() / 16;
				_viewDistance += advance;
				_fogRange += advance / 2;
			}
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
	auto i = _entities.find(id);
	if (i == _entities.end()) {
		Log::error("could not find npc with id %li", id);
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
	_camera.setPosition(pos);
	_viewDistance = _world->getChunkSize() * 3;
	_fogRange = _viewDistance;
	_lastCameraPosition = _world->getGridPos(pos);
	extractMeshAroundCamera(1000);
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
