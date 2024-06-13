/**
 * @file
 */

#include "voxedit-util/modifier/brush/PlaneBrush.h"
#include "app/tests/AbstractTest.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/ISceneRenderer.h"
#include "voxedit-util/modifier/IModifierRenderer.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/Brush.h"
#include "voxel/Face.h"
#include "voxel/Voxel.h"

namespace voxedit {

class PlaneBrushTest : public app::AbstractTest {
protected:
	PlaneBrush _brush;
	BrushContext _brushContext;
	voxel::RawVolume _volume{voxel::Region(0, 3)};
	scenegraph::SceneGraphNode _node{scenegraph::SceneGraphNodeType::Model};
	scenegraph::SceneGraph _sceneGraph;
	voxel::Voxel _voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	void prepare(PlaneBrush &brush, const voxel::Voxel &voxel, BrushContext &brushContext, voxel::FaceNames face,
				 const glm::ivec3 &mins, const glm::ivec3 &maxs) {
		brushContext.cursorVoxel = voxel;
		brushContext.hitCursorVoxel = brushContext.cursorVoxel;
		brushContext.cursorPosition = mins;
		brushContext.cursorFace = face;
		EXPECT_TRUE(brush.start(brushContext));
		ASSERT_FALSE(brush.singleMode());
		EXPECT_TRUE(brush.active());
		brushContext.cursorPosition = maxs;
	}

	void SetUp() override {
		app::AbstractTest::SetUp();
		_brush.reset();
		ASSERT_TRUE(_brush.init());
		_node.setVolume(&_volume, false);
	}

	void TearDown() override {
		_brush.shutdown();
		_volume.clear();
		_sceneGraph.clear();
		_brushContext = {};
		app::AbstractTest::TearDown();
	}
};

TEST_F(PlaneBrushTest, testExtrude) {
	for (int x = 0; x < 3; ++x) {
		for (int y = 0; y < 3; ++y) {
			_volume.setVoxel(x, y, 0, _voxel);
		}
	}

	const int maxZ = 3;
	for (int z = 1; z <= maxZ; ++z) {
		ModifierVolumeWrapper wrapper(_node, _brush.modifierType());
		prepare(_brush, _voxel, _brushContext, voxel::FaceNames::PositiveZ, glm::ivec3(1, 1, z), glm::ivec3(1, 1, z));
		EXPECT_FALSE(voxel::isBlocked(wrapper.voxel(_brushContext.cursorPosition).getMaterial())) << "for z: " << z;
		_brush.preExecute(_brushContext, &_volume);
		EXPECT_TRUE(_brush.execute(_sceneGraph, wrapper, _brushContext)) << "for z: " << z;
		_brush.postExecute(_brushContext);
		EXPECT_TRUE(voxel::isBlocked(wrapper.voxel(_brushContext.cursorPosition).getMaterial()))
			<< "for z: " << z << " " << wrapper.dirtyRegion().toString();
		_brush.stop(_brushContext);
	}
}

TEST_F(PlaneBrushTest, testExtrudeThickness) {
	voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	for (int x = 0; x < 3; ++x) {
		for (int z = 0; z < 3; ++z) {
			_volume.setVoxel(x, 0, z, voxel);
		}
	}

	ModifierVolumeWrapper wrapper(_node, _brush.modifierType());
	prepare(_brush, voxel, _brushContext, voxel::FaceNames::PositiveY, glm::ivec3(1, 1, 1), glm::ivec3(3, 2, 3));
	EXPECT_FALSE(voxel::isBlocked(wrapper.voxel(_brushContext.cursorPosition).getMaterial()));
	_brush.preExecute(_brushContext, &_volume);
	EXPECT_TRUE(_brush.execute(_sceneGraph, wrapper, _brushContext));
	_brush.postExecute(_brushContext);
	const glm::ivec3 dirtyDim = wrapper.dirtyRegion().getDimensionsInVoxels();
	EXPECT_EQ(3, dirtyDim.x);
	EXPECT_EQ(2, dirtyDim.y);
	EXPECT_EQ(3, dirtyDim.z);
	for (int y = 1; y <= 2; ++y) {
		EXPECT_TRUE(voxel::isBlocked(wrapper.voxel(glm::ivec3(1, y, 1)).getMaterial())) << "for y: " << y;
		EXPECT_FALSE(voxel::isBlocked(wrapper.voxel(glm::ivec3(3, y, 3)).getMaterial())) << "for y: " << y;
	}
	_brush.stop(_brushContext);
}

} // namespace voxedit
