/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxel/model/QB2Format.h"

namespace voxel {

class QB2FormatTest: public AbstractVoxFormatTest {
};

TEST_F(QB2FormatTest, DISABLED_testLoad) {
	const io::FilePtr& file = core::App::getInstance()->filesystem()->open("qubicle.qbt");
	ASSERT_TRUE((bool)file) << "Could not open qbt file";
	QB2Format f;
	RawVolume* volume = f.load(file);
	ASSERT_NE(nullptr, volume) << "Could not load qbt file";
	delete volume;
}

}
