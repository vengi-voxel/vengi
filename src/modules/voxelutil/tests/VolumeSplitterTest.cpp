/**
 * @file
 */

#include "voxelutil/VolumeSplitter.h"
#include "app/tests/AbstractTest.h"
#include "core/ScopedPtr.h"
#include "core/collection/Vector.h"
#include "voxel/Connectivity.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/VolumeSamplerUtil.h"
#include "voxelutil/VolumeMerger.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelutil {

class VolumeSplitterTest : public app::AbstractTest {
protected:
	void prepareSplitterVolume(voxel::RawVolume &volume) const {
		const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);

		volume.setVoxel(16, 16, 16, voxel);
		volume.setVoxel(15, 16, 16, voxel);
		volume.setVoxel(14, 16, 16, voxel);
		volume.setVoxel(16, 15, 16, voxel);
		volume.setVoxel(15, 15, 16, voxel);
		volume.setVoxel(14, 15, 16, voxel);

		volume.setVoxel(13, 14, 15, voxel);

		volume.setVoxel(10, 10, 10, voxel);

		volume.setVoxel(11, 11, 11, voxel);

		volume.setVoxel(0, 0, 0, voxel);
		volume.setVoxel(0, 0, 1, voxel);
		volume.setVoxel(0, 1, 1, voxel);

		for (int x = 0; x < 3; ++x) {
			for (int y = 0; y < 3; ++y) {
				for (int z = 0; z < 3; ++z) {
					if (x == 1 && y == 1 && z == 1) {
						continue;
					}
					volume.setVoxel(x + 20, y + 20, z + 20, voxel);
				}
			}
		}

		volume.setVoxel(25, 24, 25, voxel);
		volume.setVoxel(25, 26, 25, voxel);
		volume.setVoxel(24, 25, 25, voxel);
		volume.setVoxel(26, 25, 25, voxel);
		volume.setVoxel(25, 25, 24, voxel);
		volume.setVoxel(25, 25, 26, voxel);
	}

	void validateSplit(const core::Buffer<voxel::RawVolume *> &volumes) const {
		ASSERT_EQ(7u, volumes.size());
		EXPECT_EQ(3, voxelutil::countVoxelsByType(*volumes[0], voxel::createVoxel(voxel::VoxelType::Generic, 1)));
		EXPECT_EQ(1, voxelutil::countVoxelsByType(*volumes[1], voxel::createVoxel(voxel::VoxelType::Generic, 1)));
		EXPECT_EQ(1, voxelutil::countVoxelsByType(*volumes[2], voxel::createVoxel(voxel::VoxelType::Generic, 1)));
		EXPECT_EQ(1, voxelutil::countVoxelsByType(*volumes[3], voxel::createVoxel(voxel::VoxelType::Generic, 1)));
		EXPECT_EQ(6, voxelutil::countVoxelsByType(*volumes[4], voxel::createVoxel(voxel::VoxelType::Generic, 1)));
		EXPECT_EQ(26, voxelutil::countVoxelsByType(*volumes[5], voxel::createVoxel(voxel::VoxelType::Generic, 1)));
		EXPECT_EQ(6, voxelutil::countVoxelsByType(*volumes[6], voxel::createVoxel(voxel::VoxelType::Generic, 1)));
		for (voxel::RawVolume *v : volumes) {
			delete v;
		}
	}
};

TEST_F(VolumeSplitterTest, testSplit) {
	const voxel::Region region(0, 31);
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	core::Vector<voxel::Voxel, 32> voxels;
	voxels.assign(voxel, region.getHeightInVoxels());

	// prepare volume
	voxel::RawVolume volume(region);
	voxel::RawVolumeWrapper wrapper(&volume);
	voxel::setVoxels(wrapper, 0, 0, 0, region.getWidthInVoxels(), region.getDepthInVoxels(), &voxels.front(),
					  region.getHeightInVoxels());

	const int expectedVoxelCount = region.getWidthInVoxels() * region.getDepthInVoxels() * region.getHeightInVoxels();
	const int foundVoxels = countVoxelsByType(volume, voxel);
	EXPECT_EQ(expectedVoxelCount, foundVoxels);

	// perform split
	core::Buffer<voxel::RawVolume *> rawVolumes = voxelutil::splitVolume(&volume, glm::ivec3(16));
	EXPECT_EQ(8u, rawVolumes.size());

	// merge volumes
	core::ScopedPtr<voxel::RawVolume> merged(voxelutil::merge(rawVolumes));
	for (voxel::RawVolume *v : rawVolumes) {
		delete v;
	}
	rawVolumes.clear();

	// count voxels
	const int foundVoxelsAfterSplitAndMerge = voxelutil::countVoxelsByType(*merged, voxel);
	EXPECT_EQ(expectedVoxelCount, foundVoxelsAfterSplitAndMerge);
}

TEST_F(VolumeSplitterTest, testSplitEmpty) {
	const voxel::Region region(0, 31);
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	core::Vector<voxel::Voxel, 32> voxels;
	voxels.assign(voxel, region.getHeightInVoxels());

	// prepare volume
	voxel::RawVolume volume(region);
	voxel::RawVolumeWrapper wrapper(&volume);
	voxel::setVoxels(wrapper, 0, 0, 0, region.getWidthInVoxels(), region.getDepthInVoxels(), &voxels.front(),
					  region.getHeightInVoxels());

	const int expectedVoxelCount = region.getWidthInVoxels() * region.getDepthInVoxels() * region.getHeightInVoxels();
	const int foundVoxels = voxelutil::countVoxelsByType(volume, voxel);
	EXPECT_EQ(expectedVoxelCount, foundVoxels);

	// perform split
	core::Buffer<voxel::RawVolume *> rawVolumes = voxelutil::splitVolume(&volume, glm::ivec3(16), true);
	EXPECT_EQ(8u, rawVolumes.size());

	// merge volumes
	core::ScopedPtr<voxel::RawVolume> merged(voxelutil::merge(rawVolumes));
	for (voxel::RawVolume *v : rawVolumes) {
		delete v;
	}
	rawVolumes.clear();

	// count voxels
	const int foundVoxelsAfterSplitAndMerge = voxelutil::countVoxelsByType(*merged, voxel);
	EXPECT_EQ(expectedVoxelCount, foundVoxelsAfterSplitAndMerge);
}

TEST_F(VolumeSplitterTest, testSplitObjects) {
	const voxel::Region region(0, 31);
	voxel::RawVolume volume(region);
	prepareSplitterVolume(volume);
	validateSplit(voxelutil::splitObjects(&volume, VisitorOrder::ZYX, voxel::Connectivity::EighteenConnected));
}

} // namespace voxelutil
