/**
 * @file
 */

#include "voxedit-util/modifier/brush/PlaneBrush.h"
#include "app/tests/AbstractTest.h"
#include "math/Ray.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/ISceneRenderer.h"
#include "voxedit-util/modifier/IModifierRenderer.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/Brush.h"
#include "voxel/Face.h"
#include "voxel/Voxel.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"

namespace voxedit {

class PlaneBrushTest : public app::AbstractTest {
protected:
	PlaneBrush _brush;
	BrushContext _brushContext;
	voxel::RawVolume _volume{voxel::Region(0, 3)};
	scenegraph::SceneGraphNode _node{scenegraph::SceneGraphNodeType::Model};
	scenegraph::SceneGraph _sceneGraph;
	voxel::Voxel _voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	void prepare(voxel::FaceNames face, const math::Vec3i &mins, const math::Vec3i &maxs) {
		_brushContext.cursorVoxel = _voxel;
		_brushContext.hitCursorVoxel = _brushContext.cursorVoxel;
		_brushContext.cursorPosition = mins;
		_brushContext.cursorFace = face;
		EXPECT_TRUE(_brush.beginBrush(_brushContext));
		ASSERT_FALSE(_brush.strokeMode());
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
		_node.setUnownedVolume(&_volume);
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
		prepare(voxel::FaceNames::PositiveZ, math::Vec3i(1, 1, z), math::Vec3i(1, 1, z));
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

	ModifierVolumeWrapper wrapper(_node, _brush.modifierType());
	prepare(voxel::FaceNames::PositiveY, math::Vec3i(1, 1, 1), math::Vec3i(3, 2, 3));
	EXPECT_FALSE(voxel::isBlocked(_volume.voxel(_brushContext.cursorPosition).getMaterial()));
	EXPECT_TRUE(execute(wrapper));
	const math::Vec3i dirtyDim = wrapper.dirtyRegion().getDimensionsInVoxels();
	EXPECT_EQ(3, dirtyDim.x);
	EXPECT_EQ(2, dirtyDim.y);
	EXPECT_EQ(3, dirtyDim.z);
	for (int y = 1; y <= 2; ++y) {
		EXPECT_TRUE(voxel::isBlocked(_volume.voxel(math::Vec3i(1, y, 1)).getMaterial())) << "for y: " << y;
		EXPECT_FALSE(voxel::isBlocked(_volume.voxel(math::Vec3i(3, y, 3)).getMaterial())) << "for y: " << y;
	}
	_brush.endBrush(_brushContext);
}

TEST_F(PlaneBrushTest, testReuseAfterReleaseAtSamePosition) {
	for (int x = 0; x < 3; ++x) {
		for (int z = 0; z < 3; ++z) {
			_volume.setVoxel(x, 0, z, _voxel);
		}
	}

	{
		ModifierVolumeWrapper wrapper(_node, _brush.modifierType());
		prepare(voxel::FaceNames::PositiveY, math::Vec3i(1, 1, 1), math::Vec3i(1, 1, 1));
		EXPECT_TRUE(execute(wrapper));
		EXPECT_TRUE(voxel::isBlocked(_volume.voxel(1, 1, 1).getMaterial()));
		EXPECT_FALSE(voxel::isBlocked(_volume.voxel(1, 2, 1).getMaterial()));
		_brush.endBrush(_brushContext);
		_brush.resetRuntime();
	}
	{
		ModifierVolumeWrapper wrapper(_node, _brush.modifierType());
		prepare(voxel::FaceNames::PositiveY, math::Vec3i(1, 1, 1), math::Vec3i(1, 1, 1));
		EXPECT_TRUE(execute(wrapper));
		EXPECT_FALSE(voxel::isBlocked(_volume.voxel(1, 2, 1).getMaterial()))
			<< "Repeating the click on the already filled voxel must not grow the plane";
		_brush.endBrush(_brushContext);
		_brush.resetRuntime();
	}
	{
		ModifierVolumeWrapper wrapper(_node, _brush.modifierType());
		_brushContext.hitCursorVoxel = _volume.voxel(1, 1, 1);
		prepare(voxel::FaceNames::PositiveY, math::Vec3i(1, 2, 1), math::Vec3i(1, 2, 1));
		EXPECT_TRUE(execute(wrapper));
		EXPECT_TRUE(voxel::isBlocked(_volume.voxel(1, 2, 1).getMaterial()));
		_brush.endBrush(_brushContext);
	}
}

TEST_F(PlaneBrushTest, testDragProjectsAlongCapturedFaceNormal) {
	for (voxel::FaceNames face :
		 {voxel::FaceNames::PositiveX, voxel::FaceNames::NegativeX, voxel::FaceNames::PositiveY,
		  voxel::FaceNames::NegativeY, voxel::FaceNames::PositiveZ, voxel::FaceNames::NegativeZ}) {
		SCOPED_TRACE((int)face);
		const math::Vec3i start(1);
		const math::Vec3f direction = math::normalize(math::Vec3f(1, -1, 1));
		const math::Vec3f origin = math::Vec3f(start) + 0.5f - direction * 10.0f;
		math::Vec3i position;
		EXPECT_FALSE(_brush.projectCursor(math::Ray(origin, direction), position));
		prepare(face, start, start);
		ASSERT_TRUE(_brush.projectCursor(math::Ray(origin, direction), position));
		EXPECT_EQ(start, position);
		const int idx = math::getIndexForAxis(voxel::faceToAxis(face));
		math::Vec3f moved = origin;
		moved[idx] += 3.0f;
		ASSERT_TRUE(_brush.projectCursor(math::Ray(moved, direction), position));
		math::Vec3i expected = start;
		expected[idx] += 3;
		EXPECT_EQ(expected, position);
		math::Vec3f parallel(0.0f);
		parallel[idx] = 1.0f;
		ASSERT_TRUE(_brush.projectCursor(math::Ray(moved, parallel), position));
		EXPECT_EQ(start, position);
		_brush.endBrush(_brushContext);
		EXPECT_FALSE(_brush.projectCursor(math::Ray(origin, direction), position));
	}
}

} // namespace voxedit
