/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "backend/world/Map.h"
#include "network/ProtocolHandlerRegistry.h"
#include "backend/network/ServerNetwork.h"
#include "backend/network/ServerMessageSender.h"
#include "cooldown/CooldownProvider.h"
#include "attrib/ContainerProvider.h"
#include "backend/entity/ai/AIRegistry.h"
#include "backend/entity/ai/AILoader.h"
#include "backend/entity/EntityStorage.h"
#include "voxel/MaterialColor.h"
#include "voxelformat/VolumeCache.h"
#include "persistence/tests/Mocks.h"

namespace backend {

class MapTest: public app::AbstractTest {
public:
	EntityStoragePtr _entityStorage;
	network::ProtocolHandlerRegistryPtr _protocolHandlerRegistry;
	network::ServerNetworkPtr _network;
	network::ServerMessageSenderPtr _messageSender;
	AILoaderPtr _loader;
	attrib::ContainerProviderPtr _containerProvider;
	cooldown::CooldownProviderPtr _cooldownProvider;
	voxelformat::VolumeCachePtr _volumeCache;
	persistence::PersistenceMgrPtr _persistenceMgr;
	persistence::DBHandlerPtr _dbHandler;

	void SetUp() override {
		app::AbstractTest::SetUp();
		core::Var::get(cfg::ServerSeed, "1");
		core::Var::get(cfg::VoxelMeshSize, "16", core::CV_READONLY);
		voxel::initDefaultMaterialColors();
		_entityStorage = std::make_shared<EntityStorage>(_testApp->eventBus());
		ASSERT_TRUE(_entityStorage->init());
		_protocolHandlerRegistry = std::make_shared<network::ProtocolHandlerRegistry>();
		_network = std::make_shared<network::ServerNetwork>(_protocolHandlerRegistry, _testApp->eventBus(), _testApp->metric());
		_messageSender = std::make_shared<network::ServerMessageSender>(_network, _testApp->metric());
		const AIRegistryPtr& registry = std::make_shared<LUAAIRegistry>();
		ASSERT_TRUE(registry->init());
		_loader = std::make_shared<AILoader>(registry);
		_containerProvider = core::make_shared<attrib::ContainerProvider>();
		_cooldownProvider = std::make_shared<cooldown::CooldownProvider>();
		_volumeCache = std::make_shared<voxelformat::VolumeCache>();
		_persistenceMgr = persistence::createPersistenceMgrMock();
		_dbHandler = persistence::createDbHandlerMock();
	}

	void TearDown() override {
		_entityStorage->shutdown();
		_protocolHandlerRegistry->shutdown();
		_network->shutdown();
		_loader->shutdown();
		_volumeCache->shutdown();

		_entityStorage.reset();
		_protocolHandlerRegistry.reset();
		_network.reset();
		_messageSender.reset();
		_loader.reset();
		_containerProvider.release();
		_cooldownProvider.reset();
		_volumeCache.reset();
		_persistenceMgr.reset();
		_dbHandler.reset();

		app::AbstractTest::TearDown();
	}
};

#define create(name, id) \
	Map name(id, _testApp->eventBus(), _testApp->timeProvider(), _testApp->filesystem(), _entityStorage, \
			_messageSender, _volumeCache, _loader, _containerProvider, _cooldownProvider, _persistenceMgr, \
			std::make_shared<DBChunkPersister>(_dbHandler, id));

TEST_F(MapTest, testInitShutdown) {
	create(map, 1);
	EXPECT_TRUE(map.init()) << "Failed to initialize the map " << map.id();
	map.shutdown();
}

TEST_F(MapTest, testUpdate) {
	create(map, 1);
	EXPECT_TRUE(map.init()) << "Failed to initialize the map " << map.id();
	map.update(0ul);
	map.shutdown();
}

#undef create

}
