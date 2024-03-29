/**
 * @file
 */

#include "voxelformat/private/starmade/SMTPLFormat.h"
#include "AbstractVoxFormatTest.h"
#include "palette/Palette.h"
#include "voxelformat/tests/TestHelper.h"

namespace voxelformat {

class SMTPLFormatTest: public AbstractVoxFormatTest {
};

TEST_F(SMTPLFormatTest, testSave) {
	SMTPLFormat f;
	palette::Palette pal;
	pal.starMade();
	testSave("testSave.smtpl", &f, pal, voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Palette | voxel::ValidateFlags::Color));
}

}
