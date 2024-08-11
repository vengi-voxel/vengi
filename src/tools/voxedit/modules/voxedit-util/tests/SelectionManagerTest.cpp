/**
 * @file
 */

#include "../modifier/SelectionManager.h"
#include "app/tests/AbstractTest.h"

namespace voxedit {

class SceneManagerTest : public app::AbstractTest {
private:
	using Super = app::AbstractTest;
};

TEST_F(SceneManagerTest, test) {
	SelectionManager mgr;
	EXPECT_FALSE(mgr.hasSelection());
	// TODO: SELECTION: implement test
}

} // namespace voxedit
