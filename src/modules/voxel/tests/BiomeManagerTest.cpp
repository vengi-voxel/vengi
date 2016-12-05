/**
 * @file
 */

#include "AbstractVoxelTest.h"
#include "voxel/World.h"

namespace voxel {

class BiomeManagerTest: public AbstractVoxelTest {
};

TEST_F(BiomeManagerTest, testBasic) {
	BiomeManager mgr;
	ASSERT_TRUE(mgr.init());
	ASSERT_TRUE(mgr.addBiom(0, 1, 1.0f, 1.0f, VoxelType::Wood));
	ASSERT_TRUE(mgr.addBiom(1, 2, 1.0f, 1.0f, VoxelType::Sand));
	ASSERT_TRUE(mgr.addBiom(2, 3, 1.0f, 1.0f, VoxelType::Grass));
	ASSERT_TRUE(mgr.addBiom(3, 4, 1.0f, 1.0f, VoxelType::Rock));

	ASSERT_FALSE(isSand(mgr.getBiome(glm::ivec3(0, 5, 0))->type));
	ASSERT_FALSE(isSand(mgr.getBiome(glm::ivec3(0, 6, 0))->type));

	for (int i = 0; i <= 4; ++i) {
		const glm::ivec3 pos(0, i, 0);
		const Biome* biome = mgr.getBiome(pos);
		ASSERT_TRUE(isSand(biome->type)) << glm::to_string(pos) << " biome position doesn't lead to sand " << biome->voxel();
	}
}

TEST_F(BiomeManagerTest, testHumidityTemperature) {
	BiomeManager mgr;
	ASSERT_TRUE(mgr.init());
	const glm::ivec3 p1(1, 0, 1);
	const float h1 = mgr.getHumidity(p1);
	const float t1 = mgr.getTemperature(p1);

	const glm::ivec3 p2(10, 0, 10);
	const float h2 = mgr.getHumidity(p2);
	const float t2 = mgr.getTemperature(p2);

	const glm::ivec3 p3(20, 0, 20);
	const float h3 = mgr.getHumidity(p3);
	const float t3 = mgr.getTemperature(p3);

	ASSERT_TRUE(mgr.addBiom(0, 1, h1, t1, VoxelType::Grass));
	ASSERT_TRUE(mgr.addBiom(0, 1, h2, t2, VoxelType::Rock));
	ASSERT_TRUE(mgr.addBiom(0, 1, h3, t3, VoxelType::Sand));

	ASSERT_EQ(VoxelType::Grass, mgr.getBiome(p1)->type);
	ASSERT_EQ(VoxelType::Rock, mgr.getBiome(p2)->type);
	ASSERT_EQ(VoxelType::Sand, mgr.getBiome(p3)->type);
}

}
