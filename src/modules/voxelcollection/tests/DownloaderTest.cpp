/**
 * @file
 */

#include "voxelcollection/Downloader.h"
#include "app/tests/AbstractTest.h"

namespace voxelcollection {

class DownloaderTest : public app::AbstractTest {};

TEST_F(DownloaderTest, testParseSources) {
	const core::String json = R"(
		{
			"sources": [
				{
					"name": "Vengi voxelized",
					"license": "custom",
					"thumbnail": "https://raw.githubusercontent.com/vengi-voxel/voxelized/main/sponza-scale-0.3.png",
					"github": {
						"repo": "vengi-voxel/voxelized",
						"commit": "main",
						"license": "https://raw.githubusercontent.com/KhronosGroup/glTF-Sample-Models/main/2.0/Sponza/README.md"
					}
				},
				{
					"name": "Vengi",
					"license": "custom",
					"thumbnail": "https://raw.githubusercontent.com/vengi-voxel/voxelized/main/sponza-scale-0.3.png",
					"github": {
						"repo": "vengi-voxel/vengi",
						"path": "data",
						"commit": "master"
					}
				}
			]
		}
	)";
	Downloader downloader;
	const core::DynamicArray<VoxelSource> &sources = downloader.sources(json);
	ASSERT_EQ(2u, sources.size());
	ASSERT_EQ("Vengi voxelized", sources[0].name);
	ASSERT_EQ("custom", sources[0].license);
	ASSERT_EQ("https://raw.githubusercontent.com/vengi-voxel/voxelized/main/sponza-scale-0.3.png", sources[0].thumbnail);
	ASSERT_EQ("github", sources[0].provider);
	ASSERT_EQ("vengi-voxel/voxelized", sources[0].github.repo);
	ASSERT_EQ("main", sources[0].github.commit);
	ASSERT_EQ("https://raw.githubusercontent.com/KhronosGroup/glTF-Sample-Models/main/2.0/Sponza/README.md", sources[0].github.license);
	ASSERT_EQ("Vengi", sources[1].name);
	ASSERT_EQ("custom", sources[1].license);
	ASSERT_EQ("https://raw.githubusercontent.com/vengi-voxel/voxelized/main/sponza-scale-0.3.png", sources[1].thumbnail);
	ASSERT_EQ("github", sources[1].provider);
	ASSERT_EQ("vengi-voxel/vengi", sources[1].github.repo);
	ASSERT_EQ("master", sources[1].github.commit);
}

// disabled because it requires network access
TEST_F(DownloaderTest, DISABLED_testDownloadJsonAndParse) {
	Downloader downloader;
	const core::DynamicArray<VoxelSource> &sources = downloader.sources();
	ASSERT_FALSE(sources.empty());
}

} // namespace voxelcollection
