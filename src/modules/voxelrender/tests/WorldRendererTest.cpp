/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "voxelrender/WorldRenderer.h"
#include "voxelworld/WorldMgr.h"
#include "voxelworld/WorldPager.h"

namespace voxelrender {

class WorldRendererTest: public core::AbstractTest {
public:
	class T_WorldRenderer: public WorldRenderer {
		FRIEND_TEST(WorldRendererTest, testDistanceCulling);
	};
	voxelworld::WorldMgrPtr _world;
	voxelworld::WorldPagerPtr _worldPager;
	T_WorldRenderer* _renderer;
	WorldRenderer *_worldRenderer;

	virtual void SetUp() override {
		core::AbstractTest::SetUp();
		core::Var::get(cfg::VoxelMeshSize, "16", core::CV_READONLY);
		const voxelformat::VolumeCachePtr& volumeCache = std::make_shared<voxelformat::VolumeCache>();
		_worldPager = std::make_shared<voxelworld::WorldPager>(volumeCache);
		_world = std::make_shared<voxelworld::WorldMgr>(_worldPager);
		ASSERT_TRUE(voxel::initDefaultMaterialColors());
		const std::string& world = io::filesystem()->load("worldparams.lua");
		ASSERT_NE("", world);
		const std::string& biomes = io::filesystem()->load("biomes.lua");
		ASSERT_NE("", biomes);
		ASSERT_TRUE(_world->init());
		ASSERT_TRUE(_worldPager->init(_world->volumeData(), world, biomes));

		_worldRenderer = new WorldRenderer(_world);
		_renderer = static_cast<T_WorldRenderer*>(_worldRenderer);
	}

	virtual void TearDown() override {
		delete _renderer;
		_worldPager->shutdown();
		_world->shutdown();
	}
};

TEST_F(WorldRendererTest, testCreate) {
	ASSERT_TRUE(_world);
	_worldPager->setPersist(false);
	ASSERT_NE(nullptr, _renderer);
	video::Camera camera;
	camera.init(glm::ivec2(0), glm::ivec2(1024, 1024), glm::ivec2(1024, 1024));
	camera.setOmega(glm::vec3(0.0f, 0.1f, 0.0f));
	camera.setPosition(glm::zero<glm::vec3>());
	camera.lookAt(glm::vec3(10.0f, 70.0f, 10.0f));
	camera.setNearPlane(0.05f);
	camera.setFarPlane(40.0f);
	camera.update(0l);

	_renderer->extractMeshes(camera);
	voxelworld::ChunkMeshes mesh(0, 0, 0, 0);
	int amount = 0;
	while (!_world->pop(mesh)) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		++amount;
		int meshes, extracted, pending;
		_world->stats(meshes, extracted, pending);
		ASSERT_TRUE(amount < 100) << "Took too long to generate the chunks. Pending: " << pending << ", extracted: " << extracted << ", meshes: " << meshes;
	}
	ASSERT_GT(mesh.opaqueMesh.getNoOfVertices(), 0u);
	ASSERT_GT(mesh.opaqueMesh.getNoOfIndices(), 0u);
}

}
