/**
 * @file
 */

#include "voxelcollection/CubZHAPI.h"
#include "app/tests/AbstractTest.h"
#include "core/Log.h"
#include "http/HttpCacheStream.h"
#include "voxelcollection/Downloader.h"

namespace voxelcollection {

class CubZHAPITest : public app::AbstractTest {};

// disabled because it requires network access
TEST_F(CubZHAPITest, DISABLED_testRepoList) {
	const auto &sources = cubzh::repoList(_testApp->filesystem(), "xxx", "yyy");
	ASSERT_FALSE(sources.empty());
	for (const cubzh::TreeEntry &entry : sources) {
		VoxelFile file;
		file.source = "cubzh";
		file.name = entry.repo + "-" + entry.name + ".3zh";
		file.url = entry.url;
		file.fullPath = _testApp->filesystem()->writePath(file.targetFile());
		http::HttpCacheStream stream(_testApp->filesystem(), file.fullPath, file.url);
		if (stream.isNewInCache()) {
			Log::debug("Downloaded: %s", file.url.c_str());
		}
	}
}

} // namespace voxelcollection
