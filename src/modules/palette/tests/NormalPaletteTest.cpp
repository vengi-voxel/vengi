/**
 * @file
 */

#include "palette/NormalPalette.h"
#include "app/tests/AbstractTest.h"
#include "palette/NormalPaletteLookup.h"
#include <glm/vec3.hpp>

namespace palette {

class NormalPaletteTest : public app::AbstractTest {};

TEST_F(NormalPaletteTest, testSave) {
	NormalPalette palette;
	palette.redAlert2();
	EXPECT_TRUE(palette.save("redalert2.png"));
}

TEST_F(NormalPaletteTest, testGetClosestMatch) {
	NormalPalette palette;
	palette.redAlert2();
	EXPECT_EQ(0, palette.getClosestMatch(NormalPalette::toVec3({194, 81, 29}))); // first entry of the ra normal palette
	EXPECT_EQ(97, palette.getClosestMatch(NormalPalette::toVec3({2, 135, 101}))); // 97th entry of the ra normal palette
}

TEST_F(NormalPaletteTest, testGetClosestMatchLookup) {
	NormalPalette palette;
	palette.redAlert2();
	NormalPaletteLookup lookup(palette);
	EXPECT_EQ(0, lookup.getClosestMatch(NormalPalette::toVec3({194, 81, 29}))); // first entry of the ra normal palette
	EXPECT_EQ(97, lookup.getClosestMatch(NormalPalette::toVec3({2, 135, 101}))); // 97th entry of the ra normal palette
}

} // namespace palette
