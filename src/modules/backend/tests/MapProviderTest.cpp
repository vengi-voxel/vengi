/**
 * @file
 */

#include "core/tests/AbstractTest.h"
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

class MapProviderTest: public core::AbstractTest {
};

TEST_F(MapProviderTest, testInit) {
	core::Var::get(cfg::ServerSeed, "1");
	core::Var::get(cfg::VoxelMeshSize, "16", core::CV_READONLY);
	voxel::initDefaultMaterialColors();
	const EntityStoragePtr& entityStorage = std::make_shared<EntityStorage>(_testApp->eventBus());
	const network::ProtocolHandlerRegistryPtr& protocolHandlerRegistry = std::make_shared<network::ProtocolHandlerRegistry>();
	const network::ServerNetworkPtr& network = std::make_shared<network::ServerNetwork>(protocolHandlerRegistry, _testApp->eventBus());
	const network::ServerMessageSenderPtr& messageSender = std::make_shared<network::ServerMessageSender>(network);
	const AIRegistryPtr& registry = std::make_shared<AIRegistry>();
	registry->init();
	const AILoaderPtr& loader = std::make_shared<AILoader>(registry);
	const attrib::ContainerProviderPtr& containerProvider = std::make_shared<attrib::ContainerProvider>();
	const cooldown::CooldownProviderPtr& cooldownProvider = std::make_shared<cooldown::CooldownProvider>();

	MapProvider provider(_testApp->filesystem(), _testApp->eventBus(), _testApp->timeProvider(),
			entityStorage, messageSender, loader, containerProvider, cooldownProvider);
	ASSERT_TRUE(provider.init()) << "Failed to initialize the map provider";
}

}
