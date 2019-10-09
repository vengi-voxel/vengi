/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "mesh/Mesh.h"

namespace mesh {

class MeshTest : public core::AbstractTest {
};

TEST_F(MeshTest, testLoadKnight) {
	Mesh mesh;
	ASSERT_TRUE(mesh.loadMesh("mesh/chr_knight.fbx")) << "Failed to load mesh";
}

}
