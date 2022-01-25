/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/QBCLFormat.h"
#include "voxelformat/VolumeFormat.h"

namespace voxel {

class QBCLFormatTest: public AbstractVoxFormatTest {
};

TEST_F(QBCLFormatTest, testLoad) {
	QBCLFormat f;
	std::unique_ptr<RawVolume> volume(load("qubicle.qbcl", f));
	ASSERT_NE(nullptr, volume) << "Could not load qbcl file";
}

}
