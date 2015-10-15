#include "core/tests/AbstractTest.h"
#include "voxel/World.h"

namespace voxel {
namespace {
const long seed = 1L;
const int size = 32;
}

class WorldTest: public core::AbstractTest {
};

TEST_F(WorldTest, testCreateSaveLoad) {
	World world;
	world.create(seed, size);
	ASSERT_TRUE(world.isCreated());
	ASSERT_EQ(seed, world.seed());
	ASSERT_EQ(size, world.size());
	ASSERT_TRUE(world.save(seed));
	ASSERT_EQ(seed, world.seed());
	ASSERT_EQ(size, world.size());
	ASSERT_TRUE(world.load(seed));
	ASSERT_TRUE(world.isCreated());
	ASSERT_EQ(seed, world.seed());
	ASSERT_EQ(size, world.size());
}

TEST_F(WorldTest, testLoad) {
	World world;
	ASSERT_TRUE(world.load(seed));
	ASSERT_TRUE(world.isCreated());
	ASSERT_EQ(seed, world.seed());
	ASSERT_EQ(size, world.size());
}

}
