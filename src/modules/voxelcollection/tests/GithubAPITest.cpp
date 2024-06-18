/**
 * @file
 */

#include "voxelcollection/GithubAPI.h"
#include "app/tests/AbstractTest.h"
#include "io/FilesystemArchive.h"

namespace voxelcollection {

class GithubAPITest : public app::AbstractTest {};

// disabled because it requires network access
TEST_F(GithubAPITest, DISABLED_testReposGitTrees) {
	const io::ArchivePtr &archive = io::openFilesystemArchive(_testApp->filesystem(), "", false);
	const auto &sources = github::reposGitTrees(archive, "vengi-voxel/voxelized", "main");
	ASSERT_FALSE(sources.empty());
}

} // namespace voxelcollection
