/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxel/tests/TestHelper.h"
#include "voxelformat/MCRFormat.h"
#include "voxelformat/QBFormat.h"

namespace voxelformat {

class MCRFormatTest: public AbstractVoxFormatTest {
};

TEST_F(MCRFormatTest, DISABLED_testLoad117Compare) {
	MCRFormat f;
	std::unique_ptr<voxel::RawVolume> volume(load("r.0.-2.mca", f));
	ASSERT_NE(nullptr, volume) << "Could not load mca volume";

	QBFormat f2;
	std::unique_ptr<voxel::RawVolume> volumeqb(load("r.0.-2.qb", f2));
	ASSERT_NE(nullptr, volumeqb) << "Could not load qb volume";

	ASSERT_TRUE(volumeComparator(*volume.get(), *volumeqb.get(), true, true));
}

TEST_F(MCRFormatTest, testLoad117) {
	MCRFormat f;
	std::unique_ptr<voxel::RawVolume> volume(load("r.0.-2.mca", f));
	ASSERT_NE(nullptr, volume) << "Could not load mca volume";
}

TEST_F(MCRFormatTest, testLoad110) {
	MCRFormat f;
	std::unique_ptr<voxel::RawVolume> volume(load("minecraft_110.mca", f));
	ASSERT_NE(nullptr, volume) << "Could not load mca volume";
}

TEST_F(MCRFormatTest, DISABLED_testLoad113) {
	MCRFormat f;
	std::unique_ptr<voxel::RawVolume> volume(load("minecraft_113.mca", f));
	ASSERT_NE(nullptr, volume) << "Could not load mca volume";
}

}
