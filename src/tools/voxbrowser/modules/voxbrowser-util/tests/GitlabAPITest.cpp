/**
 * @file
 */

#include "voxbrowser-util/GitlabAPI.h"
#include "app/tests/AbstractTest.h"

namespace voxbrowser {

class GitlabAPITest : public app::AbstractTest {};

TEST_F(GitlabAPITest, testReposGitTrees) {
	const auto &sources = gitlab::reposGitTrees("veloren/veloren", "master", "assets/voxygen/voxel/armor");
	ASSERT_FALSE(sources.empty());
}

} // namespace voxbrowser
