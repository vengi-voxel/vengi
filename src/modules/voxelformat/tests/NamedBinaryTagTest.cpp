/**
 * @file
 */

#include "voxelformat/private/NamedBinaryTag.h"
#include "app/tests/AbstractTest.h"
#include "io/BufferedReadWriteStream.h"

namespace voxelformat {

class NamedBinaryTagTest : public app::AbstractTest {};

TEST_F(NamedBinaryTagTest, testWriteRead) {
	io::BufferedReadWriteStream stream;
	{
		priv::NBTCompound compound;
		// this is a workaround for a gcc 5.5 compiler bug
		const priv::NamedBinaryTag floatTag = priv::NamedBinaryTag(1.0f);
		compound.put("Root", floatTag);
		priv::NamedBinaryTag root(core::move(compound));
		ASSERT_TRUE(priv::NamedBinaryTag::write(root, "rootTagName", stream));
	}
	stream.seek(0);
	{
		priv::NamedBinaryTagContext ctx;
		ctx.stream = &stream;
		const priv::NamedBinaryTag &root = priv::NamedBinaryTag::parse(ctx);
		EXPECT_FLOAT_EQ(1.0f, root.get("Root").float32());
	}
}

} // namespace voxelformat
