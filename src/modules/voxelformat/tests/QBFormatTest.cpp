/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "voxelformat/tests/TestHelper.h"
#include "voxelformat/private/qubicle/QBFormat.h"

namespace voxelformat {

class QBFormatTest: public AbstractFormatTest {
};

TEST_F(QBFormatTest, testLoad) {
	testLoad("qubicle.qb", 10);
}

TEST_F(QBFormatTest, testLoadRGB) {
	testRGB("rgb.qb");
}

TEST_F(QBFormatTest, testLoadRGBSmall) {
	testRGBSmall("rgb_small.qb");
}

TEST_F(QBFormatTest, testLoadRGBSmallSaveLoad) {
	testRGBSmallSaveLoad("rgb_small.qb");
}

TEST_F(QBFormatTest, testSaveSingleVoxel) {
	QBFormat f;
	testSaveSingleVoxel("qubicle-singlevoxelsavetest.qb", &f);
}

TEST_F(QBFormatTest, testSaveSmallVoxel) {
	QBFormat f;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~voxel::ValidateFlags::Palette;
	testSaveLoadVoxel("qubicle-smallvolumesavetest.qb", &f, 0, 1, flags);
}

TEST_F(QBFormatTest, testSaveMultipleModels) {
	QBFormat f;
	testSaveMultipleModels("qubicle-multiplemodelsavetest.qb", &f);
}

}
