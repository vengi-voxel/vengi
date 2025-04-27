/**
 * @file
 */

#include "voxelformat/private/minecraft/MinecraftPaletteMap.h"
#include "app/App.h"
#include "app/tests/AbstractTest.h"
#include "core/Color.h"
#include "core/Log.h"
#include "core/Process.h"
#include "io/BufferedReadWriteStream.h"
#include "palette/Palette.h"
#include "json/JSON.h"
#include <glm/vec3.hpp>

namespace voxelformat {

class MinecraftPaletteMapTest : public app::AbstractTest {};

TEST_F(MinecraftPaletteMapTest, testParse) {
	EXPECT_EQ(164,
			  findPaletteIndex(
				  "minecraft:dark_oak_stairs[facing=east,half=bottom,shape=outer_left,waterlogged=false][INT] = 554"));
	EXPECT_EQ(34, findPaletteIndex("stairs:stair_outer_desert_sandstone_brick"));
	EXPECT_EQ(34, findPaletteIndex("stairs:stair_outer_desert_sandstone_brick[INT]"));
}

// parses a blocks.json file to find new colors
// disabled because the blocks.json file is not available in the repository and was parsed already
// example blocks.json layout:
// [
// 	{
// 		"rgb": [
// 			255,
// 			255,
// 			255
// 		],
// 		"blocks": [
// 			"minecraft:stone[facing=east][INT]",
// 			"minecraft:grass",
// 			"minecraft:dirt"
// 		]
// 	}
// ]

TEST_F(MinecraftPaletteMapTest, DISABLED_testNewColors) {
	const core::String &str = io::filesystem()->load("blocks.json");
	if (str.empty()) {
		GTEST_SKIP() << "No or empty blocks.json found";
		return;
	}
	palette::Palette pal;
	pal.minecraft();
	nlohmann::json j = nlohmann::json::parse(str, nullptr, false, true);
	for (const auto &e : j) {
		const auto &rgb = e["rgb"];
		if (rgb.empty()) {
			continue;
		}
		if (rgb.size() < 3 || rgb.size() > 4) {
			continue;
		}
		core::RGBA rgba;
		{
			int idx = 0;
			glm::ivec4 color(0, 0, 0, 255);
			for (const auto &c : rgb) {
				color[idx++] = c.get<int>();
			}
			rgba.r = color[0];
			rgba.g = color[1];
			rgba.b = color[2];
			rgba.a = color[3];
		}

		const auto &blocks = e["blocks"];
		if (blocks.empty()) {
			continue;
		}
		for (const auto &block : blocks) {
			const std::string blockId = block.get<std::string>();
			if (findPaletteIndex(blockId.c_str()) == -1) {
				int palMatch = pal.getClosestMatch(rgba);
				// TODO: alpha
				Log::printf("\tMCENTRY(\"%s\", %i, 0xFF),                   \\\n", blockId.c_str(), palMatch);
			} else {
				// Log::error("Found %s", blockId.c_str());
			}
		}
	}
}

TEST_F(MinecraftPaletteMapTest, testNewMcmapColors) {
	io::BufferedReadWriteStream jsonOutput;
	if (core::Process::exec("/home/mgerhardy/bin/mcmap", {"-dumpcolors"}, nullptr, &jsonOutput) != 0) {
		GTEST_SKIP() << "Failed to execute mcmap https://github.com/spoutn1k/mcmap";
		return;
	}
	const core::String str((const char *)jsonOutput.getBuffer(), jsonOutput.pos());
	ASSERT_FALSE(str.empty());
	palette::Palette pal;
	pal.minecraft();
	nlohmann::json j = nlohmann::json::parse(str, nullptr, false, true);
	struct Data {
		core::String colorHex;
		core::RGBA rgba;
		core::String type;
		core::String accent;
	};
	for (auto &[blockId, value] : j.items()) {
		Data data;
		if (value.is_string()) {
			data.colorHex = value.get<std::string>().c_str();
		} else if (value.is_object()) {
			if (value.contains("type")) {
				data.colorHex = value["type"].get<std::string>().c_str();
			}
			if (value.contains("accent")) {
				data.colorHex = value["accent"].get<std::string>().c_str();
			}
			if (value.contains("color")) {
				data.colorHex = value["color"].get<std::string>().c_str();
			}
		}
		if (data.colorHex.empty()) {
			continue;
		}
		data.rgba = core::Color::fromHex(data.colorHex.c_str());

		if (findPaletteIndex(blockId.c_str()) == -1) {
			int palMatch = pal.getClosestMatch(data.rgba);
			// TODO: alpha
			Log::printf("\tMCENTRY(\"%s\", %i, 0xFF),                   \\\n", blockId.c_str(), palMatch);
		} else {
			// Log::error("Found %s", blockId.c_str());
		}
	}
}

} // namespace voxelformat
