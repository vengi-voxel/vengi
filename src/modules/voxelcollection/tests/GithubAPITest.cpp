/**
 * @file
 */

#include "voxelcollection/GithubAPI.h"
#include "app/tests/AbstractTest.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "io/FilesystemArchive.h"
#include "io/MemoryArchive.h"

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

TEST_F(GithubAPITest, testDownloadUrlPreservesExternalUrlsAndResolvesRepositoryFiles) {
	const io::MemoryArchivePtr archive = io::openMemoryArchive();
	const char *urls[] = {"https://raw.githubusercontent.com/KhronosGroup/glTF-Sample-Models/main/2.0/Sponza/README.md",
						  "http://example.org/license.txt"};
	const core::String metadata = R"({"download_url":"https://example.org/wrong-repository-file"})";
	for (const char *url : urls) {
		core::String cache = core::String("github-org-repo-main-") + url + ".json";
		core::string::replaceAllChars(cache, '/', '-');
		ASSERT_TRUE(archive->add(cache, (const uint8_t *)metadata.c_str(), metadata.size()));
		EXPECT_EQ(url, github::downloadUrl(archive, "org/repo", "main", url, 0));
	}
	const core::String lfsMetadata =
		R"({"download_url":"https://media.githubusercontent.com/media/org/repo/main/tiny.vox"})";
	ASSERT_TRUE(
		archive->add("github-org-repo-main-tiny.vox.json", (const uint8_t *)lfsMetadata.c_str(), lfsMetadata.size()));
	EXPECT_EQ("https://media.githubusercontent.com/media/org/repo/main/tiny.vox",
			  github::downloadUrl(archive, "org/repo", "main", "tiny.vox", 80));
	EXPECT_EQ("https://raw.githubusercontent.com/org/repo/main/models/big%20model.vox",
			  github::downloadUrl(archive, "org/repo", "main", "models/big model.vox", 512));
}

// disabled because it requires network access
TEST_F(GithubAPITest, DISABLED_testReposGitTrees) {
	const io::ArchivePtr &archive = io::openFilesystemArchive(_testApp->filesystem(), "", false);
	const auto &sources = github::reposGitTrees(archive, "vengi-voxel/vengi", "master", "data");
	ASSERT_FALSE(sources.empty());
	validate("data/vengi.pot", "https://raw.githubusercontent.com/vengi-voxel/vengi/master/data/vengi.pot", sources);
	validate("data/voxel/plants/plant4.qb",
			 "https://raw.githubusercontent.com/vengi-voxel/vengi/master/data/voxel/plants/plant4.qb", sources);
}

// disabled because it requires network access
TEST_F(GithubAPITest, DISABLED_testReposGitTreesLFS) {
	const io::ArchivePtr &archive = io::openFilesystemArchive(_testApp->filesystem(), "", false);
	const auto &sources = github::reposGitTrees(archive, "floooh/voxel-data", "master");
	ASSERT_FALSE(sources.empty());
	validate("kc85.vox", "https://media.githubusercontent.com/media/floooh/voxel-data/master/kc85.vox", sources);
}

} // namespace voxelcollection
