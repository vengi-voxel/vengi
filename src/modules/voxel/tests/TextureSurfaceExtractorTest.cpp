/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "image/Image.h"
#include "io/FileStream.h"
#include "io/MemoryReadStream.h"
#include "palette/Palette.h"
#include "voxel/ChunkMesh.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/SurfaceExtractor.h"

namespace voxel {

class TextureSurfaceExtractorTest : public app::AbstractTest {};

TEST_F(TextureSurfaceExtractorTest, testTextureExtraction) {
	voxel::Region region(0, 0, 0, 31, 31, 31);
	voxel::RawVolume volume(region);

	palette::Palette palette;
	palette.nippon();

	for (int z = 0; z < 10; ++z) {
		for (int y = 0; y < 10; ++y) {
			for (int x = 0; x < 10; ++x) {
				volume.setVoxel(x, y, z, voxel::createVoxel(palette, (x * y * z) % palette.size()));
			}
		}
	}

	voxel::ChunkMesh mesh;
	voxel::SurfaceExtractionContext ctx = voxel::createContext(voxel::SurfaceExtractionType::GreedyTexture, &volume,
															   region, palette, mesh, glm::ivec3(0));
	voxel::extractSurface(ctx);

	EXPECT_GT(ctx.textureWidth, 0);
	EXPECT_GT(ctx.textureHeight, 0);
	EXPECT_FALSE(ctx.textureData.empty());
	const voxel::Mesh &omesh = mesh.mesh[0];
	EXPECT_FALSE(omesh.isEmpty());

	const VertexArray &vertices = omesh.getVertexVector();
	const UVArray &uvs = omesh.getUVVector();

	EXPECT_EQ(vertices.size(), uvs.size());
	EXPECT_GT(vertices.size(), 8u);

	for (const auto &uv : uvs) {
		EXPECT_GE(uv.x, 0.0f);
		EXPECT_LE(uv.x, 1.0f);
		EXPECT_GE(uv.y, 0.0f);
		EXPECT_LE(uv.y, 1.0f);
	}

	image::Image image("texture_output.png", 4);
	io::MemoryReadStream memStream(ctx.textureData.data(), ctx.textureData.size());
	ASSERT_TRUE(image.loadRGBA(memStream, ctx.textureWidth, ctx.textureHeight));
}

TEST_F(TextureSurfaceExtractorTest, testWindingOrder) {
	// Create a single voxel to test winding order
	voxel::Region region(0, 0, 0, 1, 1, 1);
	voxel::RawVolume volume(region);

	palette::Palette palette;
	palette.nippon();

	volume.setVoxel(0, 0, 0, voxel::createVoxel(palette, 1));

	voxel::ChunkMesh mesh;
	voxel::SurfaceExtractionContext ctx = voxel::createContext(voxel::SurfaceExtractionType::GreedyTexture, &volume,
															   region, palette, mesh, glm::ivec3(0));
	voxel::extractSurface(ctx);

	const voxel::Mesh &omesh = mesh.mesh[0];
	EXPECT_FALSE(omesh.isEmpty());

	const VertexArray &vertices = omesh.getVertexVector();
	const IndexArray &indices = omesh.getIndexVector();
	const NormalArray &normals = omesh.getNormalVector();

	// A single voxel should have 6 faces, each with 4 vertices and 2 triangles (6 indices)
	EXPECT_EQ(vertices.size(), 24u); // 6 faces * 4 vertices
	EXPECT_EQ(indices.size(), 36u);  // 6 faces * 2 triangles * 3 indices
	EXPECT_EQ(normals.size(), 24u);  // 6 faces * 4 vertices

	// Check each triangle has CCW winding when viewed from outside
	for (size_t i = 0; i < indices.size(); i += 3) {
		const glm::vec3 &v0 = vertices[indices[i]].position;
		const glm::vec3 &v1 = vertices[indices[i + 1]].position;
		const glm::vec3 &v2 = vertices[indices[i + 2]].position;
		const glm::vec3 &normal = normals[indices[i]];

		// Calculate triangle normal using cross product
		glm::vec3 edge1 = v1 - v0;
		glm::vec3 edge2 = v2 - v0;
		glm::vec3 calculatedNormal = glm::normalize(glm::cross(edge1, edge2));

		// The calculated normal should point in the same direction as the face normal
		float dot = glm::dot(calculatedNormal, normal);
		EXPECT_GT(dot, 0.9f) << "Triangle " << (i / 3) << " has incorrect winding: "
							 << "v0=" << v0.x << "," << v0.y << "," << v0.z << " "
							 << "v1=" << v1.x << "," << v1.y << "," << v1.z << " "
							 << "v2=" << v2.x << "," << v2.y << "," << v2.z << " "
							 << "normal=" << normal.x << "," << normal.y << "," << normal.z << " "
							 << "calculated=" << calculatedNormal.x << "," << calculatedNormal.y << "," << calculatedNormal.z;
	}
}

} // namespace voxel
