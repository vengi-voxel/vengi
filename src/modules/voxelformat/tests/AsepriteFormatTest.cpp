/**
 * @file
 */

#include "AbstractFormatTest.h"

namespace voxelformat {

class AsepriteFormatTest : public AbstractFormatTest {};

TEST_F(AsepriteFormatTest, testLoad) {
	testLoad("sylvie/02.ase");
	testLoad("sylvie/404.ase", 55);
	testLoad("sylvie/46.ase");
	testLoad("sylvie/line.ase", 18);
	testLoad("sylvie/mapmenu.ase");
	testLoad("libresprite.ase");
	testLoad("libresprite.aseprite");
}

} // namespace voxelformat
