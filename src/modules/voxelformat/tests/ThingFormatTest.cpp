/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "math/tests/TestMathHelper.h"
#include "voxelformat/private/rooms/ThingNodeParser.h"

namespace voxelformat {

class ThingNodeParserTest : public app::AbstractTest {};

TEST_F(ThingNodeParserTest, testParseNode) {
	ThingNodeParser parser;
	NodeSpec nodeSpec;
	const core::String node = fileToString("testrooms.node");
	ASSERT_TRUE(parser.parseNode(node, nodeSpec));
	ASSERT_EQ(nodeSpec.color, color::RGBA(0xaa, 0xbb, 0xcc, 255));
	ASSERT_EQ(nodeSpec.modelName, "root.vox");
	ASSERT_EQ(nodeSpec.name, "root");
	ASSERT_VEC_NEAR(nodeSpec.localPos, glm::vec3(0.0f, 0.0f, 0.0f), 0.000001f);
	ASSERT_VEC_NEAR(nodeSpec.localRot, glm::vec3(0.0f, 180.0f, 0.0f), 0.000001f);
	ASSERT_NEAR(nodeSpec.opacity, 1.0f, 0.000001f);
	ASSERT_EQ(nodeSpec.thingLibraryId, "abcdefgh");
	ASSERT_EQ(nodeSpec.children.size(), 4u);
	ASSERT_EQ(nodeSpec.children[0].name, "child 1");
	ASSERT_EQ(nodeSpec.children[0].modelName, "child1.vox");
	ASSERT_EQ(nodeSpec.children[0].thingLibraryId, "ghijklmn");
	ASSERT_EQ(nodeSpec.children[0].color, color::RGBA(0xaa, 0, 0, 255));
	ASSERT_VEC_NEAR(nodeSpec.children[0].localPos, glm::vec3(1.0f, 2.0f, 3.0f), 0.000001f);
	ASSERT_VEC_NEAR(nodeSpec.children[0].localRot, glm::vec3(4.0f, 5.0f, 6.0f), 0.000001f);
	ASSERT_NEAR(nodeSpec.children[0].opacity, 1.0f, 0.000001f);
	ASSERT_EQ(nodeSpec.children[0].children.size(), 0u);
	ASSERT_EQ(nodeSpec.children[1].name, "child 2");
	ASSERT_EQ(nodeSpec.children[1].modelName, "child2.vox");
	ASSERT_EQ(nodeSpec.children[1].color, color::RGBA(0, 0xaa, 0, 255));
	ASSERT_VEC_NEAR(nodeSpec.children[1].localPos, glm::vec3(7.0f, 8.0f, 9.0f), 0.000001f);
	ASSERT_VEC_NEAR(nodeSpec.children[1].localRot, glm::vec3(10.0f, 11.0f, 12.0f), 0.000001f);
	ASSERT_NEAR(nodeSpec.children[1].opacity, 1.0f, 0.000001f);
	ASSERT_EQ(nodeSpec.children[1].children.size(), 0u);
	ASSERT_EQ(nodeSpec.children[2].name, "child 3 with own children");
	ASSERT_EQ(nodeSpec.children[2].modelName, "child3.vox");
	ASSERT_EQ(nodeSpec.children[2].thingLibraryId, "12345678");
	ASSERT_EQ(nodeSpec.children[2].color, color::RGBA(0, 0, 0xaa, 255));
	ASSERT_VEC_NEAR(nodeSpec.children[2].localPos, glm::vec3(13.0f, 14.0f, 15.0f), 0.000001f);
	ASSERT_VEC_NEAR(nodeSpec.children[2].localRot, glm::vec3(16.0f, 17.0f, 18.0f), 0.000001f);
	ASSERT_NEAR(nodeSpec.children[2].opacity, 1.0f, 0.000001f);
	ASSERT_EQ(nodeSpec.children[2].children.size(), 1u);
	ASSERT_EQ(nodeSpec.children[2].children[0].name, "child 1 of child 3");
	ASSERT_EQ(nodeSpec.children[2].children[0].modelName, "child1ofchild3.vox");
	ASSERT_EQ(nodeSpec.children[2].children[0].thingLibraryId, "87654321");
}

class ThingFormatTest : public AbstractFormatTest {};

TEST_F(ThingFormatTest, testLoad) {
	testLoad("foo.thing", 1);
}

} // namespace voxelformat
