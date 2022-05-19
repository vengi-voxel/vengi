/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/QEFFormat.h"
#include "voxelformat/VolumeFormat.h"

namespace voxelformat {

class QEFFormatTest: public AbstractVoxFormatTest {
};

TEST_F(QEFFormatTest, testLoad) {
	canLoad("qubicle.qef");
}

TEST_F(QEFFormatTest, testLoad2) {
	canLoad("testload.qef");
}

TEST_F(QEFFormatTest, testLoadRGB) {
	testRGB("rgb.qef");
}

TEST_F(QEFFormatTest, testSaveSmallVoxel) {
	QEFFormat f;
	testSaveLoadVoxel("qubicle-smallvolumesavetest.qbt", &f);
}

}
