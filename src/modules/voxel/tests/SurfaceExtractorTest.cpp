/**
 * @file
 */

#include "voxel/SurfaceExtractor.h"
#include "app/tests/AbstractTest.h"
#include "voxel/ChunkMesh.h"
#include "voxel/RawVolume.h"

namespace voxel {

class SurfaceExtractorTest : public app::AbstractTest {};

// https://github.com/vengi-voxel/vengi/issues/389
// 63 vertices mesh object. When you import this one into Blender, then when manually merged (Mesh > Merge > By Distance
// 0.0001m) will yield to 48 vertices. There are 15 pairs of overlapping vertices: index 52 and 56 are overlapping in
// the final .obj file. 49 & 48 also overlapping. 30 & 20. 55 & 51. 4 & 14. 13 & 2. 10 & 16. 11 & 8. 23 & 9. 39 & 37. 41
// & 25. 44 & 33. 36 & 34. 17 & 15. 47 & 46.
TEST_F(SurfaceExtractorTest, DISABLED_testMeshExtraction) {
	glm::ivec3 mins(0, 0, 0);
	glm::ivec3 maxs(143, 22, 134);
	voxel::Region region(mins, maxs);
	voxel::RawVolume v(region);
	v.setVoxel(96, 6, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(97, 6, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(98, 6, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(96, 7, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(97, 7, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(98, 7, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(96, 8, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(97, 8, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(98, 8, 62, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(96, 6, 63, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(97, 6, 63, voxel::createVoxel(voxel::VoxelType::Generic, 2));
	v.setVoxel(98, 6, 63, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(96, 7, 63, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(97, 7, 63, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(98, 7, 63, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(96, 8, 63, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(97, 8, 63, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(98, 8, 63, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(99, 5, 64, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(95, 6, 64, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(96, 6, 64, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(97, 6, 64, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(98, 6, 64, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(99, 6, 64, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(96, 7, 64, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(97, 7, 64, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(98, 7, 64, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(96, 8, 64, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(97, 8, 64, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(98, 8, 64, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(99, 5, 65, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(95, 6, 65, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(96, 6, 65, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(97, 6, 65, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(98, 6, 65, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(99, 6, 65, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(96, 7, 65, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(97, 7, 65, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(98, 7, 65, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(95, 5, 66, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(96, 5, 66, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(97, 5, 66, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(98, 5, 66, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(99, 5, 66, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(95, 6, 66, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(96, 6, 66, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(97, 6, 66, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(98, 6, 66, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(99, 6, 66, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(95, 5, 67, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(96, 5, 67, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(97, 5, 67, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(98, 5, 67, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(95, 6, 67, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(96, 6, 67, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(97, 6, 67, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(98, 6, 67, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(95, 5, 68, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(96, 5, 68, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(97, 5, 68, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(98, 5, 68, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(95, 6, 68, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(96, 6, 68, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(97, 6, 68, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(98, 6, 68, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(95, 5, 69, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(96, 5, 69, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(97, 5, 69, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(98, 5, 69, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(95, 6, 69, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(96, 6, 69, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(97, 6, 69, voxel::createVoxel(voxel::VoxelType::Generic, 47));
	v.setVoxel(98, 6, 69, voxel::createVoxel(voxel::VoxelType::Generic, 47));

	const bool mergeQuads = true;
	const bool reuseVertices = true;
	const bool ambientOcclusion = false;

	voxel::ChunkMesh mesh;

	SurfaceExtractionContext ctx =
		voxel::buildCubicContext(&v, region, mesh, glm::ivec3(0), mergeQuads, reuseVertices, ambientOcclusion);
	voxel::extractSurface(ctx);
	EXPECT_EQ(48, (int)mesh.mesh[0].getNoOfVertices());
}

TEST_F(SurfaceExtractorTest, testMeshExtractionIssue445) {
	glm::ivec3 mins(-1, -1, -1);
	glm::ivec3 maxs(1, -1, 1);
	voxel::Region region(mins, maxs);
	voxel::RawVolume v(region);
	for (int x = mins.x; x <= maxs.x; ++x) {
		for (int y = mins.y; y <= maxs.y; ++y) {
			for (int z = mins.z; z <= maxs.z; ++z) {
				v.setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
			}
		}
	}

	const bool mergeQuads = true;
	const bool reuseVertices = true;
	const bool ambientOcclusion = true;

	voxel::ChunkMesh mesh;

	region.shiftUpperCorner(1, 1, 1);
	SurfaceExtractionContext ctx =
		voxel::buildCubicContext(&v, region, mesh, glm::ivec3(0), mergeQuads, reuseVertices, ambientOcclusion);
	voxel::extractSurface(ctx);
	EXPECT_EQ(8, (int)mesh.mesh[0].getNoOfVertices());
}

} // namespace voxel
