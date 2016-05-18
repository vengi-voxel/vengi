/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "frontend/WorldRenderer.h"

namespace frontend {

class WorldRendererTest: public core::AbstractTest {
};

TEST_F(WorldRendererTest, testCreate) {
	const voxel::WorldPtr& world = std::make_shared<voxel::World>();
	WorldRenderer renderer(world);
	ASSERT_TRUE(renderer.extractNewMeshes(glm::ivec3(0), true));
	voxel::DecodedMeshData mesh;
	int amount = 0;
	while (!world->pop(mesh)) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		++amount;
		int meshes, extracted, pending;
		world->stats(meshes, extracted, pending);
		ASSERT_TRUE(amount < 100) << "Took too long to generate the chunks. Pending: " << pending << ", extracted: " << extracted << ", meshes: " << meshes;
	}
	ASSERT_GT(mesh.mesh.getNoOfVertices(), 0);
	ASSERT_GT(mesh.mesh.getNoOfIndices(), 0);
}

}
