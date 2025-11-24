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

	void prepare(voxel::FaceNames face, const glm::ivec3 &mins, const glm::ivec3 &maxs) {
		_brushContext.cursorVoxel = _voxel;
		_brushContext.hitCursorVoxel = _brushContext.cursorVoxel;
		_brushContext.cursorPosition = mins;
		_brushContext.cursorFace = face;
		EXPECT_TRUE(_brush.beginBrush(_brushContext));
		ASSERT_FALSE(_brush.singleMode());
		EXPECT_TRUE(_brush.active());
		_brushContext.cursorPosition = maxs;
	}

	bool execute(ModifierVolumeWrapper &wrapper) {
		_brush.preExecute(_brushContext, &_volume);
		return _brush.execute(_sceneGraph, wrapper, _brushContext);
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
	SelectionManagerPtr selectionMgr = core::make_shared<SelectionManager>();
	for (int z = 1; z <= maxZ; ++z) {
		ModifierVolumeWrapper wrapper(_node, _brush.modifierType(), selectionMgr);
		prepare(voxel::FaceNames::PositiveZ, glm::ivec3(1, 1, z), glm::ivec3(1, 1, z));
		EXPECT_FALSE(voxel::isBlocked(_volume.voxel(_brushContext.cursorPosition).getMaterial())) << "for z: " << z;
		EXPECT_TRUE(execute(wrapper)) << "for z: " << z;
		EXPECT_TRUE(voxel::isBlocked(_volume.voxel(_brushContext.cursorPosition).getMaterial()))
			<< "for z: " << z << " " << wrapper.dirtyRegion().toString();
		_brush.endBrush(_brushContext);
	}
}

TEST_F(PlaneBrushTest, testExtrudeThickness) {
	for (int x = 0; x < 3; ++x) {
		for (int z = 0; z < 3; ++z) {
			_volume.setVoxel(x, 0, z, _voxel);
		}
	}

	SelectionManagerPtr selectionMgr = core::make_shared<SelectionManager>();
	ModifierVolumeWrapper wrapper(_node, _brush.modifierType(), selectionMgr);
	prepare(voxel::FaceNames::PositiveY, glm::ivec3(1, 1, 1), glm::ivec3(3, 2, 3));
	EXPECT_FALSE(voxel::isBlocked(_volume.voxel(_brushContext.cursorPosition).getMaterial()));
	EXPECT_TRUE(execute(wrapper));
	const glm::ivec3 dirtyDim = wrapper.dirtyRegion().getDimensionsInVoxels();
	EXPECT_EQ(3, dirtyDim.x);
	EXPECT_EQ(2, dirtyDim.y);
	EXPECT_EQ(3, dirtyDim.z);
	for (int y = 1; y <= 2; ++y) {
		EXPECT_TRUE(voxel::isBlocked(_volume.voxel(glm::ivec3(1, y, 1)).getMaterial())) << "for y: " << y;
		EXPECT_FALSE(voxel::isBlocked(_volume.voxel(glm::ivec3(3, y, 3)).getMaterial())) << "for y: " << y;
	}
	_brush.endBrush(_brushContext);
}

} // namespace voxedit
