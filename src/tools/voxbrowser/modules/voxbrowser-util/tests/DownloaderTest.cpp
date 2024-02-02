/**
 * @file
 */

#include "voxbrowser-util/Downloader.h"
#include "app/tests/AbstractTest.h"

namespace voxbrowser {

class DownloaderTest : public app::AbstractTest {};

TEST_F(DownloaderTest, DISABLED_testExecute) {
	Downloader downloader;
	const core::DynamicArray<VoxelSource> &sources = downloader.sources();
	ASSERT_FALSE(sources.empty());
}

} // namespace voxbrowser
