/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "backend/world/MapProvider.h"

namespace backend {

class MapProviderTest: public core::AbstractTest {
};

TEST_F(MapProviderTest, testInit) {
	core::Var::get(cfg::ServerSeed, "1");
	core::Var::get(cfg::VoxelMeshSize, "16", core::CV_READONLY);
	MapProvider provider(_testApp->filesystem(), _testApp->eventBus());
	ASSERT_TRUE(provider.init()) << "Failed to initialize the map provider";
}

}
