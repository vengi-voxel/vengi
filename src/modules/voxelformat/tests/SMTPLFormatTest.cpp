/**
 * @file
 */

#include "voxelformat/private/starmade/SMTPLFormat.h"
#include "AbstractVoxFormatTest.h"

namespace voxelformat {

class SMTPLFormatTest: public AbstractVoxFormatTest {
};

TEST_F(SMTPLFormatTest, testSave) {
	SMTPLFormat f;
	testSave("testSave.smtpl", &f);
}

}
