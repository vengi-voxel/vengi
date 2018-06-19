/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "backend/world/World.h"
#include "backend/world/MapProvider.h"
#include "network/ProtocolHandlerRegistry.h"
#include "network/ServerNetwork.h"
#include "network/ServerMessageSender.h"
#include "cooldown/CooldownProvider.h"
#include "attrib/ContainerProvider.h"
#include "backend/entity/ai/AIRegistry.h"
#include "backend/entity/ai/AILoader.h"
#include "backend/entity/EntityStorage.h"
#include "voxel/MaterialColor.h"

namespace backend {

class WorldTest: public core::AbstractTest {
public:
	EntityStoragePtr _entityStorage;
	network::ProtocolHandlerRegistryPtr _protocolHandlerRegistry;
	network::ServerNetworkPtr _network;
	network::ServerMessageSenderPtr _messageSender;
	AILoaderPtr _loader;
	attrib::ContainerProviderPtr _containerProvider;
	cooldown::CooldownProviderPtr _cooldownProvider;
	AIRegistryPtr _aiRegistry;
	MapProviderPtr _mapProvider;

	void SetUp() override {
		core::AbstractTest::SetUp();
		core::Var::get(cfg::ServerSeed, "1");
		core::Var::get(cfg::VoxelMeshSize, "16", core::CV_READONLY);
		voxel::initDefaultMaterialColors();
		_entityStorage = std::make_shared<EntityStorage>(_testApp->eventBus());
		_protocolHandlerRegistry = std::make_shared<network::ProtocolHandlerRegistry>();
		_network = std::make_shared<network::ServerNetwork>(_protocolHandlerRegistry, _testApp->eventBus());
		_messageSender = std::make_shared<network::ServerMessageSender>(_network);
		_aiRegistry = std::make_shared<AIRegistry>();
		_aiRegistry->init();
		_loader = std::make_shared<AILoader>(_aiRegistry);
		_containerProvider = std::make_shared<attrib::ContainerProvider>();
		const std::string& attributes = _testApp->filesystem()->load("test-attributes.lua");
		ASSERT_TRUE(_containerProvider->init(attributes)) << _containerProvider->error();
		_cooldownProvider = std::make_shared<cooldown::CooldownProvider>();
		_mapProvider = std::make_shared<MapProvider>(_testApp->filesystem(), _testApp->eventBus(), _testApp->timeProvider(),
				_entityStorage, _messageSender, _loader, _containerProvider, _cooldownProvider);
	}
};

#define create(name) \
	World name(_mapProvider, _aiRegistry, _testApp->eventBus(), _testApp->filesystem());

TEST_F(WorldTest, testInitShutdown) {
	create(world);
	ASSERT_TRUE(world.init());
	world.shutdown();
}

TEST_F(WorldTest, testUpdate) {
	create(world);
	ASSERT_TRUE(world.init());
	world.update(0ul);
	world.shutdown();
}

}
