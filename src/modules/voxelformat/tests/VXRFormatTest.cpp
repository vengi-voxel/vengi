/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/VXMFormat.h"
#include "voxelformat/VXRFormat.h"

#include <glm/gtc/type_ptr.hpp>

namespace voxelformat {

class VXRFormatTest: public AbstractVoxFormatTest {
};

TEST_F(VXRFormatTest, testSaveSmallVoxel) {
	VXMFormat vxm;
	testSave("sandbox-smallvolumesavetest0.vxm", &vxm);
	VXRFormat f;
	testSaveLoadVoxel("sandbox-smallvolumesavetest.vxr", &f);
}

TEST_F(VXRFormatTest, testGiantDinosaur) {
	VXRFormat f;
	SceneGraph sceneGraph;
	ASSERT_TRUE(loadGroups("giant_dinosaur/Reptiles_Biped_Giant_Dinossaur_V2.vxr", f, sceneGraph));
	{
		SceneGraphNode* node = sceneGraph.findNodeByName("Hip");
		ASSERT_NE(nullptr, node);
		const SceneGraphTransform &transform = node->transform(0);

		const float expected[]{
			1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 64.5, 0.0, 1.0};
		const float *actual = glm::value_ptr(transform.worldMatrix());
		for (int i = 0; i < 16; ++i) {
			ASSERT_FLOAT_EQ(expected[i], actual[i]) << i << ": " << glm::to_string(transform.worldMatrix());
		}
	}
	{
		SceneGraphNode* node = sceneGraph.findNodeByName("Tail4");
		ASSERT_NE(nullptr, node);
		const SceneGraphTransform &transform = node->transform(0);

		const float expected[]{
			0.941261, 0.11818516, -0.31632274, 0.0, -0.084998831, 0.989514, 0.1167788, 0.0, 0.32680732, -0.083032265, 0.94143647, 0.0, -18.847145, 51.539429, -107.957901, 1.0};
		const float *actual = glm::value_ptr(transform.worldMatrix());
		for (int i = 0; i < 16; ++i) {
			ASSERT_FLOAT_EQ(expected[i], actual[i]) << i << ": " << glm::to_string(transform.worldMatrix());
		}
	}
	{
		SceneGraphNode* node = sceneGraph.findNodeByName("L_Arm");
		ASSERT_NE(nullptr, node);
		const SceneGraphTransform &transform = node->transform(0);

		const float expected[]{
			1.0, 0.0, 0.0, 0.0, 0.0, 0.99974263, 0.022687117, 0.0, 0.0, -0.022687117, 0.99974263, 0.0, -19.000000, 52.389652, 27.726467, 1.0};
		const float *actual = glm::value_ptr(transform.worldMatrix());
		for (int i = 0; i < 16; ++i) {
			ASSERT_FLOAT_EQ(expected[i], actual[i]) << i << ": " << glm::to_string(transform.worldMatrix());
		}
	}
}

}
