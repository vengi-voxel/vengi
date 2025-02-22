/**
 * @file
 */

#include "../modifier/SelectionManager.h"
#include "app/tests/AbstractTest.h"
#include "voxel/RawVolume.h"

namespace voxedit {

class SelectionManagerTest : public app::AbstractTest {
private:
	using Super = app::AbstractTest;
};

TEST_F(SelectionManagerTest, testSelectAndInvert) {
	voxel::Region region(0, 16);
	voxel::RawVolume volume(region);
	SelectionManager mgr;
	EXPECT_FALSE(mgr.hasSelection());
	EXPECT_TRUE(mgr.select(volume, glm::ivec3(4), glm::ivec3(12)));
	EXPECT_TRUE(mgr.hasSelection());
	mgr.invert(volume);
	EXPECT_EQ(6u, mgr.selections().size());
}

} // namespace voxedit
