/**
 * @file
 */

#include "voxelcollection/CubZHAPI.h"
#include "app/tests/AbstractTest.h"
#include "core/Log.h"
#include "http/HttpCacheStream.h"
#include "io/FilesystemArchive.h"
#include "voxelcollection/Downloader.h"

namespace voxelcollection {

class CubZHAPITest : public app::AbstractTest {};

// disabled because it requires network access
TEST_F(CubZHAPITest, DISABLED_testRepoList) {
	const io::ArchivePtr &archive = io::openFilesystemArchive(_testApp->filesystem(), "", false);
	const auto &sources = cubzh::repoList(archive, "xxx", "yyy");
	ASSERT_FALSE(sources.empty());
	for (const cubzh::TreeEntry &entry : sources) {
		VoxelFile file;
		file.source = "cubzh";
		file.name = entry.repo + "-" + entry.name + ".3zh";
		file.url = entry.url;
		file.fullPath = file.name;
		http::HttpCacheStream stream(archive, file.targetFile(), file.url);
		if (stream.isNewInCache()) {
			Log::debug("Downloaded: %s", file.url.c_str());
		}
	}
}

} // namespace voxelcollection
