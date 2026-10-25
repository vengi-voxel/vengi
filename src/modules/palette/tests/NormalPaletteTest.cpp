/**
 * @file
 */

#include "palette/NormalPalette.h"
#include "app/tests/AbstractTest.h"
#include "palette/NormalPaletteLookup.h"
#include <float.h>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <math.h>

namespace palette {

class NormalPaletteTest : public app::AbstractTest {};

TEST_F(NormalPaletteTest, testSetNormalExtendsPalette) {
	NormalPalette palette;
	const color::RGBA first = NormalPalette::toRGBA(glm::vec3(1, 0, 0));
	const uint32_t emptyHash = palette.hash();
	palette.setNormal(0, first);
	EXPECT_EQ(1u, palette.size());
	EXPECT_EQ(first, palette.normal(0));
	EXPECT_NE(emptyHash, palette.hash());
	palette.markClean();
	const uint32_t firstHash = palette.hash();
	palette.setNormal(4, glm::vec3(0, 2, 0));
	EXPECT_EQ(5u, palette.size());
	EXPECT_TRUE(palette.dirty());
	EXPECT_NE(firstHash, palette.hash());
	EXPECT_EQ(NormalPalette::toRGBA(glm::vec3(0, 1, 0)), palette.normal(4));
	for (uint8_t index = 1; index < 4; ++index) {
		EXPECT_EQ(0, palette.normal(index).a);
	}
	palette.setNormal(NormalPaletteMaxNormals - 1, glm::vec3(0, 0, 1));
	EXPECT_EQ((size_t)NormalPaletteMaxNormals, palette.size());
	const uint32_t fullHash = palette.hash();
	palette.setNormal(NormalPaletteMaxNormals, first);
	palette.setNormal(NormalPaletteMaxNormals, glm::vec3(1, 0, 0));
	EXPECT_EQ((size_t)NormalPaletteMaxNormals, palette.size());
	EXPECT_EQ(fullHash, palette.hash());
}

TEST_F(NormalPaletteTest, testInvalidNormalVectors) {
	NormalPalette palette;
	palette.setNormal(0, glm::vec3(1, 0, 0));
	const uint32_t hash = palette.hash();
	const glm::vec3 invalid[] = {{0, 0, 0}, {NAN, 0, 0}, {INFINITY, 0, 0}, {FLT_MAX, FLT_MAX, FLT_MAX}};
	for (const glm::vec3 &normal : invalid) {
		palette.setNormal(1, normal);
		EXPECT_EQ(1u, palette.size());
		EXPECT_EQ(hash, palette.hash());
		EXPECT_EQ(PaletteNormalNotFound, palette.getClosestMatch(normal));
	}
}

TEST_F(NormalPaletteTest, testEmptySlotsAreNotDirections) {
	NormalPalette palette;
	palette.setNormal(4, glm::vec3(1, 0, 0));
	// Even the opposite direction must choose the only populated slot, not a hole.
	EXPECT_EQ(4, palette.getClosestMatch(glm::vec3(-1, 0, 0)));
	NormalPaletteLookup lookup(palette);
	EXPECT_EQ(4, lookup.getClosestMatch(glm::vec3(-1, 0, 0)));
	uint8_t remap[NormalPaletteMaxNormals];
	palette.createRemap(palette, remap);
	for (int i = 0; i < NormalPaletteMaxNormals; ++i) {
		EXPECT_EQ(i == 4 ? 4 : NO_NORMAL_REMAP_FOUND, remap[i]);
	}
	NormalPalette source;
	source.setNormal(0, glm::vec3(0, 1, 0));
	source.createRemap(palette, remap);
	EXPECT_EQ(4, remap[0]);
	glm::vec4 vectors[256];
	palette.toVec4f(vectors);
	for (int i = 0; i < NormalPaletteMaxNormals; ++i) {
		if (i != 4) {
			EXPECT_EQ(glm::vec4(0.0f), vectors[i]);
		}
	}
	palette.setNormal(4, color::RGBA(0, 0, 0, 0));
	EXPECT_EQ(PaletteNormalNotFound, palette.getClosestMatch(glm::vec3(1, 0, 0)));
	NormalPaletteLookup emptyLookup(palette);
	EXPECT_EQ(PaletteNormalNotFound, emptyLookup.getClosestMatch(glm::vec3(1, 0, 0)));
}

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

TEST_F(NormalPaletteTest, testSparsePaletteSaveReload) {
	NormalPalette source;
	source.setNormal(4, glm::vec3(0, 1, 0));
	ASSERT_TRUE(source.save("normal-palette-sparse.png"));
	NormalPalette restored;
	ASSERT_TRUE(restored.load("normal-palette-sparse.png"));
	EXPECT_EQ(source.size(), restored.size());
	EXPECT_EQ(source.normal(4), restored.normal(4));
	for (uint8_t i = 0; i < 4; ++i) {
		EXPECT_EQ(0, restored.normal(i).a);
	}
	EXPECT_EQ(4, restored.getClosestMatch(glm::vec3(1, 0, 0)));
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
