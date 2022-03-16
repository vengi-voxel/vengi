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
	QBCLFormat f;
	std::unique_ptr<voxel::RawVolume> volume(load("qubicle.qbcl", f));
	ASSERT_NE(nullptr, volume) << "Could not load qbcl file";
}

TEST_F(QBCLFormatTest, testSaveSmallVoxel) {
	QBCLFormat f;
	testSaveLoadVoxel("qubicle-smallvolumesavetest.qbcl", &f);
}

TEST_F(QBCLFormatTest, testLoadRGB) {
	QBCLFormat f;
	std::unique_ptr<voxel::RawVolume> volume(load("rgb.qbcl", f));
	ASSERT_NE(nullptr, volume) << "Could not load qbcl file";
	testRGB(volume.get());
}

}
