#include <gtest/gtest.h>
#include "io/FormatDescription.h"

namespace io {

class FormatDescriptionTest: public testing::Test {
};

TEST_F(FormatDescriptionTest, testIsImage) {
	ASSERT_TRUE(isImage("foobar.PNG"));
	ASSERT_TRUE(isImage("foobar.png"));
	ASSERT_FALSE(isImage("foobar.foo"));
}

TEST_F(FormatDescriptionTest, testCreateGroupPattern) {
	const FormatDescription desc[] = {
		{"Portable Network Graphics", {"png"}, nullptr, 0u},
		{"JPEG", {"jpeg", "jpg"}, nullptr, 0u},
		{"Portable Anymap", {"pnm"}, nullptr, 0u},
		{"Qubicle Binary", {"qb"}, nullptr, 0u},
		{"MagicaVoxel", {"vox"}, nullptr, 0u},
		{"Qubicle Binary Tree", {"qbt"}, nullptr, 0u},
		{"Qubicle Project", {"qbcl"}, nullptr, 0u},
		{"Sandbox VoxEdit Tilemap", {"vxt"}, nullptr, 0u},
		{"Sandbox VoxEdit Collection", {"vxc"}, nullptr, 0u},
		{"Sandbox VoxEdit Model", {"vxm"}, nullptr, 0u},
		{"Sandbox VoxEdit Hierarchy", {"vxr"}, nullptr, 0u},
		{"BinVox", {"binvox"}, nullptr, 0u},
		{"Goxel", {"gox"}, nullptr, 0u},
		{"CubeWorld", {"cub"}, nullptr, 0u},
		{"Minecraft region", {"mca", "mcr"}, nullptr, 0u},
		{"Minecraft level dat", {"dat"}, nullptr, 0u},
		{"Minecraft schematic", {"schematic", "schem", "nbt"}, nullptr, 0u},
		{"Sproxel csv", {"csv"}, nullptr, 0u},
		{"Wavefront Object", {"obj"}, nullptr, 0u},
		{"GL Transmission Format", {"gltf", "glb"}, nullptr, 0u},
		{"Standard Triangle Language", {"stl"}, nullptr, 0u},
		{"Build engine", {"kvx"}, nullptr, 0u},
		{"AceOfSpades", {"kv6"}, nullptr, 0u},
		{"Tiberian Sun", {"vxl"}, nullptr, 0u},
		{"AceOfSpades", {"vxl"}, nullptr, 0u},
		{"Qubicle Exchange", {"qef"}, nullptr, 0u},
		{"Chronovox", {"csm"}, nullptr, 0u},
		{"Nicks Voxel Model", {"nvm"}, nullptr, 0u},
		{"SLAB6 vox", {"vox"}, nullptr, 0u},
		{"", {}, nullptr, 0u}
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
		{"Portable Network Graphics", {"png"}, nullptr, 0u},
		{"JPEG", {"jpeg", "jpg"}, nullptr, 0u},
		{"Portable Anymap", {"pnm"}, nullptr, 0u},
		{"", {}, nullptr, 0u}
	};
	const core::String &all = convertToAllFilePattern(desc);
	ASSERT_EQ("*.png,*.jpeg,*.jpg,*.pnm", all);
}

TEST_F(FormatDescriptionTest, testConvertToFilePattern) {
	const FormatDescription desc1 = {"Name", {"ext1"}, nullptr, 0u};
	const FormatDescription desc2 = {"Name", {"ext1", "ext2"}, nullptr, 0u};

	ASSERT_EQ("Name (*.ext1)", convertToFilePattern(desc1));
	ASSERT_EQ("Name (*.ext1,*.ext2)", convertToFilePattern(desc2));
}

}
