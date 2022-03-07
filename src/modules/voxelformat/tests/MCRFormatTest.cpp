/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxel/tests/TestHelper.h"
#include "voxelformat/MCRFormat.h"
#include "voxelformat/QBFormat.h"

namespace voxel {

class MCRFormatTest: public AbstractVoxFormatTest {
};

TEST_F(MCRFormatTest, testLoad) {
	MCRFormat f;
	std::unique_ptr<RawVolume> volume(load("r.0.-2.mca", f));
	ASSERT_NE(nullptr, volume) << "Could not load mca volume";

	QBFormat f2;
	std::unique_ptr<RawVolume> volumeqb(load("r.0.-2.qb", f2));
	ASSERT_NE(nullptr, volumeqb) << "Could not load qb volume";

	ASSERT_TRUE(volumeComparator(*volume.get(), *volumeqb.get(), true, true));
}

}
