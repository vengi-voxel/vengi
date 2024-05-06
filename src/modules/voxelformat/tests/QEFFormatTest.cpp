/**
 * @file
 */

#include "voxelformat/private/qubicle/QEFFormat.h"
#include "AbstractFormatTest.h"

namespace voxelformat {

class QEFFormatTest : public AbstractFormatTest {};

TEST_F(QEFFormatTest, testLoad) {
	testLoad("qubicle.qef");
}

TEST_F(QEFFormatTest, testLoad2) {
	testLoad("testload.qef");
}

TEST_F(QEFFormatTest, testLoadRGB) {
	testRGB("rgb.qef");
}

TEST_F(QEFFormatTest, testSaveSmallVoxel) {
	QEFFormat f;
	testSaveLoadVoxel("qubicle-smallvolumesavetest.qef", &f);
}

} // namespace voxelformat
