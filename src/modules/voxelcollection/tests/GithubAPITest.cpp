/**
 * @file
 */

#include "voxelcollection/GithubAPI.h"
#include "app/tests/AbstractTest.h"

namespace voxelcollection {

class GithubAPITest : public app::AbstractTest {};

TEST_F(GithubAPITest, DISABLED_testReposGitTrees) {
	const auto &sources = github::reposGitTrees(_testApp->filesystem(), "vengi-voxel/voxelized", "main");
	ASSERT_FALSE(sources.empty());
}

} // namespace voxelcollection
