/**
 * @file
 */

#pragma once

#include "app/tests/AbstractTest.h"
#include "math/Axis.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/Brush.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"

namespace voxedit {

struct BrushCombination {
	voxel::FaceNames face;
	ModifierType modifier;
};

class BrushTestParamTest : public app::AbstractTest, public ::testing::WithParamInterface<BrushCombination> {
protected:
	void testPlaceAndOverride(Brush &brush) {
		const BrushCombination &param = GetParam();
		ASSERT_EQ(brush.modifierType(param.modifier), param.modifier) << "modifier not supported by brush type";
		ASSERT_TRUE(param.modifier == ModifierType::Place || param.modifier == ModifierType::Override)
			<< "this tests only support place and override modifiers";
		ASSERT_TRUE(brush.init()) << "failed to initialize brush";
		voxel::RawVolume volume(voxel::Region(0, 3));

		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setVolume(&volume, false);
		scenegraph::SceneGraph sceneGraph;
		ModifierVolumeWrapper wrapper(node, param.modifier);
		BrushContext brushContext;
		brushContext.referencePos = volume.region().getCenter();
		brushContext.cursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 0);
		brushContext.cursorFace = param.face;

		const int max = 3;
		for (int i = 0; i <= max; ++i) {
			int coord = i;
			if (voxel::isNegativeFace(brushContext.cursorFace)) {
				coord = max - i;
			}
			brushContext.cursorPosition = glm::ivec3(0);
			brushContext.cursorPosition[math::getIndexForAxis(voxel::faceToAxis(brushContext.cursorFace))] = coord;
			brush.preExecute(brushContext, &volume);
			ASSERT_TRUE(brush.execute(sceneGraph, wrapper, brushContext)) << "for coord: " << i;
			ASSERT_TRUE(voxel::isBlocked(wrapper.voxel(brushContext.cursorPosition).getMaterial()))
				<< "for coord: " << i;
			brushContext.hitCursorVoxel = wrapper.voxel(brushContext.cursorPosition);
		}
		brush.shutdown();
	}
};

inline ::std::ostream &operator<<(::std::ostream &os, const BrushCombination &state) {
	const char *modifier;
	switch (state.modifier) {
	case ModifierType::Place:
		modifier = "Place";
		break;
	case ModifierType::Override:
		modifier = "Override";
		break;
	case ModifierType::Erase:
		modifier = "Erase";
		break;
	case ModifierType::Paint:
		modifier = "Paint";
		break;
	default:
		modifier = "Unknown";
		break;
	}
	const char *face = voxel::faceNameString(state.face);
	return os << "face[" << face << "], modifier[" << modifier << "]";
}

} // namespace voxedit
