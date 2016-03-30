#pragma once

#include <cstdlib>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <chrono>
#include <mutex>
#include <unordered_set>
#include <unordered_map>
#include <SDL.h>

#include "network/messages/ServerMessages.h"
#include "frontend/WorldShader.h"
#include "frontend/MeshShader.h"
#include "frontend/WaterShader.h"
#include "frontend/ClientEntity.h"
#include "util/PosLerp.h"
#include "core/Var.h"
#include "core/Common.h"
#include "core/EventBus.h"
#include "core/TimeProvider.h"
#include "voxel/WorldEvents.h"
#include "network/Network.h"
#include "network/MessageSender.h"
#include "network/NetworkEvents.h"
#include "ui/UIApp.h"
#include "video/GLMeshData.h"
#include "video/MeshPool.h"
#include "video/GLFunc.h"
#include "video/Shader.h"
#include "video/VertexBuffer.h"
#include "video/Camera.h"

class Client: public ui::UIApp, public core::IEventBusHandler<network::NewConnectionEvent>, public core::IEventBusHandler<
		network::DisconnectEvent>, public core::IEventBusHandler<voxel::WorldCreatedEvent> {
protected:
	video::MeshPoolPtr _meshPool;
	network::NetworkPtr _network;
	voxel::WorldPtr _world;
	network::MessageSenderPtr _messageSender;
	core::TimeProviderPtr _timeProvider;
	frontend::WorldShader _worldShader;
	frontend::MeshShaderPtr _meshShader;
	frontend::WaterShaderPtr _waterShader;
	video::TexturePtr _waterTexture;
	video::GLMeshData _waterData;
	video::Camera _camera;
	glm::vec3 _lightPos = glm::vec3(1.0, 1.0, 1.0);
	glm::vec3 _diffuseColor = glm::vec3(0.1, 0.1, 0.1);
	glm::vec3 _specularColor = glm::vec3(0.0, 0.0, 0.0);
	// the position of the last extraction - we only care for x and z here
	// moving along the y axis should not arise the need to extract new meshes
	glm::ivec2 _lastCameraPosition;
	// Index/vertex buffer data
	std::vector<video::GLMeshData> _meshData;
	frontend::ClientEntityId _userId;
	ENetPeer* _peer;
	uint8_t _moveMask;
	typedef std::unordered_map<frontend::ClientEntityId, frontend::ClientEntityPtr> Entities;
	Entities _entities;

	util::PosLerp _posLerp;
	long _lastMovement;

	inline frontend::ClientEntityId id() const {
		return _userId;
	}

	// Convert a PolyVox mesh to OpenGL index/vertex buffers.
	video::GLMeshData createMesh(voxel::DecodedMesh& surfaceMesh, const glm::ivec2& translation, float scale);

	void addMeshData(video::GLMeshData meshData);
	void sendMovement();

	void renderBackground();
	void renderMap();
public:
	Client(video::MeshPoolPtr meshPool, network::NetworkPtr network, voxel::WorldPtr world, network::MessageSenderPtr messageSender,
			core::EventBusPtr eventBus, core::TimeProviderPtr timeProvider, io::FilesystemPtr filesystem);
	~Client();

	core::AppState onInit() override;
	core::AppState onRunning() override;
	core::AppState onCleanup() override;
	void beforeUI() override;

	void onEvent(const voxel::WorldCreatedEvent& event);
	void onEvent(const network::DisconnectEvent& event);
	/**
	 * @brief We send the user connect message to the server and we get the seed and a user spawn message back.
	 *
	 * @note If auth failed, we get an auth failed message
	 */
	void onEvent(const network::NewConnectionEvent& event);
	void onMouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) override;
	bool connect(uint16_t port, const std::string& hostname);
	void authFailed();
	void npcSpawn(frontend::ClientEntityId id, network::messages::NpcType type, const glm::vec3& pos);
	void npcUpdate(frontend::ClientEntityId id, const glm::vec3& pos, float orientation);
	void npcRemove(frontend::ClientEntityId id);
	void userUpdate(const glm::vec3& position);
	void disconnect();
	// spawns our own player
	void spawn(frontend::ClientEntityId id, const char *name, const glm::vec3& pos);
};

typedef std::shared_ptr<Client> ClientPtr;

inline void Client::addMeshData(video::GLMeshData meshData) {
	_meshData.push_back(meshData);
}
