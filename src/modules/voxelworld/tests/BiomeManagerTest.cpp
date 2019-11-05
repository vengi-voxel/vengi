/**
 * @file
 */

#include "AbstractVoxelTest.h"
#include "voxelworld/WorldMgr.h"

namespace voxelworld {

class BiomeManagerTest: public AbstractVoxelTest {
};

TEST_F(BiomeManagerTest, testInvalid) {
	BiomeManager mgr;
	mgr.init("");
	EXPECT_EQ(nullptr, mgr.addBiome(1, 0, 1.0f, 1.0f, voxel::VoxelType::Wood, 90, 90, 90)) << "invalid lower/height combination is accepted, but shouldn't";
}

TEST_F(BiomeManagerTest, testBasic) {
	BiomeManager mgr;
	mgr.init("");
	EXPECT_NE(nullptr, mgr.addBiome(0, 0, 1.0f, 1.0f, voxel::VoxelType::Wood, 90, 90, 90));
	EXPECT_NE(nullptr, mgr.addBiome(1, 1, 1.0f, 1.0f, voxel::VoxelType::Sand, 90, 90, 90));
	EXPECT_NE(nullptr, mgr.addBiome(2, 2, 1.0f, 1.0f, voxel::VoxelType::Grass, 90, 90, 90));
	EXPECT_NE(nullptr, mgr.addBiome(3, 3, 1.0f, 1.0f, voxel::VoxelType::Rock, 90, 90, 90));

	const voxel::VoxelType sand1 = mgr.getBiome(glm::ivec3(0, 5, 0))->type;
	const voxel::VoxelType sand2 = mgr.getBiome(glm::ivec3(0, 6, 0))->type;
	EXPECT_FALSE(voxel::isSand(sand1)) << "Got " << voxel::VoxelTypeStr[(int)sand1] << " but expected to get " << voxel::VoxelTypeStr[(int)voxel::VoxelType::Sand];
	EXPECT_FALSE(voxel::isSand(sand2)) << "Got " << voxel::VoxelTypeStr[(int)sand2] << " but expected to get " << voxel::VoxelTypeStr[(int)voxel::VoxelType::Sand];

	const Biome* biome1 = mgr.getBiome(glm::ivec3(0, 0, 0));
	EXPECT_TRUE(voxel::isWood(biome1->type)) << "y:0 - biome position doesn't lead to wood but: " << voxel::VoxelTypeStr[(int)biome1->voxel().getMaterial()];

	const Biome* biome2 = mgr.getBiome(glm::ivec3(0, 1, 0));
	EXPECT_TRUE(voxel::isSand(biome2->type)) << "y:1 - biome position doesn't lead to sand but: " << voxel::VoxelTypeStr[(int)biome2->voxel().getMaterial()];

	const Biome* biome3 = mgr.getBiome(glm::ivec3(0, 2, 0));
	EXPECT_TRUE(voxel::isGrass(biome3->type)) << "y:2 - biome position doesn't lead to grass but: " << voxel::VoxelTypeStr[(int)biome3->voxel().getMaterial()];

	const Biome* biome4 = mgr.getBiome(glm::ivec3(0, 3, 0));
	EXPECT_TRUE(voxel::	isRock(biome4->type)) << "y:3 - biome position doesn't lead to rock but: " << voxel::VoxelTypeStr[(int)biome4->voxel().getMaterial()];
}

TEST_F(BiomeManagerTest, testHumidityTemperature) {
	BiomeManager mgr;
	mgr.init("");
	const glm::ivec3 p1(1, 0, 1);
	const float h1 = mgr.getHumidity(p1.x, p1.z);
	const float t1 = mgr.getTemperature(p1.x, p1.z);

	const glm::ivec3 p2(10, 0, 10);
	const float h2 = mgr.getHumidity(p2.x, p2.z);
	const float t2 = mgr.getTemperature(p2.x, p2.z);

	const glm::ivec3 p3(20, 0, 20);
	const float h3 = mgr.getHumidity(p3.x, p3.z);
	const float t3 = mgr.getTemperature(p3.x, p3.z);

	EXPECT_NE(nullptr, mgr.addBiome(0, 1, h1, t1, voxel::VoxelType::Grass, 50, 50, 50));
	EXPECT_NE(nullptr, mgr.addBiome(0, 1, h2, t2, voxel::VoxelType::Rock, 150, 150, 150));
	EXPECT_NE(nullptr, mgr.addBiome(0, 1, h3, t3, voxel::VoxelType::Sand, 500, 500, 500));

	EXPECT_EQ(voxel::VoxelType::Grass, mgr.getBiome(p1)->type);
	EXPECT_EQ(voxel::VoxelType::Rock, mgr.getBiome(p2)->type);
	EXPECT_EQ(voxel::VoxelType::Sand, mgr.getBiome(p3)->type);
}

TEST_F(BiomeManagerTest, testLoadLUA) {
	BiomeManager mgr;
	const io::FilesystemPtr& filesystem = _testApp->filesystem();
	ASSERT_TRUE(mgr.init(filesystem->load("biomes.lua")));
}

TEST_F(BiomeManagerTest, testCityGradient) {
	const char *str = R"(function initBiomes()
		local biome = biomeMgr.addBiome(0, 512, 0.5, 0.5, "Grass", 90, 90, 90)
		biomeMgr.setDefault(biome)
		biomeMgr.addBiome(0, 512, 0.5, 0.5, "Grass", 90, 90, 90, true)
	end

	function initCities()
		biomeMgr.addCity(ivec2.new(0, 0), 1000.0)
	end)";
	BiomeManager mgr;
	ASSERT_TRUE(mgr.init(str));
	EXPECT_DOUBLE_EQ(0.0f, mgr.getCityMultiplier(glm::ivec2(0)))
		<< "The center of the city should have a very small modifier";
	EXPECT_DOUBLE_EQ(1.0f, mgr.getCityMultiplier(glm::ivec2(1000, 0)))
		<< "Out of the radius of the city - here we should not have any influence on the height anymore";
	EXPECT_DOUBLE_EQ(1.0f, mgr.getCityMultiplier(glm::ivec2(1000, 1000)))
		<< "Out of the radius of the city - here we should not have any influence on the height anymore";
	EXPECT_DOUBLE_EQ(1.0f, mgr.getCityMultiplier(glm::ivec2(0, 1000)))
		<< "Out of the radius of the city - here we should not have any influence on the height anymore";
	EXPECT_DOUBLE_EQ(1.0f, mgr.getCityMultiplier(glm::ivec2(2000, 2000)))
		<< "Out of the radius of the city - here we should not have any influence on the height anymore";
}

}
