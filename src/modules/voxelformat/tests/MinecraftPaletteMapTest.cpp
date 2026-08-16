/**
 * @file
 */

#include "voxelformat/private/minecraft/MinecraftPaletteMap.h"
#include "app/App.h"
#include "app/tests/AbstractTest.h"
#include "core/Log.h"
#include "core/String.h"
#include "core/collection/Set.h"
#include "io/FileStream.h"
#include "palette/Palette.h"
#include "palette/private/AVMTHelper.h"
#include <glm/vec3.hpp>

namespace voxelformat {

class MinecraftPaletteMapTest : public app::AbstractTest {};

TEST_F(MinecraftPaletteMapTest, testIsWaterBlock) {
	EXPECT_TRUE(isWaterBlock("minecraft:water"));
	EXPECT_TRUE(isWaterBlock("minecraft:water[level=0]"));
	EXPECT_TRUE(isWaterBlock("minecraft:flowing_water"));
	EXPECT_TRUE(isWaterBlock("minecraft:flowing_water,level=7"));
	EXPECT_FALSE(isWaterBlock("minecraft:stone"));
	EXPECT_FALSE(isWaterBlock("minecraft:blue_wool"));
	EXPECT_FALSE(isWaterBlock("minecraft:bubble_column"));
	EXPECT_TRUE(isLegacyWaterId(8));
	EXPECT_TRUE(isLegacyWaterId(9));
	EXPECT_FALSE(isLegacyWaterId(1));
	EXPECT_FALSE(isLegacyWaterId(10));
}

TEST_F(MinecraftPaletteMapTest, testStripBlockId) {
	McBlock block;
	block =
		parseBlock("minecraft:dark_oak_stairs[facing=east,half=bottom,shape=outer_left,waterlogged=false][INT] = 554");
	EXPECT_EQ("minecraft:dark_oak_stairs", block.normalize());
	block = parseBlock("minecraft:waxed_exposed_copper_bulb,lit=true");
	EXPECT_EQ("minecraft:waxed_exposed_copper_bulb,lit=true", block.normalize());
	block = parseBlock("minecraft:waxed_exposed_copper_bulb,biome=minecraft:badlands");
	EXPECT_EQ("minecraft:waxed_exposed_copper_bulb,biome=minecraft:badlands", block.normalize());
	block = parseBlock("minecraft:waxed_exposed_copper_bulb,lit=true,biome=minecraft:badlands");
	EXPECT_EQ("minecraft:waxed_exposed_copper_bulb,biome=minecraft:badlands,lit=true", block.normalize());
	block = parseBlock("minecraft:waxed_exposed_copper_bulb[lit=true],biome=minecraft:badlands");
	EXPECT_EQ("minecraft:waxed_exposed_copper_bulb,biome=minecraft:badlands,lit=true", block.normalize());
}

TEST_F(MinecraftPaletteMapTest, testFindPaletteIndexLitFallback) {
	EXPECT_EQ(162, findPaletteIndex("minecraft:waxed_copper_bulb"));
	EXPECT_EQ(162, findPaletteIndex("minecraft:waxed_copper_bulb,lit=false"));
	EXPECT_EQ(162, findPaletteIndex("minecraft:waxed_copper_bulb,lit=true"));
	EXPECT_EQ(73, findPaletteIndex("minecraft:waxed_exposed_copper_bulb"));
	EXPECT_EQ(48, findPaletteIndex("minecraft:waxed_weathered_copper_bulb,lit=false"));
}

TEST_F(MinecraftPaletteMapTest, DISABLED_testMaterialComplete) {
	for (int i = 0; i < palette::PaletteMaxColors; ++i) {
		const core::String &blockId = findPaletteName(i);
		EXPECT_FALSE(blockId.empty()) << "Failed to find block id for " << i;
	}
}

TEST_F(MinecraftPaletteMapTest, testAvoydMaterialTemplateColors) {
	// https://www.enkisoftware.com/products
	const io::FilePtr &file = io::filesystem()->open("Minecraft_Material.avmt");
	if (!file || !file->exists()) {
		GTEST_SKIP() << "No minecraft material avmt found";
		return;
	}
	palette::Palette pal;
	pal.minecraft();

	io::FileStream stream(file);
	core::DynamicArray<palette::AVMTMaterial> materials;
	core::String paletteName;
	ASSERT_TRUE(parseMaterials(stream, materials, paletteName));

	materials.sort(core::Greater<palette::AVMTMaterial>());

	core::Set<McBlock, 11, McBlock> found;
	for (const auto &e : materials) {
		const int palMatch = pal.getClosestMatch(e.rgba);
		const McBlock &block = parseBlock(e.name);
		const core::String &normalized = block.normalize();
		const int foundPalIdx = findPaletteIndex(normalized.c_str());
		if (palMatch == foundPalIdx) {
			continue;
		}
		if (foundPalIdx != -1) {
			Log::warn("Mismatch for %s (%s): should be %i but found was %i", e.name.c_str(), normalized.c_str(),
					  palMatch, foundPalIdx);
		}
		if (!found.insert(block)) {
			continue;
		}
		const core::String &normalizedBlock = block.normalize();
		Log::printf("\tMCENTRY(\"%s\", %i, 0xFF),                   \\\n", normalizedBlock.c_str(), palMatch);
	}
}

} // namespace voxelformat
