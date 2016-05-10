/**
 * @file
 */

#include "AbstractVoxelTest.h"
#include "voxel/World.h"

namespace voxel {

class BiomeManagerTest: public AbstractVoxelTest {
};

TEST_F(BiomeManagerTest, testFoo) {
	BiomeManager mgr;
	ASSERT_TRUE(mgr.addBiom(0, MAX_WATER_HEIGHT + 1, 0.5f, 0.5f, createVoxel(Sand1)));
	ASSERT_TRUE(mgr.addBiom(MAX_WATER_HEIGHT + 3, MAX_WATER_HEIGHT + 10, 1.0f, 0.7f, createVoxel(Dirt1)));
	ASSERT_TRUE(mgr.addBiom(MAX_WATER_HEIGHT + 3, MAX_TERRAIN_HEIGHT + 1, 0.5f, 0.5f, createVoxel(Grass1)));
	ASSERT_TRUE(mgr.addBiom(MAX_TERRAIN_HEIGHT - 20, MAX_TERRAIN_HEIGHT + 1, 0.4f, 0.5f, createVoxel(Rock1)));
	ASSERT_TRUE(mgr.addBiom(MAX_TERRAIN_HEIGHT - 30, MAX_MOUNTAIN_HEIGHT + 1, 0.32f, 0.32f, createVoxel(Rock2)));

	ASSERT_EQ(nullptr, mgr.getBiome(glm::ivec3(0, MAX_WATER_HEIGHT + 1, 0), 1.0f));

	const Biome* biome;

	biome = mgr.getBiome(glm::ivec3(0, 0, 0), 1.0f);
	ASSERT_NE(nullptr, biome);
	ASSERT_TRUE(isSand(biome->voxel.getMaterial()));
}

}
