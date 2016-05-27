/**
 * @file
 */

#include "AbstractVoxelTest.h"
#include "voxel/polyvox/CubicSurfaceExtractor.h"
#include "voxel/Spiral.h"
#include "voxel/IsQuadNeeded.h"

namespace voxel {

class AmbientOcclusionTest: public AbstractVoxelTest {
protected:
	bool pageIn(const Region& region, PagedVolume::Chunk* chunk) override {
		return true;
	}
};

TEST_F(AmbientOcclusionTest, testAmbientOcclusion) {
	_volData.flushAll();

	_volData.setVoxel(1, 2, 1, createVoxel(21));

	_volData.setVoxel(0, 1, 0, createVoxel(19));
	_volData.setVoxel(1, 1, 0, createVoxel(18));
	_volData.setVoxel(2, 1, 0, createVoxel(17));
	_volData.setVoxel(0, 1, 1, createVoxel(16));
	_volData.setVoxel(1, 1, 1, createVoxel(15));
	_volData.setVoxel(2, 1, 1, createVoxel(14));
	_volData.setVoxel(0, 1, 2, createVoxel(13));
	_volData.setVoxel(1, 1, 2, createVoxel(12));
	_volData.setVoxel(2, 1, 2, createVoxel(11));

	_volData.setVoxel(0, 0, 0, createVoxel(9));
	_volData.setVoxel(1, 0, 0, createVoxel(8));
	_volData.setVoxel(2, 0, 0, createVoxel(7));
	_volData.setVoxel(0, 0, 1, createVoxel(6));
	_volData.setVoxel(1, 0, 1, createVoxel(5));
	_volData.setVoxel(2, 0, 1, createVoxel(4));
	_volData.setVoxel(0, 0, 2, createVoxel(3));
	_volData.setVoxel(1, 0, 2, createVoxel(2));
	_volData.setVoxel(2, 0, 2, createVoxel(1));

	Mesh mesh(1000, 1000);
	extractCubicMesh(&_volData, _ctx.region, &mesh, IsQuadNeeded(false));
	const Vertex* vertices = mesh.getRawVertexData();
	const int amount = mesh.getNoOfVertices();
	// TODO: this was the amount before ao
	//ASSERT_EQ(116, amount);

	// TODO: replace magic constant
	const int noAO = 3;
	for (int i = 0; i < amount; ++i) {
		const Vertex& v = vertices[i];
		const int x = v.position.x;
		const int y = v.position.y;
		const int z = v.position.z;
		if (y == 0 || y == 1 || y == 3) {
			// these two levels don't receive any ao
			EXPECT_EQ(noAO, v.ambientOcclusion) << "Unexpected ao value at y level " << y << " found";
		} else if (y == 2) {
			if (x == 0 || x == 3 || z == 0 || z == 3) {
				// borders of the mesh don't receive any ao
				EXPECT_EQ(noAO, v.ambientOcclusion) << "Unexpected ao value at y level " << y << " found";
			} else {
				// these should have ao
				// 4 vertices x = [1,2] z = [1,2] - all with ao of 2 (1 occlusion cell)
				EXPECT_NE(noAO, v.ambientOcclusion) << "Unexpected ao value at " << x << ":" << y << ":" << z;
			}
		} else {
			ADD_FAILURE() << "unexpected y coordinate " << int(y);
		}
	}
}

}
