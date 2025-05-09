/**
 * @file
 */

#include "voxelformat/private/minecraft/MinecraftPaletteMap.h"
#include "app/App.h"
#include "app/tests/AbstractTest.h"
#include "core/Color.h"
#include "core/Log.h"
#include "core/Process.h"
#include "core/String.h"
#include "core/collection/StringSet.h"
#include "io/BufferedReadWriteStream.h"
#include "io/FileStream.h"
#include "palette/Palette.h"
#include "palette/private/AVMTHelper.h"
#include "json/JSON.h"
#include <glm/vec3.hpp>

namespace voxelformat {

class MinecraftPaletteMapTest : public app::AbstractTest {
protected:
	core::String cleanName(const core::String &blockId) const {
		core::String name = blockId;
		size_t n = name.find("[");
		if (n != core::String::npos) {
			name = name.substr(0, n);
		}
		size_t biome = name.find(",");
		if (biome != core::String::npos) {
			name = name.substr(biome + 1, name.size() - biome - 1);
		}
		size_t pos = name.find(":");
		if (pos == core::String::npos) {
			return name;
		}
		return name.substr(pos + 1, name.size() - pos - 1);
	}
};

TEST_F(MinecraftPaletteMapTest, testParse) {
	EXPECT_EQ(191,
			  findPaletteIndex(
				  "minecraft:dark_oak_stairs[facing=east,half=bottom,shape=outer_left,waterlogged=false][INT] = 554"));
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
		core::StringSet found;
		for (const auto &block : blocks) {
			const std::string blockId = block.get<std::string>();
			if (findPaletteIndex(blockId.c_str()) == -1) {
				int palMatch = pal.getClosestMatch(rgba);
				const core::String shortBlockId = cleanName(blockId.c_str());
				if (!found.insert(shortBlockId)) {
					continue;
				}
				// TODO: after biome removal we have duplicates here
				// TODO: alpha
				Log::printf("\tMCENTRY(\"%s\", %i, 0xFF),                   \\\n", shortBlockId.c_str(), palMatch);
			} else {
				Log::debug("Found %s", blockId.c_str());
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
	core::StringSet found;
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
			const core::String shortBlockId = cleanName(blockId.c_str());
			if (!found.insert(shortBlockId)) {
				continue;
			}
			// TODO: alpha
			Log::printf("\tMCENTRY(\"%s\", %i, 0xFF),                   \\\n", shortBlockId.c_str(), palMatch);
		} else {
			Log::debug("Found %s", blockId.c_str());
		}
	}
}

TEST_F(MinecraftPaletteMapTest, testMaterialComplete) {
	for (int i = 0; i < palette::PaletteMaxColors; ++i) {
		const core::String &blockId = findPaletteName(i);
		EXPECT_FALSE(blockId.empty()) << "Failed to find block id for " << i;
	}
}

TEST_F(MinecraftPaletteMapTest, testAvoydMaterialTemplateColors) {
	const io::FilePtr &file = io::filesystem()->open("Materials_Minecraft_1_21_5.avmt");
	if (!file || !file->exists()) {
		GTEST_SKIP() << "No Materials_Minecraft_1_21_5.avmt found";
		return;
	}
	palette::Palette pal;
	pal.minecraft();

	io::FileStream stream(file);
	core::DynamicArray<palette::AVMTMaterial> materials;
	core::String paletteName;
	ASSERT_TRUE(parseMaterials(stream, materials, paletteName));

	core::StringSet found;
	for (const auto &e : materials) {
		if (findPaletteIndex(e.name.c_str()) == -1) {
			int palMatch = pal.getClosestMatch(e.rgba);
			const core::String shortBlockId = cleanName(e.name);
			if (!found.insert(shortBlockId)) {
				continue;
			}
			// TODO: alpha
			Log::printf("\tMCENTRY(\"%s\", %i, 0xFF),                   \\\n", shortBlockId.c_str(), palMatch);
		} else {
			Log::debug("Found %s", e.name.c_str());
		}
	}
}

} // namespace voxelformat
