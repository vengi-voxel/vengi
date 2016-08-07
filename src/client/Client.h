/**
 * @file
 */

#pragma once

#include <cstdlib>
#include <SDL.h>

#include "ServerMessages_generated.h"
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
#include "ui/WaitingMessage.h"
#include "video/MeshPool.h"
#include "video/Camera.h"

// client states
constexpr uint32_t CLIENT_CONNECTING = 1 << 0;

class Client: public ui::UIApp, public core::IEventBusHandler<network::NewConnectionEvent>, public core::IEventBusHandler<
		network::DisconnectEvent>, public core::IEventBusHandler<voxel::WorldCreatedEvent> {
protected:
	using Super = ui::UIApp;
	video::Camera _camera;
	video::MeshPoolPtr _meshPool;
	network::NetworkPtr _network;
	voxel::WorldPtr _world;
	network::MessageSenderPtr _messageSender;
	core::TimeProviderPtr _timeProvider;
	frontend::WorldRenderer _worldRenderer;
	ENetPeer* _peer = nullptr;
	flatbuffers::FlatBufferBuilder _moveFbb;
	MoveDirection _moveMask = MoveDirection::NONE;
	MoveDirection _lastMoveMask = MoveDirection::NONE;
	core::VarPtr _rotationSpeed;
	core::VarPtr _maxTargetDistance;
	frontend::ClientEntityPtr _player;
	ui::WaitingMessage _waiting;

	long _lastMovement = 0l;

	int _drawCallsWorld = 0;
	int _drawCallsEntities = 0;

	uint32_t _state = 0u;

	inline void setState(uint32_t flag) {
		_state |= flag;
	}

	inline bool hasState(uint32_t flag) const {
		return (_state & flag) != 0;
	}

	inline void removeState(uint32_t flag) {
		_state &= ~flag;
	}

	inline frontend::ClientEntityId id() const {
		if (!_player) {
			return -1;
		}
		return _player->id();
	}

	void sendMovement();
	void handleLogin();
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

	bool onKeyPress(int32_t key, int16_t modifier) override;
	void onMouseWheel(int32_t x, int32_t y) override;

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
	void signup(const std::string& email, const std::string& password);
	void lostPassword(const std::string& email);

	void entitySpawn(frontend::ClientEntityId id, network::messages::EntityType type, float orientation, const glm::vec3& pos);
	void entityUpdate(frontend::ClientEntityId id, const glm::vec3& pos, float orientation);
	void entityRemove(frontend::ClientEntityId id);
	frontend::ClientEntityPtr getEntity(frontend::ClientEntityId id) const;

	void disconnect();
	// spawns our own player
	void spawn(frontend::ClientEntityId id, const char *name, const glm::vec3& pos, float orientation);
};

typedef std::shared_ptr<Client> ClientPtr;
