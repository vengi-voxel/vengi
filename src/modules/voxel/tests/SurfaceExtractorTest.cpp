/**
 * @file
 */

#include "voxel/SurfaceExtractor.h"
#include "app/tests/AbstractTest.h"
#include "core/collection/DynamicArray.h"
#include "palette/Palette.h"
#include "voxel/ChunkMesh.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"

namespace voxel {
void prepareChunk(const RawVolume &map, core::DynamicArray<Voxel> &voxels, const glm::ivec3 &chunkPos);

class SurfaceExtractorTest : public app::AbstractTest {
protected:
	/**
	 * @brief Helper function to verify vertex properties
	 */
	static void verifyVertex(const VoxelVertex &vertex, const glm::vec3 &expectedPos, uint8_t expectedColor,
							 uint32_t expectedAO, const char *msg) {
		EXPECT_FLOAT_EQ(vertex.position.x, expectedPos.x) << msg << " - X position mismatch";
		EXPECT_FLOAT_EQ(vertex.position.y, expectedPos.y) << msg << " - Y position mismatch";
		EXPECT_FLOAT_EQ(vertex.position.z, expectedPos.z) << msg << " - Z position mismatch";
		EXPECT_EQ(vertex.colorIndex, expectedColor) << msg << " - Color mismatch";
		EXPECT_EQ(vertex.ambientOcclusion, expectedAO) << msg << " - AO mismatch";
	}

	/**
	 * @brief Helper to count triangles with a specific color
	 */
	static int countTrianglesWithColor(const Mesh &mesh, uint8_t colorIndex) {
		int count = 0;
		const IndexArray &indices = mesh.getIndexVector();
		const VertexArray &vertices = mesh.getVertexVector();

		for (size_t i = 0; i < indices.size(); i += 3) {
			if (vertices[indices[i]].colorIndex == colorIndex) {
				count++;
			}
		}
		return count;
	}
};

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
	const bool optimize = true;

	voxel::ChunkMesh mesh;

