/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/QEFFormat.h"
#include "voxelformat/Loader.h"

namespace voxel {

class QEFFormatTest: public AbstractVoxFormatTest {
};

TEST_F(QEFFormatTest, testLoad) {
	QEFFormat f;
	std::unique_ptr<RawVolume> volume(load("qubicle.qef", f));
	ASSERT_NE(nullptr, volume) << "Could not load qef file";
}

}
