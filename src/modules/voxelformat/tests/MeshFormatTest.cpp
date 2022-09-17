/**
 * @file
 */

#include "voxelformat/MeshFormat.h"
#include "app/tests/AbstractTest.h"

namespace voxelformat {

class MeshFormatTest : public app::AbstractTest {};

TEST_F(MeshFormatTest, testSubdivide) {
	MeshFormat::TriCollection tinyTris;
	Tri tri;
	tri.vertices[0] = glm::vec3(-8.77272797, -11.43335, -0.154544264);
	tri.vertices[1] = glm::vec3(-8.77272701, 11.1000004, -0.154543981);
	tri.vertices[2] = glm::vec3(8.77272701, 11.1000004, -0.154543981);
	MeshFormat::subdivideTri(tri, tinyTris);
	EXPECT_EQ(1024u, tinyTris.size());
}

TEST_F(MeshFormatTest, testCalculateAABB) {
	MeshFormat::TriCollection tris;
	Tri tri;

	{
		tri.vertices[0] = glm::vec3(0, 0, 0);
		tri.vertices[1] = glm::vec3(10, 0, 0);
		tri.vertices[2] = glm::vec3(10, 0, 10);
		tris.push_back(tri);
	}

	{
		tri.vertices[0] = glm::vec3(0, 0, 0);
		tri.vertices[1] = glm::vec3(-10, 0, 0);
		tri.vertices[2] = glm::vec3(-10, 0, -10);
		tris.push_back(tri);
	}

	glm::vec3 mins, maxs;
	ASSERT_TRUE(MeshFormat::calculateAABB(tris, mins, maxs));
	EXPECT_FLOAT_EQ(mins.x, -10.0f);
	EXPECT_FLOAT_EQ(mins.y, 0.0f);
	EXPECT_FLOAT_EQ(mins.z, -10.0f);
	EXPECT_FLOAT_EQ(maxs.x, 10.0f);
	EXPECT_FLOAT_EQ(maxs.y, 0.0f);
	EXPECT_FLOAT_EQ(maxs.z, 10.0f);
}

TEST_F(MeshFormatTest, testAreAllTrisAxisAligned) {
	MeshFormat::TriCollection tris;
	Tri tri;

	{
		tri.vertices[0] = glm::vec3(0, 0, 0);
		tri.vertices[1] = glm::vec3(10, 0, 0);
		tri.vertices[2] = glm::vec3(10, 0, 10);
		tris.push_back(tri);
	}

	{
		tri.vertices[0] = glm::vec3(0, 0, 0);
		tri.vertices[1] = glm::vec3(-10, 0, 0);
		tri.vertices[2] = glm::vec3(-10, 0, -10);
		tris.push_back(tri);
	}

	EXPECT_TRUE(MeshFormat::isAxisAligned(tris));

	{
		tri.vertices[0] = glm::vec3(0, 0, 0);
		tri.vertices[1] = glm::vec3(-10, 1, 0);
		tri.vertices[2] = glm::vec3(-10, 0, -10);
		tris.push_back(tri);
	}

	EXPECT_FALSE(MeshFormat::isAxisAligned(tris));
}

} // namespace voxelformat