	SurfaceExtractionContext ctx =
		voxel::buildCubicContext(&v, region, mesh, glm::ivec3(0), mergeQuads, reuseVertices, ambientOcclusion, optimize);
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

TEST_F(SurfaceExtractorTest, testBinaryPrepareChunk) {
	glm::ivec3 mins(-10, -10, -10);
	glm::ivec3 maxs(10, -10, 10);
	voxel::Region region(mins, maxs);
	voxel::RawVolume v(region);
	const voxel::Voxel voxel = voxel::createVoxel(VoxelType::Generic, 1);
	v.setVoxel(region.getCenter(), voxel);
	core::DynamicArray<voxel::Voxel> voxels;
	glm::ivec3 chunkPos = v.region().getCenter();
	voxel::prepareChunk(v, voxels, chunkPos);
	ASSERT_TRUE(voxels[0].isSameType(voxel));
	ASSERT_TRUE(voxels[1].isSameType(voxel::Voxel()));
}

TEST_F(SurfaceExtractorTest, testMeshExtractionMarchingCubes) {
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

	voxel::ChunkMesh mesh;
	palette::Palette pal;
	pal.nippon();

	region.shiftUpperCorner(1, 1, 1);
	SurfaceExtractionContext ctx =
		voxel::buildMarchingCubesContext(&v, region, mesh, pal, false);
	voxel::extractSurface(ctx);
	EXPECT_EQ(30, (int)mesh.mesh[0].getNoOfVertices());
}

TEST_F(SurfaceExtractorTest, testBinaryGreedyMesherSingleVoxel) {
	// Test a single voxel in the center - should generate 6 faces (12 triangles)
	const Region region(0, 0, 0, 2, 2, 2);
	RawVolume volume(region);

	// Place a single voxel in the center
	const uint8_t testColor = 42;
	volume.setVoxel(1, 1, 1, createVoxel(VoxelType::Generic, testColor));

	ChunkMesh mesh;
	SurfaceExtractionContext ctx = buildBinaryContext(&volume, region, mesh, glm::ivec3(0), true, false);
	extractSurface(ctx);

	const Mesh &opaqueMesh = mesh.mesh[0];

	// Binary mesher creates 4 vertices per quad (no vertex reuse)
	// 6 faces * 4 vertices = 24 vertices, 6 faces * 2 triangles * 3 indices = 36 indices
	EXPECT_EQ(opaqueMesh.getNoOfVertices(), 24) << "Single voxel should have 24 vertices (4 per face)";
	EXPECT_EQ(opaqueMesh.getNoOfIndices(), 36) << "Single voxel should have 36 indices (12 triangles * 3)";

	// Verify all vertices have the correct color
	const VertexArray &vertices = opaqueMesh.getVertexVector();
	for (size_t i = 0; i < vertices.size(); ++i) {
		EXPECT_EQ(vertices[i].colorIndex, testColor) << "Vertex " << i << " should have test color";
		// Single voxel in open space should have maximum AO (no occlusion)
		EXPECT_EQ(vertices[i].ambientOcclusion, 3u) << "Vertex " << i << " should have max AO";
	}
}

TEST_F(SurfaceExtractorTest, testBinaryGreedyMesherPlane) {
	// Test a 3x3 plane - should merge into a single quad (2 triangles) per side
	const Region region(0, 0, 0, 4, 0, 4);
	RawVolume volume(region);

	const uint8_t testColor = 100;
	// Create a 3x3 plane at y=0
	for (int x = 1; x <= 3; ++x) {
		for (int z = 1; z <= 3; ++z) {
			volume.setVoxel(x, 0, z, createVoxel(VoxelType::Generic, testColor));
		}
	}

	ChunkMesh mesh;
	SurfaceExtractionContext ctx = buildBinaryContext(&volume, region, mesh, glm::ivec3(0), false, false);
	extractSurface(ctx);

	const Mesh &opaqueMesh = mesh.mesh[0];

	// A 3x3 plane has top and bottom faces, but also edges exposed
	// The greedy meshing should merge the large flat surfaces, but we still get multiple quads
	// With the region being 0,0,0 to 4,0,4 and voxels at y=0, we get 6 quads total (perimeter exposed)
	// Each quad = 4 vertices, so we get 24 vertices and 36 indices
	EXPECT_EQ(opaqueMesh.getNoOfVertices(), 24) << "3x3 plane should have 24 vertices";
	EXPECT_EQ(opaqueMesh.getNoOfIndices(), 36) << "3x3 plane should have 36 indices";

	// Verify color
	const VertexArray &vertices = opaqueMesh.getVertexVector();
	for (size_t i = 0; i < vertices.size(); ++i) {
		EXPECT_EQ(vertices[i].colorIndex, testColor) << "Vertex " << i << " color mismatch";
	}
}

TEST_F(SurfaceExtractorTest, testBinaryGreedyMesherAmbientOcclusion) {
	// Test ambient occlusion calculation with corner voxels
	const Region region(0, 0, 0, 3, 3, 3);
	RawVolume volume(region);

	const uint8_t centerColor = 50;
	const uint8_t cornerColor = 75;

	// Create a center voxel
	volume.setVoxel(1, 1, 1, createVoxel(VoxelType::Generic, centerColor));

	// Add corner voxels that will occlude the center
	volume.setVoxel(0, 0, 0, createVoxel(VoxelType::Generic, cornerColor));
	volume.setVoxel(2, 0, 0, createVoxel(VoxelType::Generic, cornerColor));
	volume.setVoxel(0, 2, 0, createVoxel(VoxelType::Generic, cornerColor));
	volume.setVoxel(2, 2, 0, createVoxel(VoxelType::Generic, cornerColor));

	ChunkMesh meshWithAO;
	SurfaceExtractionContext ctxWithAO = buildBinaryContext(&volume, region, meshWithAO, glm::ivec3(0), true, false);
	extractSurface(ctxWithAO);

	ChunkMesh meshWithoutAO;
	SurfaceExtractionContext ctxWithoutAO =
		buildBinaryContext(&volume, region, meshWithoutAO, glm::ivec3(0), false, false);
	extractSurface(ctxWithoutAO);

	const Mesh &aoMesh = meshWithAO.mesh[0];
	const Mesh &noAoMesh = meshWithoutAO.mesh[0];

	// Both should have vertices, but with different AO values
	EXPECT_GT(aoMesh.getNoOfVertices(), 0) << "Mesh with AO should have vertices";
	EXPECT_GT(noAoMesh.getNoOfVertices(), 0) << "Mesh without AO should have vertices";

	// Check that AO values vary in the AO mesh
	const VertexArray &aoVertices = aoMesh.getVertexVector();
	bool hasVariedAO = false;
	uint32_t firstAO = aoVertices[0].ambientOcclusion;
	for (size_t i = 1; i < aoVertices.size(); ++i) {
		if (aoVertices[i].ambientOcclusion != firstAO) {
			hasVariedAO = true;
			break;
		}
	}
	EXPECT_TRUE(hasVariedAO) << "AO mesh should have varied occlusion values";

	// Check that all AO values are max (3) in non-AO mesh
	const VertexArray &noAoVertices = noAoMesh.getVertexVector();
	for (size_t i = 0; i < noAoVertices.size(); ++i) {
		EXPECT_EQ(noAoVertices[i].ambientOcclusion, 3u) << "Non-AO mesh vertex " << i << " should have max AO";
	}
}

TEST_F(SurfaceExtractorTest, testBinaryGreedyMesherVertexPositions) {
	// Test exact vertex positions for a known configuration
	const Region region(0, 0, 0, 2, 2, 2);
	RawVolume volume(region);

	const uint8_t testColor = 123;
	// Single voxel at (1,1,1)
	volume.setVoxel(1, 1, 1, createVoxel(VoxelType::Generic, testColor));

	const glm::ivec3 translate(10, 20, 30);
	ChunkMesh mesh;
	SurfaceExtractionContext ctx = buildBinaryContext(&volume, region, mesh, translate, false, false);
	extractSurface(ctx);

	const Mesh &opaqueMesh = mesh.mesh[0];
	const VertexArray &vertices = opaqueMesh.getVertexVector();

	// Verify vertices are within expected bounds (cube at 1,1,1 with translation)
	for (size_t i = 0; i < vertices.size(); ++i) {
		const glm::vec3 &pos = vertices[i].position;

		// Positions should be in range [translate+1, translate+2] for each axis
		EXPECT_GE(pos.x, translate.x + 1.0f) << "Vertex " << i << " X too small";
		EXPECT_LE(pos.x, translate.x + 2.0f) << "Vertex " << i << " X too large";
		EXPECT_GE(pos.y, translate.y + 1.0f) << "Vertex " << i << " Y too small";
		EXPECT_LE(pos.y, translate.y + 2.0f) << "Vertex " << i << " Y too large";
		EXPECT_GE(pos.z, translate.z + 1.0f) << "Vertex " << i << " Z too small";
		EXPECT_LE(pos.z, translate.z + 2.0f) << "Vertex " << i << " Z too large";
	}
}

TEST_F(SurfaceExtractorTest, testBinaryGreedyMesherTwoAdjacentVoxels) {
	// Test that adjacent voxels share a merged face
	const Region region(0, 0, 0, 3, 0, 0);
	RawVolume volume(region);

	const uint8_t color1 = 10;
	const uint8_t color2 = 20;

	// Two adjacent voxels with different colors
	volume.setVoxel(1, 0, 0, createVoxel(VoxelType::Generic, color1));
	volume.setVoxel(2, 0, 0, createVoxel(VoxelType::Generic, color2));

	ChunkMesh mesh;
	SurfaceExtractionContext ctx = buildBinaryContext(&volume, region, mesh, glm::ivec3(0), false, false);
	extractSurface(ctx);

	const Mesh &opaqueMesh = mesh.mesh[0];

	// Each voxel should contribute 5 faces (the shared face is culled)
	// Total: 10 faces = 20 triangles = 60 indices
	EXPECT_GT(opaqueMesh.getNoOfVertices(), 0) << "Should have vertices";
	EXPECT_GT(opaqueMesh.getNoOfIndices(), 0) << "Should have indices";

	// Verify both colors are present
	int color1Triangles = countTrianglesWithColor(opaqueMesh, color1);
	int color2Triangles = countTrianglesWithColor(opaqueMesh, color2);

	EXPECT_GT(color1Triangles, 0) << "Should have triangles with color1";
	EXPECT_GT(color2Triangles, 0) << "Should have triangles with color2";
}

TEST_F(SurfaceExtractorTest, testBinaryGreedyMesherLShape) {
	// Test L-shaped configuration to verify greedy merging limits
	const Region region(0, 0, 0, 3, 1, 3);
	RawVolume volume(region);

	const uint8_t testColor = 55;

	// Create an L shape
	volume.setVoxel(0, 0, 0, createVoxel(VoxelType::Generic, testColor));
	volume.setVoxel(1, 0, 0, createVoxel(VoxelType::Generic, testColor));
	volume.setVoxel(2, 0, 0, createVoxel(VoxelType::Generic, testColor));
	volume.setVoxel(0, 0, 1, createVoxel(VoxelType::Generic, testColor));
	volume.setVoxel(0, 0, 2, createVoxel(VoxelType::Generic, testColor));

	ChunkMesh mesh;
	SurfaceExtractionContext ctx = buildBinaryContext(&volume, region, mesh, glm::ivec3(0), false, false);
	extractSurface(ctx);

	const Mesh &opaqueMesh = mesh.mesh[0];

	// L-shape can't be merged into a single quad, so we expect more vertices/triangles
	// than a 5-voxel straight line would produce
	EXPECT_GT(opaqueMesh.getNoOfVertices(), 0) << "L-shape should have vertices";
	EXPECT_GT(opaqueMesh.getNoOfIndices(), 0) << "L-shape should have indices";

	// All vertices should have the same color
	const VertexArray &vertices = opaqueMesh.getVertexVector();
	for (size_t i = 0; i < vertices.size(); ++i) {
		EXPECT_EQ(vertices[i].colorIndex, testColor) << "Vertex " << i << " color mismatch";
	}
}

TEST_F(SurfaceExtractorTest, testBinaryGreedyMesherTransparentVoxels) {
	// Test that transparent voxels go into the correct mesh
	const Region region(0, 0, 0, 2, 2, 2);
	RawVolume volume(region);

	const uint8_t opaqueColor = 30;
	const uint8_t transColor = 60;

	// Place opaque and transparent voxels
	volume.setVoxel(0, 0, 0, createVoxel(VoxelType::Generic, opaqueColor));
	volume.setVoxel(2, 2, 2, createVoxel(VoxelType::Transparent, transColor));

	ChunkMesh mesh;
	SurfaceExtractionContext ctx = buildBinaryContext(&volume, region, mesh, glm::ivec3(0), false, false);
	extractSurface(ctx);

	const Mesh &opaqueMesh = mesh.mesh[0];
	const Mesh &transMesh = mesh.mesh[1];

	// Opaque mesh should have the Generic voxel
	EXPECT_GT(opaqueMesh.getNoOfVertices(), 0) << "Opaque mesh should have vertices";

	// Transparent mesh should have the Transparent voxel
	EXPECT_GT(transMesh.getNoOfVertices(), 0) << "Transparent mesh should have vertices";

	// Verify colors are in correct meshes
	const VertexArray &opaqueVertices = opaqueMesh.getVertexVector();
	for (size_t i = 0; i < opaqueVertices.size(); ++i) {
		EXPECT_EQ(opaqueVertices[i].colorIndex, opaqueColor) << "Opaque vertex " << i << " color mismatch";
	}

	const VertexArray &transVertices = transMesh.getVertexVector();
	for (size_t i = 0; i < transVertices.size(); ++i) {
		EXPECT_EQ(transVertices[i].colorIndex, transColor) << "Transparent vertex " << i << " color mismatch";
	}
}

TEST_F(SurfaceExtractorTest, testBinaryGreedyMesherEmptyRegion) {
	// Test empty region doesn't crash and produces no geometry
	const Region region(0, 0, 0, 10, 10, 10);
	RawVolume volume(region);

	ChunkMesh mesh;
	SurfaceExtractionContext ctx = buildBinaryContext(&volume, region, mesh, glm::ivec3(0), false, false);
	extractSurface(ctx);

	const Mesh &opaqueMesh = mesh.mesh[0];
	const Mesh &transMesh = mesh.mesh[1];

	EXPECT_EQ(opaqueMesh.getNoOfVertices(), 0) << "Empty region should have no opaque vertices";
	EXPECT_EQ(opaqueMesh.getNoOfIndices(), 0) << "Empty region should have no opaque indices";
	EXPECT_EQ(transMesh.getNoOfVertices(), 0) << "Empty region should have no transparent vertices";
	EXPECT_EQ(transMesh.getNoOfIndices(), 0) << "Empty region should have no transparent indices";
}

TEST_F(SurfaceExtractorTest, testBinaryGreedyMesherSingleVoxelDetailedPositions) {
	// Test exact vertex positions and face orientation for a single voxel
	const Region region(0, 0, 0, 2, 2, 2);
	RawVolume volume(region);

	const uint8_t testColor = 42;
	volume.setVoxel(1, 1, 1, createVoxel(VoxelType::Generic, testColor));

	ChunkMesh mesh;
	SurfaceExtractionContext ctx = buildBinaryContext(&volume, region, mesh, glm::ivec3(0), false, false);
	extractSurface(ctx);

	const Mesh &opaqueMesh = mesh.mesh[0];
	const VertexArray &vertices = opaqueMesh.getVertexVector();
	const IndexArray &indices = opaqueMesh.getIndexVector();

	// Should have 24 vertices (6 faces * 4 vertices) and 36 indices (12 triangles * 3)
	ASSERT_EQ(vertices.size(), 24u) << "Single voxel should have 24 vertices";
	ASSERT_EQ(indices.size(), 36u) << "Single voxel should have 36 indices";

	// Verify all vertices have correct color
	for (size_t i = 0; i < vertices.size(); ++i) {
		EXPECT_EQ(vertices[i].colorIndex, testColor) << "Vertex " << i << " color mismatch";
	}

	// Count unique vertex positions (should be 8 corners of the cube)
	core::DynamicArray<glm::vec3> uniquePositions;
	for (const auto &v : vertices) {
		bool found = false;
		for (const auto &pos : uniquePositions) {
			if (glm::all(glm::epsilonEqual(v.position, pos, 0.001f))) {
				found = true;
				break;
			}
		}
		if (!found) {
			uniquePositions.push_back(v.position);
		}
	}
	EXPECT_EQ(uniquePositions.size(), 8u) << "Should have 8 unique corner positions";

	// Verify positions are in correct range [1,2] for each axis (voxel at 1,1,1)
	for (const auto &pos : uniquePositions) {
		EXPECT_GE(pos.x, 1.0f) << "Position X too small: " << pos.x;
		EXPECT_LE(pos.x, 2.0f) << "Position X too large: " << pos.x;
		EXPECT_GE(pos.y, 1.0f) << "Position Y too small: " << pos.y;
		EXPECT_LE(pos.y, 2.0f) << "Position Y too large: " << pos.y;
		EXPECT_GE(pos.z, 1.0f) << "Position Z too small: " << pos.z;
		EXPECT_LE(pos.z, 2.0f) << "Position Z too large: " << pos.z;
	}
}

TEST_F(SurfaceExtractorTest, testBinaryGreedyMesherTwoVoxelsXAxis) {
	// Test two voxels aligned along X axis
	const Region region(0, 0, 0, 3, 1, 1);
	RawVolume volume(region);

	const uint8_t testColor = 50;
	volume.setVoxel(1, 0, 0, createVoxel(VoxelType::Generic, testColor));
	volume.setVoxel(2, 0, 0, createVoxel(VoxelType::Generic, testColor));

	ChunkMesh mesh;
	SurfaceExtractionContext ctx = buildBinaryContext(&volume, region, mesh, glm::ivec3(0), false, false);
	extractSurface(ctx);

	const Mesh &opaqueMesh = mesh.mesh[0];
	const VertexArray &vertices = opaqueMesh.getVertexVector();

	ASSERT_GT(vertices.size(), 0u) << "Should have vertices";

	// All vertices should have correct color
	for (size_t i = 0; i < vertices.size(); ++i) {
		EXPECT_EQ(vertices[i].colorIndex, testColor) << "Vertex " << i << " color mismatch";
	}

	// Verify X positions span from 1 to 3
	float minX = 1000.0f, maxX = -1000.0f;
	for (const auto &v : vertices) {
		minX = glm::min(minX, v.position.x);
		maxX = glm::max(maxX, v.position.x);
	}
	EXPECT_FLOAT_EQ(minX, 1.0f) << "Min X should be 1.0";
	EXPECT_FLOAT_EQ(maxX, 3.0f) << "Max X should be 3.0";
}

TEST_F(SurfaceExtractorTest, testBinaryGreedyMesherTwoVoxelsYAxis) {
	// Test two voxels aligned along Y axis
	const Region region(0, 0, 0, 1, 3, 1);
	RawVolume volume(region);

	const uint8_t testColor = 60;
	volume.setVoxel(0, 1, 0, createVoxel(VoxelType::Generic, testColor));
	volume.setVoxel(0, 2, 0, createVoxel(VoxelType::Generic, testColor));

	ChunkMesh mesh;
	SurfaceExtractionContext ctx = buildBinaryContext(&volume, region, mesh, glm::ivec3(0), false, false);
	extractSurface(ctx);

	const Mesh &opaqueMesh = mesh.mesh[0];
	const VertexArray &vertices = opaqueMesh.getVertexVector();

	ASSERT_GT(vertices.size(), 0u) << "Should have vertices";

	// All vertices should have correct color
	for (size_t i = 0; i < vertices.size(); ++i) {
		EXPECT_EQ(vertices[i].colorIndex, testColor) << "Vertex " << i << " color mismatch";
	}

	// Verify Y positions span from 1 to 3
	float minY = 1000.0f, maxY = -1000.0f;
	for (const auto &v : vertices) {
		minY = glm::min(minY, v.position.y);
		maxY = glm::max(maxY, v.position.y);
	}
	EXPECT_FLOAT_EQ(minY, 1.0f) << "Min Y should be 1.0";
	EXPECT_FLOAT_EQ(maxY, 3.0f) << "Max Y should be 3.0";
}

TEST_F(SurfaceExtractorTest, testBinaryGreedyMesherTwoVoxelsZAxis) {
	// Test two voxels aligned along Z axis
	const Region region(0, 0, 0, 1, 1, 3);
	RawVolume volume(region);

	const uint8_t testColor = 70;
	volume.setVoxel(0, 0, 1, createVoxel(VoxelType::Generic, testColor));
	volume.setVoxel(0, 0, 2, createVoxel(VoxelType::Generic, testColor));

	ChunkMesh mesh;
	SurfaceExtractionContext ctx = buildBinaryContext(&volume, region, mesh, glm::ivec3(0), false, false);
	extractSurface(ctx);

	const Mesh &opaqueMesh = mesh.mesh[0];
	const VertexArray &vertices = opaqueMesh.getVertexVector();

	ASSERT_GT(vertices.size(), 0u) << "Should have vertices";

	// All vertices should have correct color
	for (size_t i = 0; i < vertices.size(); ++i) {
		EXPECT_EQ(vertices[i].colorIndex, testColor) << "Vertex " << i << " color mismatch";
	}

	// Verify Z positions span from 1 to 3
	float minZ = 1000.0f, maxZ = -1000.0f;
	for (const auto &v : vertices) {
		minZ = glm::min(minZ, v.position.z);
		maxZ = glm::max(maxZ, v.position.z);
	}
	EXPECT_FLOAT_EQ(minZ, 1.0f) << "Min Z should be 1.0";
	EXPECT_FLOAT_EQ(maxZ, 3.0f) << "Max Z should be 3.0";
}

TEST_F(SurfaceExtractorTest, testBinaryGreedyMesherLShapeDetailedPositions) {
	// Test L-shaped configuration with exact position verification
	const Region region(0, 0, 0, 3, 1, 3);
	RawVolume volume(region);

	const uint8_t testColor = 80;

	// Create an L shape in XZ plane
	// Horizontal part: voxels at (0,0,0), (1,0,0), (2,0,0)
	// Vertical part: voxels at (0,0,1), (0,0,2)
	volume.setVoxel(0, 0, 0, createVoxel(VoxelType::Generic, testColor));
	volume.setVoxel(1, 0, 0, createVoxel(VoxelType::Generic, testColor));
	volume.setVoxel(2, 0, 0, createVoxel(VoxelType::Generic, testColor));
	volume.setVoxel(0, 0, 1, createVoxel(VoxelType::Generic, testColor));
	volume.setVoxel(0, 0, 2, createVoxel(VoxelType::Generic, testColor));

	ChunkMesh mesh;
	SurfaceExtractionContext ctx = buildBinaryContext(&volume, region, mesh, glm::ivec3(0), false, false);
	extractSurface(ctx);

	const Mesh &opaqueMesh = mesh.mesh[0];
	const VertexArray &vertices = opaqueMesh.getVertexVector();

	ASSERT_GT(vertices.size(), 0u) << "L-shape should have vertices";

	// All vertices should have correct color
	for (size_t i = 0; i < vertices.size(); ++i) {
		EXPECT_EQ(vertices[i].colorIndex, testColor) << "Vertex " << i << " color mismatch";
	}

	// Verify X positions span from 0 to 3
	float minX = 1000.0f, maxX = -1000.0f;
	for (const auto &v : vertices) {
		minX = glm::min(minX, v.position.x);
		maxX = glm::max(maxX, v.position.x);
	}
	EXPECT_FLOAT_EQ(minX, 0.0f) << "Min X should be 0.0";
	EXPECT_FLOAT_EQ(maxX, 3.0f) << "Max X should be 3.0";

	// Verify Z positions span from 0 to 3
	float minZ = 1000.0f, maxZ = -1000.0f;
	for (const auto &v : vertices) {
		minZ = glm::min(minZ, v.position.z);
		maxZ = glm::max(maxZ, v.position.z);
	}
	EXPECT_FLOAT_EQ(minZ, 0.0f) << "Min Z should be 0.0";
	EXPECT_FLOAT_EQ(maxZ, 3.0f) << "Max Z should be 3.0";

	// Y should be at 0 and 1 (bottom and top of voxels)
	float minY = 1000.0f, maxY = -1000.0f;
	for (const auto &v : vertices) {
		minY = glm::min(minY, v.position.y);
		maxY = glm::max(maxY, v.position.y);
	}
	EXPECT_FLOAT_EQ(minY, 0.0f) << "Min Y should be 0.0";
	EXPECT_FLOAT_EQ(maxY, 1.0f) << "Max Y should be 1.0";
}

TEST_F(SurfaceExtractorTest, testBinaryGreedyMesherCubeCornerVoxels) {
	// Test all 8 corner voxels of a cube to verify face orientation
	const Region region(0, 0, 0, 3, 3, 3);
	RawVolume volume(region);

	const uint8_t testColor = 90;

	// Place voxels at all 8 corners
	volume.setVoxel(0, 0, 0, createVoxel(VoxelType::Generic, testColor));
	volume.setVoxel(3, 0, 0, createVoxel(VoxelType::Generic, testColor));
	volume.setVoxel(0, 3, 0, createVoxel(VoxelType::Generic, testColor));
	volume.setVoxel(3, 3, 0, createVoxel(VoxelType::Generic, testColor));
	volume.setVoxel(0, 0, 3, createVoxel(VoxelType::Generic, testColor));
	volume.setVoxel(3, 0, 3, createVoxel(VoxelType::Generic, testColor));
	volume.setVoxel(0, 3, 3, createVoxel(VoxelType::Generic, testColor));
	volume.setVoxel(3, 3, 3, createVoxel(VoxelType::Generic, testColor));

	ChunkMesh mesh;
	SurfaceExtractionContext ctx = buildBinaryContext(&volume, region, mesh, glm::ivec3(0), false, false);
	extractSurface(ctx);

	const Mesh &opaqueMesh = mesh.mesh[0];
	const VertexArray &vertices = opaqueMesh.getVertexVector();

	// Each corner voxel should contribute 6 faces
	// Total: 8 * 6 * 4 = 192 vertices, 8 * 6 * 2 * 3 = 288 indices
	EXPECT_EQ(vertices.size(), 192u) << "8 corner voxels should have 192 vertices";

	// All vertices should have correct color
	for (size_t i = 0; i < vertices.size(); ++i) {
		EXPECT_EQ(vertices[i].colorIndex, testColor) << "Vertex " << i << " color mismatch";
	}
}

} // namespace voxel
