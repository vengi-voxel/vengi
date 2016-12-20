/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "frontend/WorldRenderer.h"

namespace frontend {

class WorldRendererTest: public core::AbstractTest {
public:
	class T_WorldRenderer: public WorldRenderer {
		FRIEND_TEST(WorldRendererTest, testDistanceCulling);
	};
	voxel::WorldPtr _world;
	T_WorldRenderer* _renderer;
	WorldRenderer *_worldRenderer;

	virtual void SetUp() override {
		core::AbstractTest::SetUp();
		_world = std::make_shared<voxel::World>();
		_worldRenderer = new WorldRenderer(_world);
		_renderer = static_cast<T_WorldRenderer*>(_worldRenderer);
	}

	virtual void TearDown() override {
		delete _renderer;
	}
};

TEST_F(WorldRendererTest, testCreate) {
	_world->setPersist(false);
	ASSERT_TRUE(_renderer->extractNewMeshes(glm::ivec3(0), true));
	voxel::ChunkMeshData mesh(0, 0, 0, 0);
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
