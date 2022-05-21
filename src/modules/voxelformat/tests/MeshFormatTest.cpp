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
	EXPECT_EQ(4u, tinyTris.size());
}

} // namespace voxelformat
