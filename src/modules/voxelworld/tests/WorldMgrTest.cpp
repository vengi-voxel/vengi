/**
 * @file
 */

#include "AbstractVoxelTest.h"
#include "voxelworld/WorldMgr.h"
#include "voxelworld/WorldPager.h"
#include "engine-config.h"
#include <chrono>
#include <string>

namespace voxelworld {

class WorldMgrTest: public AbstractVoxelTest {
private:
	int _chunkMeshPositionTest = 0;
protected:
	void extract(int expected) {
		const voxelformat::VolumeCachePtr& volumeCache = std::make_shared<voxelformat::VolumeCache>();
		WorldPagerPtr pager = std::make_shared<WorldPager>(volumeCache, std::make_shared<voxelworld::ChunkPersister>());
		pager->setSeed(0);

		WorldMgr world(pager);
		world.setSeed(0);
		ASSERT_TRUE(world.init());

		const io::FilesystemPtr& filesystem = io::filesystem();
		ASSERT_TRUE(pager->init(world.volumeData(), filesystem->load("worldparams.lua"), filesystem->load("biomes.lua")));
		core::Var::get(cfg::VoxelMeshSize, "16", core::CV_READONLY);

		for (int i = 0; i < expected; ++i) {
			const glm::ivec3 pos { i * 1024, 0, i };
			ASSERT_TRUE(world.scheduleMeshExtraction(pos)) << "Failed to schedule mesh extraction for " << glm::to_string(pos);
		}

		int meshes;
		int extracted;
		int pending;
		world.stats(meshes, extracted, pending);

		auto start = std::chrono::high_resolution_clock::now();
		while (pending <= 0) {
			ChunkMeshes meshData(0, 0, 0, 0);
			while (!world.pop(meshData)) {
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
#if USE_GPROF == 0
				auto end = std::chrono::high_resolution_clock::now();
				std::chrono::duration<double, std::milli> elapsed = end - start;
				const double millis = elapsed.count();
				ASSERT_LT(millis, 120 * 1000) << "Took too long to got a finished mesh from the queue";
#endif
			}
			world.stats(meshes, extracted, pending);
		}
		pager->shutdown();
		world.shutdown();
	}

	void chunkMeshPositionTest(
			const WorldMgr& world,
			int worldX, int worldY, int worldZ,
			int chunkX, int chunkY, int chunkZ,
			int meshX,  int meshY,  int meshZ
			) {
		SCOPED_TRACE("Testcase call: " + std::to_string(++_chunkMeshPositionTest));

		const glm::ivec3 vec(worldX, worldY, worldZ);

		const glm::ivec3& chunkPos = world.chunkPos(vec);
		ASSERT_EQ(glm::ivec3(chunkX, chunkY, chunkZ), chunkPos)
			<< "Chunk position doesn't match the expected for chunk size: " << world.chunkSize()
			<< " at: " << vec.x << ", " << vec.y << ", " << vec.z;

		const glm::ivec3& meshPos = world.meshPos(vec);
		ASSERT_EQ(glm::ivec3(meshX, meshY, meshZ), meshPos)
			<< "Mesh position doesn't match the expected for mesh size: "
			<< glm::to_string(world.meshSize())
			<< " at: " << vec.x << ", " << vec.y << ", " << vec.z;
	}
};

TEST_F(WorldMgrTest, testExtractionMultiple) {
	extract(4);
}

TEST_F(WorldMgrTest, testExtractionSingle) {
	extract(1);
}

}
