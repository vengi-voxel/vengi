/**
 * @file
 */

#include "voxelcollection/GitlabAPI.h"
#include "app/tests/AbstractTest.h"
#include "core/Log.h"
#include "io/FilesystemArchive.h"

namespace voxelcollection {

class GitlabAPITest : public app::AbstractTest {
protected:
	void validate(const core::String &name, const core::String &url,
				  const core::DynamicArray<gitlab::TreeEntry> &entries) {
		auto iter = core::find_if(entries.begin(), entries.end(), [&](const gitlab::TreeEntry &entry) {
			return entry.path == name && entry.url == url;
		});
		EXPECT_NE(iter, entries.end()) << "Could not find " << name << " in the list of entries with " << url;
		if (HasFailure()) {
			for (const auto &entry : entries) {
				Log::error("%s: %s", entry.path.c_str(), entry.url.c_str());
			}
			FAIL();
		}
	}
};

// disabled because it requires network access
TEST_F(GitlabAPITest, DISABLED_testReposGitTrees) {
	const io::ArchivePtr &archive = io::openFilesystemArchive(_testApp->filesystem(), "", false);
	const auto &sources = gitlab::reposGitTrees(archive, "veloren/veloren", "master", "assets/voxygen/voxel/armor");
	ASSERT_FALSE(sources.empty());
	validate("assets/voxygen/voxel/armor/warlord/belt.vox",
			 "https://gitlab.com/veloren/veloren/-/raw/master/assets/voxygen/voxel/armor/warlord/belt.vox", sources);
}

} // namespace voxelcollection
