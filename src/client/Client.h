/**
 * @file
 */

#pragma once

#include "ServerMessages_generated.h"
#include "frontend/ClientEntity.h"
#include "frontend/Movement.h"
#include "voxelrender/WorldRenderer.h"
#include "voxelfont/VoxelFont.h"
#include "util/PosLerp.h"
#include "core/Var.h"
#include "core/Common.h"
#include "voxel/WorldEvents.h"
#include "network/ClientNetwork.h"
#include "network/ClientMessageSender.h"
#include "network/NetworkEvents.h"
#include "ui/turbobadger/UIApp.h"
#include "ui/turbobadger/WaitingMessage.h"
#include "video/MeshPool.h"
#include "video/Camera.h"

#include <stdlib.h>
#include <SDL.h>

// client states
constexpr uint32_t CLIENT_CONNECTING = 1 << 0;

class Client: public ui::turbobadger::UIApp, public core::IEventBusHandler<network::NewConnectionEvent>, public core::IEventBusHandler<
		network::DisconnectEvent>, public core::IEventBusHandler<voxel::WorldCreatedEvent> {
protected:
	using Super = ui::turbobadger::UIApp;
	video::Camera _camera;
	video::MeshPoolPtr _meshPool;
	network::ClientNetworkPtr _network;
	voxel::WorldMgrPtr _world;
	network::ClientMessageSenderPtr _messageSender;
	voxelrender::WorldRenderer _worldRenderer;
	flatbuffers::FlatBufferBuilder _moveFbb;
	frontend::Movement _movement;
	network::MoveDirection _lastMoveMask = network::MoveDirection::NONE;
	core::VarPtr _rotationSpeed;
	core::VarPtr _maxTargetDistance;
	frontend::ClientEntityPtr _player;
	voxel::VoxelFont _voxelFont;
	ui::turbobadger::WaitingMessage _waiting;

	uint64_t _lastMovement = 0l;

	int _drawCallsWorld = 0;
	int _drawCallsEntities = 0;

	uint32_t _state = 0u;

	void setState(uint32_t flag);
	bool hasState(uint32_t flag) const;
	void removeState(uint32_t flag);

	frontend::ClientEntityId id() const;

	void sendMovement();
	void handleLogin();
	int renderMap(video::Shader& shader, const voxel::WorldMgrPtr& world, const glm::mat4& view, float aspect);
public:
	Client(const metric::MetricPtr& metric, const video::MeshPoolPtr& meshPool, const network::ClientNetworkPtr& network, const voxel::WorldMgrPtr& world,
			const network::ClientMessageSenderPtr& messageSender, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider,
			const io::FilesystemPtr& filesystem);
	~Client();

	core::AppState onConstruct() override;
	core::AppState onInit() override;
	core::AppState onRunning() override;
	core::AppState onCleanup() override;
	void beforeUI() override;
	void afterRootWidget() override;
	bool onKeyPress(int32_t key, int16_t modifier) override;
	bool onMouseWheel(int32_t x, int32_t y) override;
	void onWindowResize(int windowWidth, int windowHeight) override;

	/**
	 * @brief We send the user connect message to the server and we get the seed and a user spawn message back.
	 *
	 * @note If auth failed, we get an auth failed message
	 */
	void onEvent(const network::NewConnectionEvent& event) override;
	void onEvent(const voxel::WorldCreatedEvent& event) override;
	void onEvent(const network::DisconnectEvent& event) override;

	bool connect(uint16_t port, const std::string& hostname);
	void authFailed();
	void signup(const std::string& email, const std::string& password);
	void lostPassword(const std::string& email);
	void disconnect();
	/** @brief spawns our own player */
	void spawn(frontend::ClientEntityId id, const char *name, const glm::vec3& pos, float orientation);

	void entitySpawn(frontend::ClientEntityId id, network::EntityType type, float orientation, const glm::vec3& pos);
	void entityUpdate(frontend::ClientEntityId id, const glm::vec3& pos, float orientation);
	void entityRemove(frontend::ClientEntityId id);
	frontend::ClientEntityPtr getEntity(frontend::ClientEntityId id) const;
};

inline frontend::ClientEntityPtr Client::getEntity(frontend::ClientEntityId id) const {
	return _worldRenderer.getEntity(id);
}

inline void Client::setState(uint32_t flag) {
	_state |= flag;
}

inline bool Client::hasState(uint32_t flag) const {
	return (_state & flag) != 0;
}

inline void Client::removeState(uint32_t flag) {
	_state &= ~flag;
}

inline frontend::ClientEntityId Client::id() const {
	if (!_player) {
		return -1;
	}
	return _player->id();
}

typedef std::shared_ptr<Client> ClientPtr;
