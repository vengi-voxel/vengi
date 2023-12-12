/**
 * @file
 */

#include "ambient-occlusion.h"
#include "app/tests/AbstractTest.h"
#include "voxel/ChunkMesh.h"
#include "voxel/SurfaceExtractor.h"
#include "voxel/VoxelVertex.h"
#include <cstddef>

namespace voxel {

class AmbientOcclusion : public app::AbstractTest {};

TEST_F(AmbientOcclusion, testAOFaces) {
	core::SharedPtr<voxel::RawVolume> volume = ambientocclusion::create();
	voxel::ChunkMesh mesh;
	voxel::SurfaceExtractionContext ctx = voxel::buildCubicContext(volume.get(), volume->region(), mesh);
	voxel::extractSurface(ctx);
	const voxel::Mesh &opaqueMesh = mesh.mesh[0];
	const VertexArray &verts = opaqueMesh.getVertexVector();
	EXPECT_EQ((int)verts.size(), 110);
	int aofound[4] {0, 0, 0, 0};
	int voxelvertices = 0;
	for (const VoxelVertex &v : verts) {
		if (v.colorIndex != 1) {
			continue;
		}
		++voxelvertices;
		++aofound[v.ambientOcclusion];
	}
	EXPECT_EQ(voxelvertices, 6);
	EXPECT_EQ(aofound[0], 0); // full occlusion
	EXPECT_EQ(aofound[1], 0);
	// TODO: this looks wrong - it should be 4 and 2, not 2 and 4
	EXPECT_EQ(aofound[2], 2);
	EXPECT_EQ(aofound[3], 4); // no ao
}

} // namespace voxel
