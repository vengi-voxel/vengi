/**
 * @file
 */

#pragma once

#include "ServerMessages_generated.h"
#include "frontend/ClientEntity.h"
#include "frontend/PlayerMovement.h"
#include "frontend/PlayerAction.h"
#include "voxelworldrender/WorldRenderer.h"
#include "voxelworldrender/PlayerCamera.h"
#include "voxelformat/MeshCache.h"
#include "core/Var.h"
#include "core/Common.h"
#include "audio/SoundManager.h"
#include "voxelworld/WorldEvents.h"
#include "voxelworld/WorldPager.h"
#include "voxelworld/CachedFloorResolver.h"
#include "network/ClientNetwork.h"
#include "network/ClientMessageSender.h"
#include "network/NetworkEvents.h"
#include "ui/nuklear/LUAUIApp.h"
#include "animation/AnimationCache.h"
#include "video/Camera.h"
#include "stock/StockDataProvider.h"
#include "voxel/ClientPager.h"
#include "cooldown/CooldownHandler.h"

class Client: public ui::nuklear::LUAUIApp, public core::IEventBusHandler<network::NewConnectionEvent>, public core::IEventBusHandler<
		network::DisconnectEvent>, public core::IEventBusHandler<voxelworld::WorldCreatedEvent> {
protected:
	using Super = ui::nuklear::LUAUIApp;
	animation::AnimationCachePtr _animationCache;
	network::ClientNetworkPtr _network;
	voxelworld::WorldMgrPtr _worldMgr;
	voxelworld::CachedFloorResolver _floorResolver;
	client::ClientPagerPtr _clientPager;
	network::ClientMessageSenderPtr _messageSender;
	voxelworldrender::WorldRenderer _worldRenderer;
	flatbuffers::FlatBufferBuilder _moveFbb;
	frontend::PlayerMovement _movement;
	flatbuffers::FlatBufferBuilder _actionFbb;
	frontend::PlayerAction _action;
	client::CooldownHandler _cooldownHandler;
	network::MoveDirection _lastMoveMask = network::MoveDirection::NONE;
	glm::vec2 _lastMoveAngles {0.0f};
	core::VarPtr _rotationSpeed;
	core::VarPtr _chunkUrl;
	core::VarPtr _seed;
	frontend::ClientEntityPtr _player;
	stock::StockDataProviderPtr _stockDataProvider;
	voxelformat::VolumeCachePtr _volumeCache;
	voxelformat::MeshCachePtr _meshCache;
	voxelworldrender::PlayerCamera _camera;
	audio::SoundManagerPtr _soundManager;
	voxelworldrender::AssetVolumeCachePtr _assetVolumeCache;

	frontend::ClientEntityId id() const;

	void sendVars() const;
	void sendMovement();
	void sendTriggerAction();

	void beforeUI() override;
	void configureLUA(lua::LUA& lua) override;
	void initUIConfig(struct nk_convert_config& config) override;
	void initUISkin() override;

	int renderMap(video::Shader& shader, const voxelworld::WorldMgrPtr& world, const glm::mat4& view, float aspect);
public:
	Client(const metric::MetricPtr& metric, const animation::AnimationCachePtr& animationCache,
			const stock::StockDataProviderPtr& stockDataProvider,
			const network::ClientNetworkPtr& network, const voxelworld::WorldMgrPtr& world,
			const client::ClientPagerPtr& worldPager,
			const network::ClientMessageSenderPtr& messageSender, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider,
			const io::FilesystemPtr& filesystem, const voxelformat::VolumeCachePtr& volumeCache,
			const voxelformat::MeshCachePtr& meshCache,
			const video::TexturePoolPtr& texturePool,
			const voxelrender::CachedMeshRendererPtr& meshRenderer,
			const video::TextureAtlasRendererPtr& textureAtlasRenderer,
			const audio::SoundManagerPtr& soundManager,
			const voxelworldrender::AssetVolumeCachePtr& assetVolumeCache);
	~Client();

	app::AppState onConstruct() override;
	app::AppState onInit() override;
	app::AppState onRunning() override;
	app::AppState onCleanup() override;
	void onWindowResize(int windowWidth, int windowHeight) override;

	client::CooldownHandler& cooldownHandler();

	/**
	 * @brief We send the user connect message to the server and we get the seed and a user spawn message back.
	 *
	 * @note If auth failed, we get an auth failed message
	 */
	void onEvent(const network::NewConnectionEvent& event) override;
	void onEvent(const voxelworld::WorldCreatedEvent& event) override;
	void onEvent(const network::DisconnectEvent& event) override;

	bool isConnected() const;
	bool isConnecting() const;
	bool connect(uint16_t port, const core::String& hostname);
	void disconnect();
	bool auth(const core::String &email, const core::String &password);
	void authFailed();
	bool signup(const core::String &email, const core::String &password);
	bool validate(const core::String &email, const core::String &token);
	void validationState(bool state);

	/** @brief spawn our own player */
	void spawn(frontend::ClientEntityId id, const char *name, const glm::vec3& pos, float orientation);

	void entitySpawn(frontend::ClientEntityId id, network::EntityType type, float orientation, const glm::vec3& pos, animation::Animation animation);
	void entityRemove(frontend::ClientEntityId id);
	frontend::ClientEntityPtr getEntity(frontend::ClientEntityId id) const;
};

inline client::CooldownHandler& Client::cooldownHandler() {
	return _cooldownHandler;
}

typedef std::shared_ptr<Client> ClientPtr;
