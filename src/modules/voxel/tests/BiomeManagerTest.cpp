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
	ASSERT_TRUE(mgr.addBiom(0, 1, 1.0f, 1.0f, createVoxel(VoxelType::Wood, 0)));
	ASSERT_TRUE(mgr.addBiom(1, 2, 1.0f, 1.0f, createVoxel(VoxelType::Sand, 0)));
	ASSERT_TRUE(mgr.addBiom(2, 3, 1.0f, 1.0f, createVoxel(VoxelType::Grass, 0)));
	ASSERT_TRUE(mgr.addBiom(3, 4, 1.0f, 1.0f, createVoxel(VoxelType::Rock, 0)));

	ASSERT_FALSE(isSand(mgr.getBiome(glm::ivec3(0, 5, 0))->voxel.getMaterial()));
	ASSERT_FALSE(isSand(mgr.getBiome(glm::ivec3(0, 6, 0))->voxel.getMaterial()));

	for (int i = 0; i <= 4; ++i) {
		ASSERT_TRUE(isSand(mgr.getBiome(glm::ivec3(0, i, 0))->voxel.getMaterial()));
	}
}

TEST_F(BiomeManagerTest, testHumidityTemperature) {
	BiomeManager mgr;
	const glm::ivec3 p1(1, 0, 1);
	const float h1 = mgr.getHumidity(p1);
	const float t1 = mgr.getTemperature(p1);

	const glm::ivec3 p2(10, 0, 10);
	const float h2 = mgr.getHumidity(p2);
	const float t2 = mgr.getTemperature(p2);

	const glm::ivec3 p3(20, 0, 20);
	const float h3 = mgr.getHumidity(p3);
	const float t3 = mgr.getTemperature(p3);

	ASSERT_TRUE(mgr.addBiom(0, 1, h1, t1, createVoxel(VoxelType::Grass, 0)));
	ASSERT_TRUE(mgr.addBiom(0, 1, h2, t2, createVoxel(VoxelType::Rock, 0)));
	ASSERT_TRUE(mgr.addBiom(0, 1, h3, t3, createVoxel(VoxelType::Sand, 0)));

	ASSERT_EQ(VoxelType::Grass, mgr.getBiome(p1)->voxel.getMaterial());
	ASSERT_EQ(VoxelType::Rock, mgr.getBiome(p2)->voxel.getMaterial());
	ASSERT_EQ(VoxelType::Sand, mgr.getBiome(p3)->voxel.getMaterial());
}

}
