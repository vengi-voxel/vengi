/**
 * @file
 */

#include "voxelcollection/GithubAPI.h"
#include "app/tests/AbstractTest.h"
#include "core/Log.h"
#include "io/FilesystemArchive.h"

namespace voxelcollection {

class GithubAPITest : public app::AbstractTest {
protected:
	void validate(const core::String &name, const core::String &url,
				  const core::DynamicArray<github::TreeEntry> &entries) {
		auto iter = core::find_if(entries.begin(), entries.end(), [&](const github::TreeEntry &entry) {
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
TEST_F(GithubAPITest, DISABLED_testReposGitTrees) {
	const io::ArchivePtr &archive = io::openFilesystemArchive(_testApp->filesystem(), "", false);
	const auto &sources = github::reposGitTrees(archive, "vengi-voxel/vengi", "master", "data");
	ASSERT_FALSE(sources.empty());
	validate("data/vengi.pot", "https://raw.githubusercontent.com/vengi-voxel/vengi/master/data/vengi.pot", sources);
	validate("data/voxel/plants/plant4.qb",
			 "https://raw.githubusercontent.com/vengi-voxel/vengi/master/data/voxel/plants/plant4.qb", sources);
}

} // namespace voxelcollection
