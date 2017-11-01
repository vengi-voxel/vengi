/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "backend/world/Map.h"

namespace backend {

class MapTest: public core::AbstractTest {
};

TEST_F(MapTest, testInitShutdown) {
	core::Var::get(cfg::ServerSeed, "1");
	core::Var::get(cfg::VoxelMeshSize, "16", core::CV_READONLY);
	Map map(1, _testApp->eventBus());
	ASSERT_TRUE(map.init(_testApp->filesystem())) << "Failed to initalize the map " << map.id();
	map.shutdown();
}

}
