/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/QBTFormat.h"

namespace voxel {

class QBTFormatTest: public AbstractVoxFormatTest {
};

TEST_F(QBTFormatTest, DISABLED_testLoad) {
	const io::FilePtr& file = _testApp->filesystem()->open("qubicle.qbt");
	ASSERT_TRUE((bool)file) << "Could not open qbt file";
	QBTFormat f;
	RawVolume* volume = f.load(file);
	ASSERT_NE(nullptr, volume) << "Could not load qbt file";
	delete volume;
}

}
