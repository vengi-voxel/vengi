/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "voxelformat/private/mesh/BlendShared.h"

namespace voxelformat {

class BlendFormatTest : public AbstractFormatTest {};

TEST_F(BlendFormatTest, testLoad) {
	testLoad("blender-tests-data-cubes-hierarchy.blend", 22);
}

TEST_F(BlendFormatTest, testField) {
	Field field;
	field.name = "foo[1024]";
	ASSERT_TRUE(field.isArray());
	ASSERT_FALSE(field.isPointer());
	field.name = "*foo[1024]";
	ASSERT_TRUE(field.isArray());
	ASSERT_TRUE(field.isPointer());
}

TEST_F(BlendFormatTest, testCalcSize) {
	Type type;
	type.size = 4;
	type.name = "fake";

	{
		Field field;
		field.name = "foo[1024]";
		calcSize(field, type, false);
		EXPECT_EQ(type.size * 1024, field.size);
	}
	{
		Field field;
		field.name = "*foo[1024]";
		calcSize(field, type, false);
		EXPECT_EQ(4 * 1024, field.size);
	}
	{
		Field field;
		field.name = "*foo[1024]";
		calcSize(field, type, true);
		EXPECT_EQ(8 * 1024, field.size);
	}
	{
		Field field;
		field.name = "foo";
		calcSize(field, type, false);
		EXPECT_EQ(type.size, field.size);
	}
}

} // namespace voxelformat
