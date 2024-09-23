/**
 * @file
 */

#include "palette/NormalPalette.h"
#include "app/tests/AbstractTest.h"

namespace palette {

class NormalPaletteTest : public app::AbstractTest {};

TEST_F(NormalPaletteTest, testSave) {
	NormalPalette palette;
	palette.redAlert2();
	EXPECT_TRUE(palette.save("redalert2.png"));
}

} // namespace palette
