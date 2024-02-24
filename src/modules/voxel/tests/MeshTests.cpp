/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "voxel/Mesh.h"
#include "voxel/VoxelVertex.h"

namespace voxel {

class MeshTest : public app::AbstractTest {};

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

} // namespace voxel
