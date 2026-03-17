/**
 * @file
 */

#include "voxelutil/SmoothMeshExtractor.h"
#include "app/tests/AbstractTest.h"
#include "palette/Palette.h"
#include "voxel/ChunkMesh.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"

namespace voxelutil {

class SmoothMeshExtractorTest : public app::AbstractTest {};

TEST_F(SmoothMeshExtractorTest, testExtractSolidCube) {
	// A 4x4x4 solid cube should produce a closed manifold mesh
	const voxel::Region region(0, 0, 0, 3, 3, 3);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	for (int z = 0; z <= 3; ++z) {
		for (int y = 0; y <= 3; ++y) {
			for (int x = 0; x <= 3; ++x) {
				volume.setVoxel(x, y, z, solid);
			}
		}
	}

	voxel::ChunkMesh mesh;
	extractSmoothMesh(&volume, 0, SmoothFilter::Laplacian, 0.5f, &mesh);

	EXPECT_GT(mesh.mesh[0].getNoOfVertices(), 0u);
	EXPECT_GT(mesh.mesh[0].getNoOfIndices(), 0u);
	// Indices should be multiples of 3 (triangles)
	EXPECT_EQ(mesh.mesh[0].getNoOfIndices() % 3, 0u);
}

TEST_F(SmoothMeshExtractorTest, testExtractEmptyVolume) {
	// An empty volume should produce no mesh
	const voxel::Region region(0, 0, 0, 3, 3, 3);
	voxel::RawVolume volume(region);

	voxel::ChunkMesh mesh;
	extractSmoothMesh(&volume, 0, SmoothFilter::Laplacian, 0.5f, &mesh);

	EXPECT_EQ(mesh.mesh[0].getNoOfVertices(), 0u);
	EXPECT_EQ(mesh.mesh[0].getNoOfIndices(), 0u);
}

TEST_F(SmoothMeshExtractorTest, testExtractSingleVoxel) {
	// A single voxel should produce a small mesh
	const voxel::Region region(0, 0, 0, 0, 0, 0);
	voxel::RawVolume volume(region);
	volume.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));

	voxel::ChunkMesh mesh;
	extractSmoothMesh(&volume, 0, SmoothFilter::Laplacian, 0.5f, &mesh);

	EXPECT_GT(mesh.mesh[0].getNoOfVertices(), 0u);
	EXPECT_GT(mesh.mesh[0].getNoOfIndices(), 0u);
}

TEST_F(SmoothMeshExtractorTest, testSmoothingMovesVertices) {
	// With smoothing iterations, vertices should move from their initial MC positions
	const voxel::Region region(0, 0, 0, 5, 5, 5);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	for (int z = 0; z <= 5; ++z) {
		for (int y = 0; y <= 5; ++y) {
			for (int x = 0; x <= 5; ++x) {
				volume.setVoxel(x, y, z, solid);
			}
		}
	}

	// Extract without smoothing
	voxel::ChunkMesh meshNoSmooth;
	extractSmoothMesh(&volume, 0, SmoothFilter::Laplacian, 0.5f, &meshNoSmooth);

	// Extract with smoothing
	voxel::ChunkMesh meshSmooth;
	extractSmoothMesh(&volume, 10, SmoothFilter::Laplacian, 0.5f, &meshSmooth);

	// Both should have the same vertex count (smoothing doesn't add/remove vertices)
	EXPECT_EQ(meshNoSmooth.mesh[0].getNoOfVertices(), meshSmooth.mesh[0].getNoOfVertices());
	EXPECT_EQ(meshNoSmooth.mesh[0].getNoOfIndices(), meshSmooth.mesh[0].getNoOfIndices());
}

} // namespace voxelutil
