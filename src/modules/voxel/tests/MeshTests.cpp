/**
 * @file
 */

#include "core/ConfigVar.h"
#include "voxel/tests/AbstractVoxelTest.h"
#include "voxel/ChunkMesh.h"
#include "voxel/Mesh.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/SurfaceExtractor.h"
#include "voxel/Voxel.h"
#include "voxel/VoxelVertex.h"

namespace voxel {

class MeshTest : public AbstractVoxelTest {
protected:
	static void addColoredQuad(Mesh &mesh, const glm::vec3 &p0, const glm::vec3 &p1, const glm::vec3 &p2,
							   const glm::vec3 &p3, uint8_t colorIndex) {
		auto addVertex = [&](const glm::vec3 &pos) {
			VoxelVertex vertex;
			vertex.position = pos;
			vertex.colorIndex = colorIndex;
			vertex.ambientOcclusion = 3;
			vertex.normalIndex = 0;
			return mesh.addVertex(vertex);
		};
		const IndexType i0 = addVertex(p0);
		const IndexType i1 = addVertex(p1);
		const IndexType i2 = addVertex(p2);
		const IndexType i3 = addVertex(p3);
		mesh.addTriangle(i0, i1, i2);
		mesh.addTriangle(i0, i2, i3);
	}

	static bool triangleUsesColor(const Mesh &mesh, uint8_t colorIndex) {
		const IndexArray &indices = mesh.getIndexVector();
		const VertexArray &vertices = mesh.getVertexVector();
		for (size_t i = 0; i < indices.size(); i += 3) {
			if (vertices[indices[i]].colorIndex == colorIndex || vertices[indices[i + 1]].colorIndex == colorIndex ||
				vertices[indices[i + 2]].colorIndex == colorIndex) {
				return true;
			}
		}
		return false;
	}
};

TEST_F(MeshTest, DISABLED_testSort) {
	Mesh mesh;
	voxel::VoxelVertex v;
	v.info = 3;
	v.colorIndex = 0;
	v.position = {31.000000, 0.000000, 0.000000};
	mesh.addVertex(v);
	v.position = {31.000000, 0.000000, 1.000000};
	mesh.addVertex(v);
	v.position = {31.000000, 1.000000, 1.000000};
	mesh.addVertex(v);
	v.position = {31.000000, 1.000000, 0.000000};
	mesh.addVertex(v);
	v.position = {32.000000, 0.000000, 0.000000};
	mesh.addVertex(v);
	v.position = {32.000000, 0.000000, 1.000000};
	mesh.addVertex(v);
	v.position = {32.000000, 1.000000, 0.000000};
	mesh.addVertex(v);
	v.position = {32.000000, 1.000000, 1.000000};
	mesh.addVertex(v);
	mesh.addTriangle(4, 6, 7);
	mesh.addTriangle(4, 7, 5);
	mesh.addTriangle(3, 2, 7);
	mesh.addTriangle(3, 7, 6);
	mesh.addTriangle(1, 5, 7);
	mesh.addTriangle(1, 7, 2);
	mesh.addTriangle(0, 1, 2);
	mesh.addTriangle(0, 2, 3);
	mesh.addTriangle(0, 4, 5);
	mesh.addTriangle(0, 5, 1);
	mesh.addTriangle(0, 3, 6);
	mesh.addTriangle(0, 6, 4);

	EXPECT_TRUE(mesh.sort(glm::vec3(100.0f, 100.0f, 100.0f)));
}

// https://github.com/vengi-voxel/vengi/issues/833
TEST_F(MeshTest, testOptimizePreservesColors) {
	core::VarPtr simplifyRatio = core::getVar(cfg::VoxformatMeshSimplifyRatio);
	const float previousRatio = simplifyRatio->floatVal();
	simplifyRatio->setVal(0.8f);

	Mesh mesh;
	addColoredQuad(mesh, {0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}, 10);
	addColoredQuad(mesh, {1, 0, 0}, {2, 0, 0}, {2, 0, 1}, {1, 0, 1}, 20);

	const size_t indexCountBefore = mesh.getNoOfIndices();
	ASSERT_GE(indexCountBefore, 9u);

	mesh.optimize();

	EXPECT_GT(mesh.getNoOfIndices(), 0u);
	EXPECT_TRUE(triangleUsesColor(mesh, 10)) << "Color 10 should remain after optimization";
	EXPECT_TRUE(triangleUsesColor(mesh, 20)) << "Color 20 should remain after optimization";
	simplifyRatio->setVal(previousRatio);
}

TEST_F(MeshTest, testOptimizeReducesIndices) {
	core::VarPtr simplifyRatio = core::getVar(cfg::VoxformatMeshSimplifyRatio);
	const float previousRatio = simplifyRatio->floatVal();
	simplifyRatio->setVal(0.5f);

	// Build a grid with shared vertices so the simplifier can identify interior edges
	constexpr int gridSize = 8;
	Mesh mesh;
	const uint8_t color = 42;
	// Create shared vertex grid
	for (int z = 0; z <= gridSize; ++z) {
		for (int x = 0; x <= gridSize; ++x) {
			VoxelVertex vertex;
			vertex.position = glm::vec3((float)x, 0.0f, (float)z);
			vertex.colorIndex = color;
			vertex.ambientOcclusion = 3;
			vertex.normalIndex = 0;
			mesh.addVertex(vertex);
		}
	}
	// Create triangles referencing shared vertices
	for (int z = 0; z < gridSize; ++z) {
		for (int x = 0; x < gridSize; ++x) {
			const IndexType i0 = (IndexType)(z * (gridSize + 1) + x);
			const IndexType i1 = i0 + 1;
			const IndexType i2 = (IndexType)((z + 1) * (gridSize + 1) + x + 1);
			const IndexType i3 = (IndexType)((z + 1) * (gridSize + 1) + x);
			mesh.addTriangle(i0, i1, i2);
			mesh.addTriangle(i0, i2, i3);
		}
	}

	const size_t indexCountBefore = mesh.getNoOfIndices();
	ASSERT_GE(indexCountBefore, 9u);
	mesh.optimize();
	EXPECT_LT(mesh.getNoOfIndices(), indexCountBefore);
	simplifyRatio->setVal(previousRatio);
}

TEST_F(MeshTest, testOptimizeWithoutSimplificationKeepsIndexCount) {
	core::VarPtr simplifyRatio = core::getVar(cfg::VoxformatMeshSimplifyRatio);
	const float previousRatio = simplifyRatio->floatVal();
	simplifyRatio->setVal(0.0f);

	const Region region(0, 0, 0, 2, 2, 2);
	RawVolume volume(region);
	volume.setVoxel(1, 1, 1, createVoxel(VoxelType::Generic, 1));

	ChunkMesh chunkMesh;
	SurfaceExtractionContext ctx =
		buildCubicContext(&volume, region, chunkMesh, glm::ivec3(0), true, true, false, true);
	extractSurface(ctx);

	const Mesh &mesh = chunkMesh.mesh[0];
	EXPECT_EQ(mesh.getNoOfIndices(), 36u);
	EXPECT_EQ(mesh.getNoOfVertices(), 8u);
	simplifyRatio->setVal(previousRatio);
}

} // namespace voxel
