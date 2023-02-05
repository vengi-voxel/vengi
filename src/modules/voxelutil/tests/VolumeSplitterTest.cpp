/**
 * @file
 */

#include "core/collection/Vector.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "app/tests/AbstractTest.h"
#include "voxelutil/VolumeMerger.h"
#include "voxelutil/VolumeSplitter.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelutil {

class VolumeSplitterTest: public app::AbstractTest {
protected:
	template<typename Volume>
	inline int countVoxels(const Volume& volume, const voxel::Voxel &voxel) {
		int cnt = 0;
		voxelutil::visitVolume(volume, [&](int, int, int, const voxel::Voxel &v) {
			if (v == voxel) {
				++cnt;
			}
		}, voxelutil::VisitAll());
		return cnt;
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
	wrapper.setVoxels(0, 0, 0, region.getWidthInVoxels(), region.getDepthInVoxels(), &voxels.front(), region.getHeightInVoxels());

	const int expectedVoxelCount = region.getWidthInVoxels() * region.getDepthInVoxels() * region.getHeightInVoxels();
	const int foundVoxels = countVoxels(volume, voxel);
	EXPECT_EQ(expectedVoxelCount, foundVoxels);

	// perform split
	core::DynamicArray<voxel::RawVolume *> rawVolumes;
	voxelutil::splitVolume(&volume, glm::ivec3(16), rawVolumes);
	EXPECT_EQ(8u, rawVolumes.size());

	// merge volumes
	voxel::RawVolume *merged = voxelutil::merge(rawVolumes);
	for (voxel::RawVolume* v: rawVolumes) {
		delete v;
	}
	rawVolumes.clear();

	// count voxels
	const int foundVoxelsAfterSplitAndMerge = countVoxels(*merged, voxel);
	EXPECT_EQ(expectedVoxelCount, foundVoxelsAfterSplitAndMerge);

	delete merged;
}

}
