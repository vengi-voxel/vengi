/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/QEFFormat.h"
#include "voxelformat/VolumeFormat.h"

namespace voxel {

class QEFFormatTest: public AbstractVoxFormatTest {
};

TEST_F(QEFFormatTest, testLoad) {
	QEFFormat f;
	std::unique_ptr<RawVolume> volume(load("qubicle.qef", f));
	ASSERT_NE(nullptr, volume) << "Could not load qef file";
}

TEST_F(QEFFormatTest, testLoad2) {
	QEFFormat f;
	std::unique_ptr<RawVolume> volume(load("testload.qef", f));
	ASSERT_NE(nullptr, volume) << "Could not load qef file";
}

TEST_F(QEFFormatTest, testLoadRGB) {
	QEFFormat f;
	std::unique_ptr<RawVolume> volume(load("rgb.qef", f));
	ASSERT_NE(nullptr, volume) << "Could not load qef file";
	testRGB(volume.get());
}

TEST_F(QEFFormatTest, testSaveSmallVoxel) {
	QEFFormat f;
	testSaveLoadVoxel("qubicle-smallvolumesavetest.qbt", &f);
}

}
