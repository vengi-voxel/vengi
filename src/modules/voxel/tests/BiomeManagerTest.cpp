/**
 * @file
 */

#include "AbstractVoxelTest.h"
#include "voxel/WorldMgr.h"

namespace voxel {

class BiomeManagerTest: public AbstractVoxelTest {
};

TEST_F(BiomeManagerTest, testInvalid) {
	BiomeManager mgr;
	mgr.init("");
	EXPECT_EQ(nullptr, mgr.addBiome(1, 0, 1.0f, 1.0f, VoxelType::Wood)) << "invalid lower/height combination is accepted, but shouldn't";
}

TEST_F(BiomeManagerTest, testBasic) {
	BiomeManager mgr;
	mgr.init("");
	EXPECT_NE(nullptr, mgr.addBiome(0, 0, 1.0f, 1.0f, VoxelType::Wood));
	EXPECT_NE(nullptr, mgr.addBiome(1, 1, 1.0f, 1.0f, VoxelType::Sand));
	EXPECT_NE(nullptr, mgr.addBiome(2, 2, 1.0f, 1.0f, VoxelType::Grass));
	EXPECT_NE(nullptr, mgr.addBiome(3, 3, 1.0f, 1.0f, VoxelType::Rock));

	const VoxelType sand1 = mgr.getBiome(glm::ivec3(0, 5, 0))->type;
	const VoxelType sand2 = mgr.getBiome(glm::ivec3(0, 6, 0))->type;
	EXPECT_FALSE(isSand(sand1)) << "Got " << voxel::VoxelTypeStr[(int)sand1] << " but expected to get " << voxel::VoxelTypeStr[(int)VoxelType::Sand];
	EXPECT_FALSE(isSand(sand2)) << "Got " << voxel::VoxelTypeStr[(int)sand2] << " but expected to get " << voxel::VoxelTypeStr[(int)VoxelType::Sand];

	const Biome* biome1 = mgr.getBiome(glm::ivec3(0, 0, 0));
	EXPECT_TRUE(isWood(biome1->type)) << "y:0 - biome position doesn't lead to wood but: " << biome1->voxel();

	const Biome* biome2 = mgr.getBiome(glm::ivec3(0, 1, 0));
	EXPECT_TRUE(isSand(biome2->type)) << "y:1 - biome position doesn't lead to sand but: " << biome2->voxel();

	const Biome* biome3 = mgr.getBiome(glm::ivec3(0, 2, 0));
	EXPECT_TRUE(isGrass(biome3->type)) << "y:2 - biome position doesn't lead to grass but: " << biome3->voxel();

	const Biome* biome4 = mgr.getBiome(glm::ivec3(0, 3, 0));
	EXPECT_TRUE(isRock(biome4->type)) << "y:3 - biome position doesn't lead to rock but: " << biome4->voxel();
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

	EXPECT_NE(nullptr, mgr.addBiome(0, 1, h1, t1, VoxelType::Grass));
	EXPECT_NE(nullptr, mgr.addBiome(0, 1, h2, t2, VoxelType::Rock));
	EXPECT_NE(nullptr, mgr.addBiome(0, 1, h3, t3, VoxelType::Sand));

	EXPECT_EQ(VoxelType::Grass, mgr.getBiome(p1)->type);
	EXPECT_EQ(VoxelType::Rock, mgr.getBiome(p2)->type);
	EXPECT_EQ(VoxelType::Sand, mgr.getBiome(p3)->type);
}

TEST_F(BiomeManagerTest, testLoadLUA) {
	BiomeManager mgr;
	const io::FilesystemPtr& filesystem = _testApp->filesystem();
	ASSERT_TRUE(mgr.init(filesystem->load("biomes.lua")));
}

TEST_F(BiomeManagerTest, testCityGradient) {
	const char *str = R"(function initBiomes()
		local biome = biomeMgr.addBiome(0, 512, 0.5, 0.5, "Grass", underGround)
		biomeMgr.setDefault(biome)
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
