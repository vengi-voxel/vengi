/**
 * @file
 */

#include "voxedit-util/VoxBoxApi.h"
#include "app/tests/AbstractTest.h"

namespace voxedit {

class VoxBoxApiTest : public app::AbstractTest {};

TEST_F(VoxBoxApiTest, DISABLED_testSearch) {
	VoxBoxApi api;
	VoxBoxState results = api.search();
	EXPECT_FALSE(results.info.empty());
}

} // namespace voxedit
