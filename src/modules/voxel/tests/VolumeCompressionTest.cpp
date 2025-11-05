/**
 * @file
 */

#include "voxel/VolumeCompression.h"
#include "app/tests/AbstractTest.h"
#include "core/ScopedPtr.h"
#include "io/BufferedReadWriteStream.h"
#include "io/ZipWriteStream.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxel {

class VoxelCompressionTest : public app::AbstractTest {};

TEST_F(VoxelCompressionTest, testToVolume) {
	voxel::Region region(0, 0, 0, 15, 15, 15);
	voxel::RawVolume v(region);
	voxel::Voxel voxel = createVoxel(voxel::VoxelType::Generic, 1);
	for (int x = region.getLowerX() + 1; x <= region.getUpperX(); ++x) {
		for (int y = region.getLowerY(); y <= region.getUpperY(); ++y) {
			for (int z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
				v.setVoxel(glm::ivec3(x, y, z), voxel);
			}
		}
	}
	const int expected = voxelutil::countVoxels(v);
	const int allVoxels = v.region().voxels();
	io::BufferedReadWriteStream outStream((int64_t)allVoxels * sizeof(voxel::Voxel));
	{
		io::ZipWriteStream stream(outStream, 12);
		ASSERT_NE(stream.write(v.data(), allVoxels * sizeof(voxel::Voxel)), -1);
		ASSERT_TRUE(stream.flush());
	}
	ASSERT_NE(outStream.seek(0), -1);
	core::ScopedPtr<voxel::RawVolume> decompressedVolume(
		voxel::toVolume(outStream.getBuffer(), outStream.size(), region));
	ASSERT_TRUE(decompressedVolume != nullptr);
	const int actual = voxelutil::countVoxels(*decompressedVolume);
	EXPECT_EQ(expected, actual);
}

} // namespace voxel
