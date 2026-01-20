#include <gtest/gtest.h>
#include "io/FormatDescription.h"

namespace io {

class FormatDescriptionTest: public testing::Test {
};

TEST_F(FormatDescriptionTest, testIsA) {
	EXPECT_TRUE(isA("image.png", io::format::images()));
	EXPECT_FALSE(isA("something.else", io::format::lua()));
}

TEST_F(FormatDescriptionTest, testIsImage) {
	ASSERT_TRUE(isImage("foobar.PNG"));
	ASSERT_TRUE(isImage("foobar.png"));
	ASSERT_FALSE(isImage("foobar.foo"));
}

TEST_F(FormatDescriptionTest, testCreateGroupPattern) {
	const FormatDescription desc[] = {
		{"Portable Network Graphics", "", {"png"}, {}, 0u},
		{"JPEG", "", {"jpeg", "jpg"}, {}, 0u},
		{"Portable Anymap", "", {"pnm"}, {}, 0u},
		{"Qubicle Binary", "", {"qb"}, {}, 0u},
		{"MagicaVoxel", "", {"vox"}, {}, 0u},
		{"Qubicle Binary Tree", "", {"qbt"}, {}, 0u},
		{"Qubicle Project", "", {"qbcl"}, {}, 0u},
		{"Sandbox VoxEdit Tilemap", "", {"vxt"}, {}, 0u},
		{"Sandbox VoxEdit Collection", "", {"vxc"}, {}, 0u},
		{"Sandbox VoxEdit Model", "", {"vxm"}, {}, 0u},
		{"Sandbox VoxEdit Hierarchy", "", {"vxr"}, {}, 0u},
		{"BinVox", "", {"binvox"}, {}, 0u},
		{"Goxel", "", {"gox"}, {}, 0u},
		{"CubeWorld", "", {"cub"}, {}, 0u},
		{"Minecraft region", "", {"mca", "mcr"}, {}, 0u},
		{"Minecraft level dat", "", {"dat"}, {}, 0u},
		{"Minecraft schematic", "", {"schematic", "schem", "nbt", "litematic"}, {}, 0u},
		{"Sproxel csv", "", {"csv"}, {}, 0u},
		{"Wavefront Object", "", {"obj"}, {}, 0u},
		{"GL Transmission Format", "", {"gltf", "glb"}, {}, 0u},
		{"Standard Triangle Language", "", {"stl"}, {}, 0u},
		{"Build engine", "", {"kvx"}, {}, 0u},
		{"AceOfSpades", "", {"kv6"}, {}, 0u},
		{"Tiberian Sun", "", {"vxl"}, {}, 0u},
		{"AceOfSpades", "", {"vxl"}, {}, 0u},
		{"Qubicle Exchange", "", {"qef"}, {}, 0u},
		{"Chronovox", "", {"csm"}, {}, 0u},
		{"Nicks Voxel Model", "", {"nvm"}, {}, 0u},
		{"SLAB6 vox", "", {"vox"}, {}, 0u},
		FormatDescription::END
	};
	core::DynamicArray<io::FormatDescription> groups;
	createGroupPatterns(desc, groups);
	ASSERT_EQ(5u, groups.size()) << "Unexpected amount of groups: " << groups.size();
	EXPECT_EQ("AceOfSpades", groups[0].name);
	EXPECT_EQ("kv6", groups[0].exts[0]);
	EXPECT_EQ("vxl", groups[0].exts[1]);
	EXPECT_EQ("Minecraft", groups[1].name);
	EXPECT_EQ("dat", groups[1].exts[0]);
	EXPECT_EQ("mca", groups[1].exts[1]);
	EXPECT_EQ("mcr", groups[1].exts[2]);
	EXPECT_EQ("Portable", groups[2].name);
	EXPECT_EQ("pnm", groups[2].exts[0]);
	EXPECT_EQ("png", groups[2].exts[1]);
	EXPECT_EQ("Qubicle", groups[3].name);
	EXPECT_EQ("qb", groups[3].exts[0]);
	EXPECT_EQ("qbt", groups[3].exts[1]);
	EXPECT_EQ("qef", groups[3].exts[2]);
}

TEST_F(FormatDescriptionTest, testConvertToAllFilePattern) {
	const FormatDescription desc[] = {
		{"Portable Network Graphics", "", {"png"}, {}, 0u},
		{"JPEG", "", {"jpeg", "jpg"}, {}, 0u},
		{"Portable Anymap", "", {"pnm"}, {}, 0u},
		FormatDescription::END
	};
	const core::String &all = convertToAllFilePattern(desc);
	ASSERT_EQ("*.png,*.jpeg,*.jpg,*.pnm", all);
}

TEST_F(FormatDescriptionTest, testConvertToFilePattern) {
	const FormatDescription desc1 = {"Name", "", {"ext1"}, {}, 0u};
	const FormatDescription desc2 = {"Name", "", {"ext1", "ext2"}, {}, 0u};

	ASSERT_EQ("Name (*.ext1)", convertToFilePattern(desc1));
	ASSERT_EQ("Name (*.ext1,*.ext2)", convertToFilePattern(desc2));
}

}
