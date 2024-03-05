/**
 * @file
 */

#include "voxbrowser-util/GitlabAPI.h"
#include "app/tests/AbstractTest.h"

namespace voxbrowser {

class GitlabAPITest : public app::AbstractTest {};

TEST_F(GitlabAPITest, DISABLED_testReposGitTrees) {
	const auto &sources = gitlab::reposGitTrees(_testApp->filesystem(), "veloren/veloren", "master", "assets/voxygen/voxel/armor");
	ASSERT_FALSE(sources.empty());
}

} // namespace voxbrowser
