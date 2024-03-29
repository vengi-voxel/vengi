/**
 * @file
 */

#include "voxelformat/private/starmade/SMTPLFormat.h"
#include "AbstractVoxFormatTest.h"
#include "voxelformat/tests/TestHelper.h"

namespace voxelformat {

class SMTPLFormatTest: public AbstractVoxFormatTest {
};

TEST_F(SMTPLFormatTest, testSave) {
	SMTPLFormat f;
	testSave("testSave.smtpl", &f, voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Palette | voxel::ValidateFlags::Color));
}

}
