/**
 * @file
 */

#include "voxelcollection/CubZHAPI.h"
#include "app/tests/AbstractTest.h"
#include "core/Log.h"

namespace voxelcollection {

class CubZHAPITest : public app::AbstractTest {};

// disabled because it requires network access
TEST_F(CubZHAPITest, DISABLED_testRepoList) {
	const auto &sources = cubzh::repoList(_testApp->filesystem(), "xxx", "yyy");
	ASSERT_FALSE(sources.empty());
}

} // namespace voxelcollection
