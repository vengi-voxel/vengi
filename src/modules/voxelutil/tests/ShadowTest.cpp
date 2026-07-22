/**
 * @file
 */

#include "voxelutil/Shadow.h"
#include "app/tests/AbstractTest.h"
#include "palette/Palette.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelutil {

class ShadowTest : public app::AbstractTest {};

TEST_F(ShadowTest, testShadowPreservesSolidCount) {
	voxel::Region region(0, 0, 0, 7, 7, 7);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 3);
	for (int x = 1; x <= 5; ++x) {
		for (int z = 1; z <= 5; ++z) {
			volume.setVoxel(x, 1, z, solid);
			volume.setVoxel(x, 3, z, solid);
		}
	}
	const int before = visitVolumeParallel(volume, EmptyVisitor(), VisitSolid());
	palette::Palette palette;
	palette.nippon();
	voxelutil::shadow(volume, palette);
	EXPECT_EQ(before, visitVolumeParallel(volume, EmptyVisitor(), VisitSolid()));
}

TEST_F(ShadowTest, testShadowEmptyVolumeNoCrash) {
	voxel::RawVolume volume(voxel::Region(0, 3));
	palette::Palette palette;
	palette.nippon();
	voxelutil::shadow(volume, palette);
	EXPECT_EQ(0, visitVolumeParallel(volume, EmptyVisitor(), VisitSolid()));
}

TEST_F(ShadowTest, testShadowDenseVolumeNoCrash) {
	// Stress the light queue path used by dense/air-heavy volumes (reserve size matters).
	voxel::Region region(0, 0, 0, 15, 15, 15);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	for (int x = 0; x <= 15; ++x) {
		for (int z = 0; z <= 15; ++z) {
			volume.setVoxel(x, 0, z, solid);
		}
	}
	volume.setVoxel(8, 8, 8, solid);
	palette::Palette palette;
	palette.nippon();
	voxelutil::shadow(volume, palette);
	EXPECT_EQ(16 * 16 + 1, visitVolumeParallel(volume, EmptyVisitor(), VisitSolid()));
}

} // namespace voxelutil
