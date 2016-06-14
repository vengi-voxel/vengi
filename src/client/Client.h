/**
 * @file
 */

#pragma once

#include <cstdlib>
#include <SDL.h>

#include "messages/ServerMessages_generated.h"
#include "frontend/WorldShader.h"
#include "frontend/MeshShader.h"
#include "frontend/WaterShader.h"
#include "frontend/WorldInstancedShader.h"
#include "frontend/DeferredDirectionalLight.h"
#include "frontend/ShadowMapShader.h"
#include "frontend/ClientEntity.h"
#include "frontend/WorldRenderer.h"
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
#include "video/MeshPool.h"
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
	frontend::WorldInstancedShader _plantShader;
	frontend::WaterShader _waterShader;
	frontend::MeshShader _meshShader;
	frontend::DeferredDirectionalLight _deferredDirLightShader;
	frontend::ShadowMapShader _shadowMapShader;
	// moving along the y axis should not arise the need to extract new meshes
	video::Camera _camera;
	frontend::WorldRenderer _worldRenderer;
	ENetPeer* _peer = nullptr;
	uint8_t _moveMask = 0;
	uint8_t _lastMoveMask = 0;
	core::VarPtr _rotationSpeed;
	frontend::ClientEntityPtr _player;

	long _lastMovement = 0l;

	int _drawCallsWorld = 0;
	int _drawCallsEntities = 0;

	inline frontend::ClientEntityId id() const {
		if (!_player)
			return -1;
		return _player->id();
	}

	void sendMovement();
	void renderBackground();
	int renderMap(video::Shader& shader, const voxel::WorldPtr& world, const glm::mat4& view, float aspect);
public:
	Client(video::MeshPoolPtr meshPool, network::NetworkPtr network, voxel::WorldPtr world, network::MessageSenderPtr messageSender,
			core::EventBusPtr eventBus, core::TimeProviderPtr timeProvider, io::FilesystemPtr filesystem);
	~Client();

	core::AppState onInit() override;
	core::AppState onRunning() override;
	core::AppState onCleanup() override;
	void beforeUI() override;
	void afterUI() override;

	void onEvent(const voxel::WorldCreatedEvent& event) override;
	void onEvent(const network::DisconnectEvent& event) override;
	/**
	 * @brief We send the user connect message to the server and we get the seed and a user spawn message back.
	 *
	 * @note If auth failed, we get an auth failed message
	 */
	void onEvent(const network::NewConnectionEvent& event) override;
	void onMouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) override;
	void onWindowResize() override;
	bool connect(uint16_t port, const std::string& hostname);
	void authFailed();
	void npcSpawn(frontend::ClientEntityId id, network::messages::NpcType type, const glm::vec3& pos);
	void entityUpdate(frontend::ClientEntityId id, const glm::vec3& pos, float orientation);
	void entityRemove(frontend::ClientEntityId id);
	void disconnect();
	// spawns our own player
	void spawn(frontend::ClientEntityId id, const char *name, const glm::vec3& pos);
};

typedef std::shared_ptr<Client> ClientPtr;
