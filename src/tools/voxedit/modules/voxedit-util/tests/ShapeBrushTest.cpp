/**
 * @file
 */

#include "voxedit-util/modifier/brush/ShapeBrush.h"
#include "app/tests/AbstractTest.h"
#include "scenegraph/SceneGraph.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/tests/AbstractBrushTest.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxedit {

class ShapeBrushTest : public app::AbstractTest {
protected:
	class TestShapeBrush : public ShapeBrush {
	public:
		using ShapeBrush::setShapeType;
	};

	void ellipse(const glm::ivec2 &mins2, const glm::ivec2 &maxs2) {
		TestShapeBrush brush;
		BrushContext brushContext;
		ASSERT_TRUE(brush.init());
		brush.setShapeType(ShapeType::Ellipse);

		const glm::ivec3 mins(0, 0, 31);
		const glm::ivec3 maxs(31, 31, 31);
		const voxel::Region vregion(mins, maxs);
		voxel::RawVolume volume(vregion);
		scenegraph::SceneGraph sceneGraph;
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setUnownedVolume(&volume);
		ModifierVolumeWrapper wrapper(node, ModifierType::Place);
		brush.preExecute(brushContext, wrapper.volume());
		brushContext.cursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
		brushContext.cursorPosition = glm::ivec3(mins2.x, mins2.y, maxs.z);
		brushContext.cursorFace = voxel::FaceNames::PositiveX;
		EXPECT_TRUE(brush.beginBrush(brushContext));
		EXPECT_TRUE(brush.active());
		brushContext.cursorPosition = glm::ivec3(maxs2.x, maxs2.y, maxs.z);
		brush.step(brushContext);
		const voxel::Region region = brush.calcRegion(brushContext);
		const glm::ivec3 &lc = region.getLowerCorner();
		const glm::ivec3 &uc = region.getUpperCorner();
		ASSERT_EQ(lc, glm::ivec3(mins2.x, mins2.y, maxs.z));
		ASSERT_EQ(uc, glm::ivec3(maxs2.x, maxs2.y, maxs.z));
		brush.execute(sceneGraph, wrapper, brushContext);
		const int voxelCount = voxelutil::countVoxels(volume);
		ASSERT_LT(voxelCount, region.voxels());
		ASSERT_GE(voxelCount, 80); // a 10x10 ellipse has approximately pi*5*5 ~= 78-80 voxels
		EXPECT_FALSE(voxel::isAir(volume.voxel(maxs2.x, mins2.y + (maxs2.y - mins2.y) / 2, lc.z).getMaterial()))
			<< "side voxel expected in the middle of the aabb: " << voxelCount << " voxels in total";
		EXPECT_FALSE(voxel::isAir(volume.voxel(mins2.x, mins2.y + (maxs2.y - mins2.y) / 2, lc.z).getMaterial()))
			<< "side voxel expected in the middle of the aabb: " << voxelCount << " voxels in total";
		EXPECT_FALSE(voxel::isAir(volume.voxel(mins2.x + (maxs2.x - mins2.x) / 2, maxs2.y, lc.z).getMaterial()))
			<< "top voxel expected in the middle of the aabb: " << voxelCount << " voxels in total";
		EXPECT_FALSE(voxel::isAir(volume.voxel(mins2.x + (maxs2.x - mins2.x) / 2, mins2.y, lc.z).getMaterial()))
			<< "bottom voxel expected in the middle of the aabb: " << voxelCount << " voxels in total";
		brush.shutdown();
	}

	void prepare(ShapeBrush &brush, BrushContext &brushContext, const glm::ivec3 &mins, const glm::ivec3 &maxs) {
		brushContext.cursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
		brushContext.cursorPosition = mins;
		brushContext.cursorFace = voxel::FaceNames::PositiveX;
		EXPECT_TRUE(brush.beginBrush(brushContext));
		if (brush.singleMode()) {
			EXPECT_FALSE(brush.active());
		} else {
			EXPECT_TRUE(brush.active());
			brushContext.cursorPosition = maxs;
			brush.step(brushContext);
		}
	}

