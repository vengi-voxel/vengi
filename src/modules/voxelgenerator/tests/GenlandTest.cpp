/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "core/ScopedPtr.h"
#include "voxel/RawVolume.h"
#include "voxelgenerator/Genland.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelgenerator {

class GenlandTest : public app::AbstractTest {
protected:
	static int columnHeight(const voxel::RawVolume &volume, int x, int z) {
		const voxel::Region &region = volume.region();
		for (int y = region.getUpperY(); y >= region.getLowerY(); --y) {
			if (!voxel::isAir(volume.voxel(x, y, z).getMaterial())) {
				return y - region.getLowerY() + 1;
			}
		}
		return 0;
	}

	static bool volumesEqual(const voxel::RawVolume &a, const voxel::RawVolume &b) {
		if (a.region() != b.region()) {
			return false;
		}
		const voxel::Region &region = a.region();
		for (int z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
			for (int y = region.getLowerY(); y <= region.getUpperY(); ++y) {
				for (int x = region.getLowerX(); x <= region.getUpperX(); ++x) {
					if (!(a.voxel(x, y, z) == b.voxel(x, y, z))) {
						return false;
					}
				}
			}
		}
		return true;
	}
};

TEST_F(GenlandTest, testDefaultsProduceVolume) {
	GenlandSettings settings;
	settings.size = 32;
	settings.height = 64;
	settings.seed = 0;
	core::ScopedPtr<voxel::RawVolume> volume(genland(settings));
	ASSERT_NE(nullptr, volume);
	EXPECT_EQ(32, volume->region().getWidthInVoxels());
	EXPECT_EQ(64, volume->region().getHeightInVoxels());
	EXPECT_EQ(32, volume->region().getDepthInVoxels());
	EXPECT_GT(voxelutil::countVoxels(*volume), 0);
}

TEST_F(GenlandTest, testTallHeightUsesIntColumns) {
	GenlandSettings settings;
	settings.size = 32;
	settings.height = 400;
	settings.baseHeight = 300.0;
	settings.amplitude = 20.0;
	settings.seed = 1;
	settings.river = false;
	core::ScopedPtr<voxel::RawVolume> volume(genland(settings));
	ASSERT_NE(nullptr, volume);
	EXPECT_EQ(400, volume->region().getHeightInVoxels());

	int maxY = -1;
	voxelutil::visitVolume(*volume, [&maxY](int, int y, int, const voxel::Voxel &) {
		maxY = core_max(maxY, y);
	});
	// Without int column heights, values above 255 would wrap in uint8 storage.
	EXPECT_GT(maxY, 255);
}

TEST_F(GenlandTest, testHeightIsClippedToVolume) {
	GenlandSettings settings;
	settings.size = 32;
	settings.height = 48;
	settings.baseHeight = 500.0;
	settings.amplitude = 0.0;
	settings.river = false;
	settings.seed = 2;
	core::ScopedPtr<voxel::RawVolume> volume(genland(settings));
	ASSERT_NE(nullptr, volume);

	int maxHeight = 0;
	int minHeight = settings.height + 1;
	for (int z = 0; z < settings.size; ++z) {
		for (int x = 0; x < settings.size; ++x) {
			const int h = columnHeight(*volume, x, z);
			maxHeight = core_max(maxHeight, h);
			minHeight = core_min(minHeight, h);
		}
	}
	EXPECT_EQ(settings.height, maxHeight);
	EXPECT_EQ(settings.height, minHeight);
}

TEST_F(GenlandTest, testSeedIsDeterministic) {
	GenlandSettings settings;
	settings.size = 32;
	settings.height = 64;
	settings.seed = 42;

	core::ScopedPtr<voxel::RawVolume> a(genland(settings));
	core::ScopedPtr<voxel::RawVolume> b(genland(settings));
	ASSERT_NE(nullptr, a);
	ASSERT_NE(nullptr, b);
	EXPECT_TRUE(volumesEqual(*a, *b));

	settings.seed = 43;
	core::ScopedPtr<voxel::RawVolume> c(genland(settings));
	ASSERT_NE(nullptr, c);
	EXPECT_FALSE(volumesEqual(*a, *c));
}

TEST_F(GenlandTest, testRiversCarveTerrain) {
	GenlandSettings base;
	base.size = 64;
	base.height = 64;
	base.seed = 7;
	base.amplitude = 20.0;
	base.baseHeight = 28.0;
	base.riverWidth = 0.25;
	base.numRivers = 2;
	base.riverMeander = 0.0;

	GenlandSettings withRivers = base;
	withRivers.river = true;

	GenlandSettings withoutRivers = base;
	withoutRivers.river = false;

	core::ScopedPtr<voxel::RawVolume> riverVolume(genland(withRivers));
	core::ScopedPtr<voxel::RawVolume> landVolume(genland(withoutRivers));
	ASSERT_NE(nullptr, riverVolume);
	ASSERT_NE(nullptr, landVolume);

	const int riverVoxels = voxelutil::countVoxels(*riverVolume);
	const int landVoxels = voxelutil::countVoxels(*landVolume);
	EXPECT_LT(riverVoxels, landVoxels);
	EXPECT_FALSE(volumesEqual(*riverVolume, *landVolume));
}

TEST_F(GenlandTest, testNumRiversChangesLayout) {
	GenlandSettings settings;
	settings.size = 64;
	settings.height = 64;
	settings.seed = 11;
	settings.river = true;
	settings.riverWidth = 0.2;
	settings.riverMeander = 0.0;
	settings.riverPhase = 0.0;

	settings.numRivers = 1;
	core::ScopedPtr<voxel::RawVolume> oneRiver(genland(settings));
	settings.numRivers = 4;
	core::ScopedPtr<voxel::RawVolume> fourRivers(genland(settings));
	ASSERT_NE(nullptr, oneRiver);
	ASSERT_NE(nullptr, fourRivers);
	EXPECT_FALSE(volumesEqual(*oneRiver, *fourRivers));
}

TEST_F(GenlandTest, testRejectInvalidSettings) {
	{
		GenlandSettings settings;
		settings.size = 30;
		core::ScopedPtr<voxel::RawVolume> volume(genland(settings));
		EXPECT_EQ(nullptr, volume);
	}
	{
		GenlandSettings settings;
		settings.size = 32;
		settings.octaves = 0;
		core::ScopedPtr<voxel::RawVolume> volume(genland(settings));
		EXPECT_EQ(nullptr, volume);
	}
	{
		GenlandSettings settings;
		settings.size = 32;
		settings.height = 0;
		core::ScopedPtr<voxel::RawVolume> volume(genland(settings));
		EXPECT_EQ(nullptr, volume);
	}
}

} // namespace voxelgenerator
