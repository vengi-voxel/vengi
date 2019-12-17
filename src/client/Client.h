/**
 * @file
 */

#pragma once

#include "ServerMessages_generated.h"
#include "frontend/ClientEntity.h"
#include "frontend/PlayerMovement.h"
#include "voxelrender/WorldRenderer.h"
#include "voxelrender/PlayerCamera.h"
#include "voxelfont/VoxelFont.h"
#include "core/Var.h"
#include "core/Common.h"
#include "voxelworld/WorldEvents.h"
#include "network/ClientNetwork.h"
#include "network/ClientMessageSender.h"
#include "network/NetworkEvents.h"
#include "ui/turbobadger/UIApp.h"
#include "ui/turbobadger/WaitingMessage.h"
#include "animation/AnimationCache.h"
#include "video/Camera.h"
#include "stock/StockDataProvider.h"

#include <stdlib.h>
#include <SDL.h>

class Client: public ui::turbobadger::UIApp, public core::IEventBusHandler<network::NewConnectionEvent>, public core::IEventBusHandler<
		network::DisconnectEvent>, public core::IEventBusHandler<voxelworld::WorldCreatedEvent> {
protected:
	using Super = ui::turbobadger::UIApp;
	animation::AnimationCachePtr _animationCache;
	network::ClientNetworkPtr _network;
	voxelworld::WorldMgrPtr _worldMgr;
	network::ClientMessageSenderPtr _messageSender;
	voxelrender::WorldRenderer _worldRenderer;
	flatbuffers::FlatBufferBuilder _moveFbb;
	frontend::PlayerMovement _movement;
	network::MoveDirection _lastMoveMask = network::MoveDirection::NONE;
	glm::vec2 _lastMoveAngles {0.0f};
	core::VarPtr _rotationSpeed;
	core::VarPtr _maxTargetDistance;
	frontend::ClientEntityPtr _player;
	voxel::VoxelFont _voxelFont;
	ui::turbobadger::WaitingMessage _waiting;
	stock::StockDataProviderPtr _stockDataProvider;
	voxelformat::VolumeCachePtr _volumeCache;
	voxelrender::PlayerCamera _camera;

	void setState(uint32_t flag);
	bool hasState(uint32_t flag) const;
	void removeState(uint32_t flag);

	frontend::ClientEntityId id() const;

	void sendMovement();
	void handleLogin();
	int renderMap(video::Shader& shader, const voxelworld::WorldMgrPtr& world, const glm::mat4& view, float aspect);
public:
	Client(const metric::MetricPtr& metric, const animation::AnimationCachePtr& animationCache,
			const stock::StockDataProviderPtr& stockDataProvider,
			const network::ClientNetworkPtr& network, const voxelworld::WorldMgrPtr& world,
			const network::ClientMessageSenderPtr& messageSender, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider,
			const io::FilesystemPtr& filesystem, const voxelformat::VolumeCachePtr& volumeCache);
	~Client();

	core::AppState onConstruct() override;
	core::AppState onInit() override;
	core::AppState onRunning() override;
	core::AppState onCleanup() override;
	void beforeUI() override;
	void afterRootWidget() override;
	bool onKeyPress(int32_t key, int16_t modifier) override;
	void onWindowResize(int windowWidth, int windowHeight) override;

	/**
	 * @brief We send the user connect message to the server and we get the seed and a user spawn message back.
	 *
	 * @note If auth failed, we get an auth failed message
	 */
	void onEvent(const network::NewConnectionEvent& event) override;
	void onEvent(const voxelworld::WorldCreatedEvent& event) override;
	void onEvent(const network::DisconnectEvent& event) override;

	bool connect(uint16_t port, const std::string& hostname);
	void authFailed();
	void signup(const std::string& email, const std::string& password);
	void lostPassword(const std::string& email);
	void disconnect();
	/** @brief spawns our own player */
	void spawn(frontend::ClientEntityId id, const char *name, const glm::vec3& pos, float orientation);

	void entitySpawn(frontend::ClientEntityId id, network::EntityType type, float orientation, const glm::vec3& pos, animation::Animation animation);
	void entityUpdate(frontend::ClientEntityId id, const glm::vec3& pos, float orientation, animation::Animation animation);
	void entityRemove(frontend::ClientEntityId id);
	frontend::ClientEntityPtr getEntity(frontend::ClientEntityId id) const;
};

inline frontend::ClientEntityPtr Client::getEntity(frontend::ClientEntityId id) const {
	return _worldRenderer.getEntity(id);
}

inline frontend::ClientEntityId Client::id() const {
	if (!_player) {
		return -1;
	}
	return _player->id();
}

typedef std::shared_ptr<Client> ClientPtr;
