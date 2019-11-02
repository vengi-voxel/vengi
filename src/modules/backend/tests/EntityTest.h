/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "backend/entity/ai/AIRegistry.h"
#include "backend/world/MapProvider.h"
#include "backend/world/Map.h"
#include "backend/spawn/SpawnMgr.h"
#include "poi/PoiProvider.h"
#include "network/ProtocolHandlerRegistry.h"
#include "network/ServerNetwork.h"
#include "network/ServerMessageSender.h"
#include "cooldown/CooldownProvider.h"
#include "attrib/ContainerProvider.h"
#include "backend/entity/ai/AIRegistry.h"
#include "backend/entity/ai/AILoader.h"
#include "backend/entity/EntityStorage.h"
#include "voxel/MaterialColor.h"
#include "voxelformat/VolumeCache.h"
#include "persistence/tests/Mocks.h"

#pragma once

namespace backend {
namespace {

const char *CONTAINER = R"(function init()
local player = attrib.createContainer("PLAYER")
player:absolute("FIELDOFVIEW", 360.0)
player:absolute("HEALTH", 100.0)
player:absolute("STRENGTH", 1.0)
player:absolute("VIEWDISTANCE", 10000.0)
player:register()

local rabbit = attrib.createContainer("ANIMAL_RABBIT")
rabbit:absolute("FIELDOFVIEW", 360.0)
rabbit:absolute("HEALTH", 100.0)
rabbit:absolute("STRENGTH", 1.0)
rabbit:absolute("VIEWDISTANCE", 10000.0)
rabbit:register()

local wolf = attrib.createContainer("ANIMAL_WOLF")
wolf:absolute("FIELDOFVIEW", 360.0)
wolf:absolute("HEALTH", 100.0)
wolf:absolute("STRENGTH", 1.0)
wolf:absolute("VIEWDISTANCE", 10000.0)
wolf:register()
end)";

}

class EntityTest: public core::AbstractTest {
private:
	using Super = core::AbstractTest;
protected:
	EntityStoragePtr entityStorage;
	network::ProtocolHandlerRegistryPtr protocolHandlerRegistry;
	network::ServerNetworkPtr network;
	network::ServerMessageSenderPtr messageSender;
	AIRegistryPtr registry;
	AILoaderPtr loader;
	attrib::ContainerProviderPtr containerProvider;
	cooldown::CooldownProviderPtr cooldownProvider;
	core::EventBusPtr eventBus;
	io::FilesystemPtr filesystem;
	core::TimeProviderPtr timeProvider;
	std::shared_ptr<persistence::PersistenceMgrMock> persistenceMgr;
	MapProviderPtr mapProvider;
	MapPtr map;

	void SetUp() override {
		Super::SetUp();
		core::Var::get(cfg::ServerSeed, "1");
		core::Var::get(cfg::VoxelMeshSize, "16", core::CV_READONLY);
		voxel::initDefaultMaterialColors();
		entityStorage = std::make_shared<EntityStorage>(_testApp->eventBus());
		protocolHandlerRegistry = std::make_shared<network::ProtocolHandlerRegistry>();
		network = std::make_shared<network::ServerNetwork>(protocolHandlerRegistry, _testApp->eventBus());
		messageSender = std::make_shared<network::ServerMessageSender>(network);
		registry = std::make_shared<AIRegistry>();
		registry->init();
		loader = std::make_shared<AILoader>(registry);
		containerProvider = std::make_shared<attrib::ContainerProvider>();
		ASSERT_TRUE(containerProvider->init(CONTAINER));
		cooldownProvider = std::make_shared<cooldown::CooldownProvider>();
		filesystem = _testApp->filesystem();
		eventBus = _testApp->eventBus();
		voxelformat::VolumeCachePtr volumeCache = std::make_shared<voxelformat::VolumeCache>();
		timeProvider = _testApp->timeProvider();
		persistenceMgr = std::make_shared<persistence::PersistenceMgrMock>();
		EXPECT_CALL(*persistenceMgr, registerSavable(testing::_, testing::_)).WillRepeatedly(testing::Return(true));
		EXPECT_CALL(*persistenceMgr, unregisterSavable(testing::_, testing::_)).WillRepeatedly(testing::Return(true));
		testing::Mock::AllowLeak(persistenceMgr.get());
		mapProvider = std::make_shared<MapProvider>(filesystem, eventBus, timeProvider,
				entityStorage, messageSender, loader, containerProvider, cooldownProvider, persistenceMgr, volumeCache);
		ASSERT_TRUE(mapProvider->init()) << "Failed to initialize the map provider";
		map = mapProvider->map(1);
	}
};

}
