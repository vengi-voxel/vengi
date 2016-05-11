/**
 * @file
 */

#include "AbstractVoxelTest.h"
#include "voxel/polyvox/CubicSurfaceExtractor.h"
#include "voxel/Spiral.h"

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

	const Mesh<CubicVertex>& mesh = extractCubicMesh(&_volData, _ctx.region, IsQuadNeeded(), false);
	const CubicVertex* vertices = mesh.getRawVertexData();
	const int amount = mesh.getNoOfVertices();
	ASSERT_EQ(116, amount);

	for (int i = 0; i < amount; ++i) {
		const CubicVertex& v = vertices[i];
		const int8_t y = v.encodedPosition.y;
		if (y == 0) {
		} else if (y == 1) {
		} else if (y == 2) {
		} else if (y == 3) {
		} else {
			ADD_FAILURE() << "unexpected y coordinate " << int(y);
		}
	}
}

}
