/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "io/BufferedReadWriteStream.h"
#include "voxelformat/QBCLFormat.h"
#include "voxelformat/VolumeFormat.h"

namespace voxelformat {

class QBCLFormatTest: public AbstractVoxFormatTest {
};

TEST_F(QBCLFormatTest, testLoad) {
	canLoad("qubicle.qbcl");
}

TEST_F(QBCLFormatTest, testSaveSmallVoxel) {
	QBCLFormat f;
	testSaveLoadVoxel("qubicle-smallvolumesavetest.qbcl", &f);
}

TEST_F(QBCLFormatTest, testLoadRGB) {
	testRGB("rgb.qbcl");
}

}
