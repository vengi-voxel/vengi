/**
 * @file
 */

#include "voxelcollection/GitlabAPI.h"
#include "app/tests/AbstractTest.h"
#include "io/FilesystemArchive.h"

namespace voxelcollection {

class GitlabAPITest : public app::AbstractTest {};

// disabled because it requires network access
TEST_F(GitlabAPITest, DISABLED_testReposGitTrees) {
	const io::ArchivePtr &archive = io::openFilesystemArchive(_testApp->filesystem(), "", false);
	const auto &sources = gitlab::reposGitTrees(archive, "veloren/veloren", "master", "assets/voxygen/voxel/armor");
	ASSERT_FALSE(sources.empty());
}

} // namespace voxelcollection
