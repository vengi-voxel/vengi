/**
 * @file
 */

#include "voxelformat/private/minecraft/MinecraftPaletteMap.h"
#include "app/App.h"
#include "app/tests/AbstractTest.h"
#include "core/Color.h"
#include "core/Log.h"
#include "voxel/Palette.h"
#include <json.hpp>
#include <glm/vec3.hpp>

namespace voxelformat {

class MinecraftPaletteMapTest : public app::AbstractTest {};

TEST_F(MinecraftPaletteMapTest, testParse) {
	EXPECT_EQ(164,
			  findPaletteIndex(
				  "minecraft:dark_oak_stairs[facing=east,half=bottom,shape=outer_left,waterlogged=false][INT] = 554"));
}

// parses a blocks.json file to find new colors
TEST_F(MinecraftPaletteMapTest, DISABLED_testNewColors) {
	const core::String &str = io::filesystem()->load("blocks.json");
	if (str.empty()) {
		GTEST_SKIP() << "No or empty blocks.json found";
		return;
	}
	voxel::Palette pal;
	pal.minecraft();
	nlohmann::json j = nlohmann::json::parse(str);
	for (const auto &e : j) {
		const auto &rgb = e["rgb"];
		if (rgb.empty()) {
			continue;
		}
		if (rgb.size() != 3) {
			continue;
		}
		core::RGBA rgba;
		{
			int idx = 0;
			glm::ivec3 color;
			for (const auto & c : rgb) {
				color[idx++] = c.get<int>();
			}
			rgba.r = color[0];
			rgba.g = color[1];
			rgba.b = color[2];
			rgba.a = 255;
		}

		const auto &blocks = e["blocks"];
		if (blocks.empty()) {
			continue;
		}
		for (const auto &block : blocks) {
			const std::string blockId = block.get<std::string>();
			if (findPaletteIndex(blockId.c_str()) == -1) {
				float distance = 0.0f;
				int palMatch = pal.getClosestMatch(rgba, &distance);
				if (distance > 0.005) {
					printf("%s\n", core::Color::toHex(rgba).c_str());
				} else {
					printf("\tMCENTRY(\"%s\", %i, 0xFF),                   \\\n", blockId.c_str(), palMatch);
				}
			} else {
				//Log::error("Found %s", blockId.c_str());
			}
		}
	}
}

} // namespace voxelformat
