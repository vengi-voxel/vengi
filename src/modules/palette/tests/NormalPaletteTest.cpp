/**
 * @file
 */

#include "palette/NormalPalette.h"
#include "app/tests/AbstractTest.h"
#include "palette/NormalPaletteLookup.h"
#include <glm/vec3.hpp>

namespace palette {

class NormalPaletteTest : public app::AbstractTest {};

TEST_F(NormalPaletteTest, testRemap) {
	const glm::vec3 targetNormals[] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {-1, 0, 0}};
	const glm::vec3 sourceNormals[] = {{0, 0, 1}, {1, 0, 0}, {-1, 0, 0}};
	NormalPalette target;
	target.loadNormalMap(targetNormals, 4);
	NormalPalette source;
	source.loadNormalMap(sourceNormals, 3);
	uint8_t remap[NormalPaletteMaxNormals];
	source.createRemap(target, remap);
	EXPECT_EQ(2, remap[0]);
	EXPECT_EQ(0, remap[1]);
	EXPECT_EQ(3, remap[2]);
	EXPECT_EQ(NO_NORMAL_REMAP_FOUND, remap[3]);
	NormalPalette empty;
	source.createRemap(empty, remap);
	EXPECT_EQ(NO_NORMAL_REMAP_FOUND, remap[0]);
	for (const char *name : NormalPalette::builtIn) {
		ASSERT_TRUE(source.load(name));
		source.createRemap(source, remap);
		for (size_t i = 0; i < source.size(); ++i) {
			EXPECT_EQ(i, remap[i]);
		}
	}
}

TEST_F(NormalPaletteTest, testSave) {
	NormalPalette palette;
	palette.redAlert2();
	EXPECT_TRUE(palette.save("redalert2.png"));
}

TEST_F(NormalPaletteTest, testGetClosestMatch) {
	NormalPalette palette;
	palette.redAlert2();
	EXPECT_EQ(0, palette.getClosestMatch(NormalPalette::toVec3({194, 29, 174}))); // first entry of the ra normal palette
	EXPECT_EQ(97, palette.getClosestMatch(NormalPalette::toVec3({2, 101, 120}))); // 97th entry of the ra normal palette
}

TEST_F(NormalPaletteTest, testGetClosestMatchLookup) {
	NormalPalette palette;
	palette.redAlert2();
	NormalPaletteLookup lookup(palette);
	EXPECT_EQ(0, lookup.getClosestMatch(NormalPalette::toVec3({194, 29, 174}))); // first entry of the ra normal palette
	EXPECT_EQ(97, lookup.getClosestMatch(NormalPalette::toVec3({2, 101, 120}))); // 97th entry of the ra normal palette
}

} // namespace palette
