/**
 * @file
 */

#include "voxbrowser-util/GithubAPI.h"
#include "app/tests/AbstractTest.h"

namespace voxbrowser {

class GithubAPITest : public app::AbstractTest {};

TEST_F(GithubAPITest, DISABLE_testReposGitTrees) {
	const auto &sources = github::reposGitTrees(_testApp->filesystem(), "vengi-voxel/voxelized", "main");
	ASSERT_FALSE(sources.empty());
}

} // namespace voxbrowser