	void testMirror(math::Axis axis, const glm::ivec3 &expectedMins, const glm::ivec3 &expectedMaxs) {
		ShapeBrush brush;
		BrushContext brushContext;
		ASSERT_TRUE(brush.init());
		const glm::ivec3 regMins(-2);
		const glm::ivec3 regMaxs = regMins;
		prepare(brush, brushContext, regMins, regMaxs);
		EXPECT_TRUE(brush.setMirrorAxis(axis, glm::ivec3(0)));
		const voxel::Region region(-3, 3);
		voxel::RawVolume volume(region);
		scenegraph::SceneGraph sceneGraph;
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setUnownedVolume(&volume);
		ModifierVolumeWrapper wrapper(node, ModifierType::Place);
		brush.preExecute(brushContext, wrapper.volume());
		brush.execute(sceneGraph, wrapper, brushContext);
		const voxel::Region dirtyRegion = wrapper.dirtyRegion();
		EXPECT_TRUE(dirtyRegion.isValid());
		EXPECT_FALSE(voxel::isAir(brushContext.cursorVoxel.getMaterial()));
		EXPECT_TRUE(brushContext.cursorVoxel.isSame(volume.voxel(regMins)));
		EXPECT_EQ(dirtyRegion.getLowerCorner(), expectedMins);
		EXPECT_EQ(dirtyRegion.getUpperCorner(), expectedMaxs);
		brush.shutdown();
	}
};

TEST_F(ShapeBrushTest, testCenterPositive) {
	ShapeBrush brush;
	BrushContext brushContext;
	ASSERT_TRUE(brush.init());
	brush.setCenterMode();

	const glm::ivec3 mins(0);
	const glm::ivec3 maxs(1);
	prepare(brush, brushContext, mins, maxs);
	const voxel::Region region = brush.calcRegion(brushContext);
	const glm::ivec3 &dim = region.getDimensionsInVoxels();
	EXPECT_EQ(glm::ivec3(3), dim);
	brush.shutdown();
}

TEST_F(ShapeBrushTest, testCenterNegative) {
	ShapeBrush brush;
	BrushContext brushContext;
	ASSERT_TRUE(brush.init());
	brush.setCenterMode();
	prepare(brush, brushContext, glm::ivec3(0), glm::ivec3(-1));
	const voxel::Region region = brush.calcRegion(brushContext);
	const glm::ivec3 &dim = region.getDimensionsInVoxels();
	EXPECT_EQ(glm::ivec3(3), dim);
	brush.shutdown();
}

TEST_F(ShapeBrushTest, testModifierStartStop) {
	ShapeBrush brush;
	BrushContext brushContext;
	ASSERT_TRUE(brush.init());
	EXPECT_TRUE(brush.beginBrush(brushContext));
	EXPECT_TRUE(brush.active());
	brush.endBrush(brushContext);
	EXPECT_FALSE(brush.active());
	brush.shutdown();
}

TEST_F(ShapeBrushTest, testModifierDim) {
	ShapeBrush brush;
	BrushContext brushContext;
	ASSERT_TRUE(brush.init());
	prepare(brush, brushContext, glm::ivec3(-1), glm::ivec3(1));
	const voxel::Region region = brush.calcRegion(brushContext);
	const glm::ivec3 &dim = region.getDimensionsInVoxels();
	EXPECT_EQ(glm::ivec3(3), dim);
	brush.shutdown();
}

TEST_F(ShapeBrushTest, testModifierActionMirrorAxisX) {
	testMirror(math::Axis::X, glm::ivec3(-2), glm::ivec3(-2, -2, 1));
}

TEST_F(ShapeBrushTest, testModifierActionMirrorAxisY) {
	testMirror(math::Axis::Y, glm::ivec3(-2), glm::ivec3(-2, 1, -2));
}

TEST_F(ShapeBrushTest, testModifierActionMirrorAxisZ) {
	testMirror(math::Axis::Z, glm::ivec3(-2), glm::ivec3(1, -2, -2));
}

TEST_P(BrushTestParamTest, testShapeBrush) {
	ShapeBrush brush;
	testPlaceAndOverride(brush);
}

TEST_F(ShapeBrushTest, testEllipse) {
	ellipse(glm::ivec2(10, 10), glm::ivec2(19, 19));
}

}; // namespace voxedit
