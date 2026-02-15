/**
 * @file
 */

#include "../MementoHandler.h"
#include "app/tests/AbstractTest.h"
#include "core/Pair.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "math/tests/TestMathHelper.h"
#include "scenegraph/IKConstraint.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphTransform.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeRotator.h"
#include "voxelutil/VolumeVisitor.h"

namespace memento {

class MementoHandlerTest : public app::AbstractTest {
private:
	using Super = app::AbstractTest;

protected:
	static core::UUID toFakeUUID(int id) {
		if (id == InvalidNodeId) {
			return core::UUID();
		}
		return core::UUID(id);
	}

	class TestMementoHandler : public MementoHandler {
	private:
		using Super = MementoHandler;

	public:
		bool markUndo(int parentId, int nodeId, int referenceId, const core::String &name,
					  scenegraph::SceneGraphNodeType nodeType, const voxel::RawVolume *volume, MementoType type,
					  const voxel::Region &region = voxel::Region::InvalidRegion,
					  const glm::vec3 &pivot = glm::vec3(0.0f),
					  const scenegraph::SceneGraphKeyFramesMap &allKeyFrames = {}, const palette::Palette &palette = {},
					  const palette::NormalPalette &normalPalette = {},
					  const scenegraph::SceneGraphNodeProperties &properties = {}) {
			return Super::markUndo(toFakeUUID(parentId), toFakeUUID(nodeId), toFakeUUID(referenceId), name, nodeType,
								   volume, type, region, pivot, allKeyFrames, palette, normalPalette, properties);
		}
	};

	TestMementoHandler _mementoHandler;
	scenegraph::SceneGraph _sceneGraph;

	core::SharedPtr<voxel::RawVolume> create(int size) const {
		const voxel::Region region(glm::ivec3(0), glm::ivec3(size - 1));
		EXPECT_EQ(size, region.getWidthInVoxels());
		return core::make_shared<voxel::RawVolume>(region);
	}

	void SetUp() override {
		Super::SetUp();
		ASSERT_TRUE(_mementoHandler.init());
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model, core::UUID(1));
		node.setVolume(new voxel::RawVolume(voxel::Region(0, 0, 0, 1, 1, 1)), true);
		node.setName("Node name");
		_sceneGraph.emplace(core::move(node));
	}

	void TearDown() override {
		_mementoHandler.shutdown();
		_sceneGraph.clear();
		Super::TearDown();
	}

	static inline MementoState firstState(const MementoStateGroup &group) {
		core_assert(!group.states.empty());
		return group.states[0];
	}
	// Helper function to verify exact voxel states in a memento volume
	void verifyVoxelState(const MementoState &state, const core::String &description,
						  const core::DynamicArray<core::Pair<glm::ivec3, uint8_t>> &expectedVoxels,
						  const core::DynamicArray<glm::ivec3> &expectedAirVoxels = core::DynamicArray<glm::ivec3>()) {
		ASSERT_TRUE(state.hasVolumeData()) << "State " << description << " should have volume data";

		// Extract volume from memento state
		voxel::RawVolume volume(state.dataRegion());
		ASSERT_TRUE(state.data.toVolume(&volume, state.data, state.dataRegion()))
			<< "Failed to extract volume from state " << description;

		// Check expected voxels with specific colors
		for (const core::Pair<glm::ivec3, uint8_t> &expectedVoxel : expectedVoxels) {
			const glm::ivec3 &pos = expectedVoxel.first;
			const uint8_t expectedColor = expectedVoxel.second;

			ASSERT_TRUE(volume.region().containsPoint(pos)) << "State " << description << ": Position " << pos.x << ","
															<< pos.y << "," << pos.z << " is outside volume region";
			const voxel::Voxel voxelAtPos = volume.voxel(pos);
			EXPECT_EQ(voxel::VoxelType::Generic, voxelAtPos.getMaterial())
				<< "State " << description << ": Expected Generic voxel at " << pos.x << "," << pos.y << "," << pos.z;
			EXPECT_EQ(expectedColor, voxelAtPos.getColor())
				<< "State " << description << ": Expected color " << (int)expectedColor << " at " << pos.x << ","
				<< pos.y << "," << pos.z << " but got " << (int)voxelAtPos.getColor();
		}

		// Check expected air voxels
		for (const glm::ivec3 &pos : expectedAirVoxels) {
			if (volume.region().containsPoint(pos)) {
				const voxel::Voxel voxelAtPos = volume.voxel(pos);
				EXPECT_TRUE(voxel::isAir(voxelAtPos.getMaterial()))
					<< "State " << description << ": Expected empty voxel at " << pos.x << "," << pos.y << "," << pos.z
					<< " but got material type " << (int)voxelAtPos.getMaterial();
			}
		}
	};
};

TEST_F(MementoHandlerTest, testMarkUndo) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	core::SharedPtr<voxel::RawVolume> third = create(3);
	EXPECT_FALSE(_mementoHandler.canRedo());
	EXPECT_FALSE(_mementoHandler.canUndo());

	_mementoHandler.markUndo(0, 0, InvalidNodeId, "", scenegraph::SceneGraphNodeType::Max, first.get(),
							 MementoType::Modification);
	EXPECT_FALSE(_mementoHandler.canRedo())
		<< "Without a second entry and without undoing something before, you can't redo anything";
	EXPECT_FALSE(_mementoHandler.canUndo())
		<< "Without a second entry, you can't undo anything, because it is your initial state";
	EXPECT_EQ(1, (int)_mementoHandler.stateSize());
	EXPECT_EQ(0, (int)_mementoHandler.statePosition());

	_mementoHandler.markUndo(0, 0, InvalidNodeId, "", scenegraph::SceneGraphNodeType::Max, second.get(),
							 MementoType::Modification);
	EXPECT_FALSE(_mementoHandler.canRedo());
	EXPECT_TRUE(_mementoHandler.canUndo());
	EXPECT_EQ(2, (int)_mementoHandler.stateSize());
	EXPECT_EQ(1, (int)_mementoHandler.statePosition());

	_mementoHandler.markUndo(0, 0, InvalidNodeId, "", scenegraph::SceneGraphNodeType::Max, third.get(),
							 MementoType::Modification);
	EXPECT_FALSE(_mementoHandler.canRedo());
	EXPECT_TRUE(_mementoHandler.canUndo());
	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
	EXPECT_EQ(2, (int)_mementoHandler.statePosition());
}

TEST_F(MementoHandlerTest, testUndoRedo) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	core::SharedPtr<voxel::RawVolume> third = create(3);
	ASSERT_TRUE(_mementoHandler.markUndo(0, 0, InvalidNodeId, "", scenegraph::SceneGraphNodeType::Max, first.get(),
										 MementoType::Modification));
	ASSERT_TRUE(_mementoHandler.markUndo(0, 0, InvalidNodeId, "", scenegraph::SceneGraphNodeType::Max, second.get(),
										 MementoType::Modification));
	ASSERT_TRUE(_mementoHandler.markUndo(0, 0, InvalidNodeId, "", scenegraph::SceneGraphNodeType::Max, third.get(),
										 MementoType::Modification));

	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
	EXPECT_EQ(2, _mementoHandler.statePosition());
	EXPECT_TRUE(_mementoHandler.canUndo());
	EXPECT_FALSE(_mementoHandler.canRedo());

	const MementoState &undoThird = firstState(_mementoHandler.undo());
	ASSERT_TRUE(undoThird.hasVolumeData());
	EXPECT_EQ(2, undoThird.dataRegion().getWidthInVoxels());
	EXPECT_TRUE(_mementoHandler.canRedo());
	EXPECT_TRUE(_mementoHandler.canUndo());
	EXPECT_EQ(1, (int)_mementoHandler.statePosition());

	MementoState undoSecond = firstState(_mementoHandler.undo());
	ASSERT_TRUE(undoSecond.hasVolumeData());
	EXPECT_EQ(1, undoSecond.dataRegion().getWidthInVoxels());
	EXPECT_TRUE(_mementoHandler.canRedo());
	EXPECT_FALSE(_mementoHandler.canUndo());
	EXPECT_EQ(0, (int)_mementoHandler.statePosition());

	const MementoState &redoSecond = firstState(_mementoHandler.redo());
	ASSERT_TRUE(redoSecond.hasVolumeData());
	EXPECT_EQ(2, redoSecond.dataRegion().getWidthInVoxels());
	EXPECT_TRUE(_mementoHandler.canRedo());
	EXPECT_TRUE(_mementoHandler.canUndo());
	EXPECT_EQ(1, (int)_mementoHandler.statePosition());

	undoSecond = firstState(_mementoHandler.undo());
	ASSERT_TRUE(undoSecond.hasVolumeData());
	EXPECT_EQ(1, undoSecond.dataRegion().getWidthInVoxels());
	EXPECT_TRUE(_mementoHandler.canRedo());
	EXPECT_FALSE(_mementoHandler.canUndo());
	EXPECT_EQ(0, (int)_mementoHandler.statePosition());

	const MementoStateGroup &undoNotPossibleGroup = _mementoHandler.undo();
	ASSERT_TRUE(undoNotPossibleGroup.states.empty());
}

TEST_F(MementoHandlerTest, testUndoRedoDifferentNodes) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	core::SharedPtr<voxel::RawVolume> third = create(3);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 0", scenegraph::SceneGraphNodeType::Model, first.get(),
							 MementoType::Modification);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 1", scenegraph::SceneGraphNodeType::Model, second.get(),
							 MementoType::SceneNodeAdded);
	_mementoHandler.markUndo(0, 2, InvalidNodeId, "Node 2", scenegraph::SceneGraphNodeType::Model, third.get(),
							 MementoType::SceneNodeAdded);
	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
	EXPECT_EQ(2, _mementoHandler.statePosition());
	EXPECT_TRUE(_mementoHandler.canUndo());
	EXPECT_FALSE(_mementoHandler.canRedo());

	MementoState state;
	{
		// undo of adding node 2
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(core::UUID(2), state.nodeUUID);
		EXPECT_EQ(MementoType::SceneNodeAdded, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
	}
	{
		// undo of adding node 1
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(core::UUID(1), state.nodeUUID);
		EXPECT_EQ(MementoType::SceneNodeAdded, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	}
	EXPECT_FALSE(_mementoHandler.canUndo());
	EXPECT_TRUE(_mementoHandler.canRedo());
	{
		// redo adding node 1
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(core::UUID(1), state.nodeUUID);
		EXPECT_EQ(MementoType::SceneNodeAdded, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	}
}

TEST_F(MementoHandlerTest, testCutStates) {
	core::SharedPtr<voxel::RawVolume> second = create(2);
	for (int i = 0; i < 4; ++i) {
		auto v = create(1);
		_mementoHandler.markUndo(0, i, InvalidNodeId, "", scenegraph::SceneGraphNodeType::Max, v.get(),
								 MementoType::Modification);
	}
	EXPECT_EQ(4, (int)_mementoHandler.stateSize());
	EXPECT_EQ(3, _mementoHandler.statePosition());
	_mementoHandler.undo();
	_mementoHandler.undo();
	EXPECT_EQ(1, _mementoHandler.statePosition());
	_mementoHandler.markUndo(0, 4, InvalidNodeId, "Node 4", scenegraph::SceneGraphNodeType::Model, second.get(),
							 MementoType::SceneNodeAdded);
	EXPECT_EQ(2, _mementoHandler.statePosition());
	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
}

TEST_F(MementoHandlerTest, testAddNewNode) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	core::SharedPtr<voxel::RawVolume> third = create(3);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 0", scenegraph::SceneGraphNodeType::Max, first.get(),
							 MementoType::Modification);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 0 Modified", scenegraph::SceneGraphNodeType::Max, second.get(),
							 MementoType::Modification);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 1", scenegraph::SceneGraphNodeType::Max, third.get(),
							 MementoType::SceneNodeAdded);
	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
	EXPECT_EQ(2, _mementoHandler.statePosition());
	EXPECT_TRUE(_mementoHandler.canUndo());
	EXPECT_FALSE(_mementoHandler.canRedo());

	MementoState state;

	{
		// undo of adding node 1
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(core::UUID(1), state.nodeUUID);
		EXPECT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
	}
	{
		// undo modification in node 0
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(core::UUID(0), state.nodeUUID);
		EXPECT_TRUE(state.hasVolumeData());
		EXPECT_EQ(1, state.dataRegion().getWidthInVoxels());
	}

	{
		// redo modification in node 0
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(core::UUID(0), state.nodeUUID);
		EXPECT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	}
	{
		// redo of adding node 1
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(core::UUID(1), state.nodeUUID);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
	}
}

TEST_F(MementoHandlerTest, testAddNewNodeSimple) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 0", scenegraph::SceneGraphNodeType::Max, first.get(),
							 MementoType::Modification);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 1", scenegraph::SceneGraphNodeType::Max, second.get(),
							 MementoType::SceneNodeAdded);
	MementoState state;

	EXPECT_EQ(2, (int)_mementoHandler.stateSize());
	EXPECT_EQ(1, _mementoHandler.statePosition());

	{
		// undo adding node 1
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(0, _mementoHandler.statePosition());
		EXPECT_EQ(core::UUID(1), state.nodeUUID);
		EXPECT_EQ("Node 1", state.name);
		EXPECT_EQ(MementoType::SceneNodeAdded, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
		EXPECT_FALSE(_mementoHandler.canUndo());
		EXPECT_TRUE(_mementoHandler.canRedo());
	}
	{
		// redo adding node 1
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(1, _mementoHandler.statePosition());
		EXPECT_EQ(core::UUID(1), state.nodeUUID);
		EXPECT_EQ(MementoType::SceneNodeAdded, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canUndo());
		EXPECT_FALSE(_mementoHandler.canRedo());
	}
}

TEST_F(MementoHandlerTest, testDeleteNode) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 1", scenegraph::SceneGraphNodeType::Max, first.get(),
							 MementoType::Modification);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 2 Added", scenegraph::SceneGraphNodeType::Max, second.get(),
							 MementoType::SceneNodeAdded);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 2 Deleted", scenegraph::SceneGraphNodeType::Max, second.get(),
							 MementoType::SceneNodeRemoved);

	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
	EXPECT_EQ(2, _mementoHandler.statePosition());

	MementoState state;

	{
		// undo adding node 1
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(core::UUID(1), state.nodeUUID);
		EXPECT_EQ("Node 2 Deleted", state.name);
		EXPECT_EQ(MementoType::SceneNodeRemoved, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	}
	{
		// redo adding node 1
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(core::UUID(1), state.nodeUUID);
		EXPECT_EQ("Node 2 Deleted", state.name);
		EXPECT_EQ(MementoType::SceneNodeRemoved, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	}
}

TEST_F(MementoHandlerTest, testAddNewNodeExt) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	core::SharedPtr<voxel::RawVolume> third = create(3);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 0", scenegraph::SceneGraphNodeType::Max, first.get(),
							 MementoType::Modification);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 0 Modified", scenegraph::SceneGraphNodeType::Max, second.get(),
							 MementoType::Modification);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 1 Added", scenegraph::SceneGraphNodeType::Max, third.get(),
							 MementoType::SceneNodeAdded);

	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
	EXPECT_EQ(2, _mementoHandler.statePosition());

	MementoState state;

	{
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(1, _mementoHandler.statePosition());
		EXPECT_EQ(core::UUID(1), state.nodeUUID);
		EXPECT_EQ(MementoType::SceneNodeAdded, state.type);
		EXPECT_EQ("Node 1 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
	}

	{
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(0, _mementoHandler.statePosition());
		EXPECT_EQ(core::UUID(0), state.nodeUUID);
		EXPECT_EQ(MementoType::Modification, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(1, state.dataRegion().getWidthInVoxels());
	}

	{
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(core::UUID(0), state.nodeUUID);
		EXPECT_EQ(MementoType::Modification, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	}

	{
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(core::UUID(1), state.nodeUUID);
		EXPECT_EQ("Node 1 Added", state.name);
		EXPECT_EQ(MementoType::SceneNodeAdded, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
	}
}

TEST_F(MementoHandlerTest, testDeleteNodeExt) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	core::SharedPtr<voxel::RawVolume> third = create(3);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 1", scenegraph::SceneGraphNodeType::Max, first.get(),
							 MementoType::Modification);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 1 Modified", scenegraph::SceneGraphNodeType::Max, second.get(),
							 MementoType::Modification);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 2 Added", scenegraph::SceneGraphNodeType::Max, third.get(),
							 MementoType::SceneNodeAdded);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 2 Deleted", scenegraph::SceneGraphNodeType::Max, third.get(),
							 MementoType::SceneNodeRemoved);

	EXPECT_EQ(4, (int)_mementoHandler.stateSize());
	EXPECT_EQ(3, _mementoHandler.statePosition());

	MementoState state;

	{
		// undo the deletion of node 1
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(2, _mementoHandler.statePosition());
		EXPECT_EQ(core::UUID(1), state.nodeUUID);
		EXPECT_EQ("Node 2 Deleted", state.name);
		EXPECT_EQ(MementoType::SceneNodeRemoved, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canUndo());
	}

	{
		// undo the creation of node 1
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(1, _mementoHandler.statePosition());
		EXPECT_EQ(core::UUID(1), state.nodeUUID);
		EXPECT_EQ("Node 2 Added", state.name);
		EXPECT_EQ(MementoType::SceneNodeAdded, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canUndo());
	}

	{
		// undo the modification of node 0
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(0, _mementoHandler.statePosition());
		EXPECT_EQ(core::UUID(0), state.nodeUUID);
		EXPECT_EQ(MementoType::Modification, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(1, state.dataRegion().getWidthInVoxels());
		EXPECT_FALSE(_mementoHandler.canUndo());
	}

	{
		// redo the modification of node 0
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(1, _mementoHandler.statePosition());
		EXPECT_EQ(core::UUID(0), state.nodeUUID);
		EXPECT_EQ("Node 1 Modified", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canRedo());
	}

	{
		// redo the add of node 1
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(2, _mementoHandler.statePosition());
		EXPECT_EQ(core::UUID(1), state.nodeUUID);
		EXPECT_EQ("Node 2 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canRedo());
	}

	{
		// redo the removal of node 1
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(3, _mementoHandler.statePosition());
		EXPECT_EQ(core::UUID(1), state.nodeUUID);
		EXPECT_EQ("Node 2 Deleted", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_FALSE(_mementoHandler.canRedo());
	}

	{
		// undo the removal of node 1
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(2, _mementoHandler.statePosition());
		EXPECT_EQ(core::UUID(1), state.nodeUUID);
		EXPECT_EQ("Node 2 Deleted", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canUndo());
	}

	{
		// redo the removal of node 1
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(3, _mementoHandler.statePosition());
		EXPECT_EQ(core::UUID(1), state.nodeUUID);
		EXPECT_EQ("Node 2 Deleted", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_FALSE(_mementoHandler.canRedo());
	}

	{
		// undo the removal of node 1
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(2, _mementoHandler.statePosition());
		EXPECT_EQ(core::UUID(1), state.nodeUUID);
		EXPECT_EQ("Node 2 Deleted", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canUndo());
	}

	{
		// undo the creation of node 1
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(1, _mementoHandler.statePosition());
		EXPECT_EQ(core::UUID(1), state.nodeUUID);
		EXPECT_EQ("Node 2 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_TRUE(_mementoHandler.canUndo());
	}
}

TEST_F(MementoHandlerTest, testAddNewNodeMultiple) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	core::SharedPtr<voxel::RawVolume> third = create(3);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 0", scenegraph::SceneGraphNodeType::Max, first.get(),
							 MementoType::Modification);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 1 Added", scenegraph::SceneGraphNodeType::Max, second.get(),
							 MementoType::SceneNodeAdded);
	_mementoHandler.markUndo(0, 2, InvalidNodeId, "Node 2 Added", scenegraph::SceneGraphNodeType::Max, third.get(),
							 MementoType::SceneNodeAdded);

	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
	EXPECT_EQ(2, _mementoHandler.statePosition());

	MementoState state;

	{
		// undo the creation of node 2
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(1, _mementoHandler.statePosition());
		EXPECT_EQ(core::UUID(2), state.nodeUUID);
		EXPECT_EQ("Node 2 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_TRUE(_mementoHandler.canUndo());
	}
	{
		// undo the creation of node 1
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(0, _mementoHandler.statePosition());
		EXPECT_EQ(core::UUID(1), state.nodeUUID);
		EXPECT_EQ("Node 1 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_FALSE(_mementoHandler.canUndo());
	}
	{
		// redo the creation of node 1
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(1, _mementoHandler.statePosition());
		EXPECT_EQ(core::UUID(1), state.nodeUUID);
		EXPECT_EQ("Node 1 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canRedo());
	}
	{
		// redo the creation of node 2
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(2, _mementoHandler.statePosition());
		EXPECT_EQ(core::UUID(2), state.nodeUUID);
		EXPECT_EQ("Node 2 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_FALSE(_mementoHandler.canRedo());
	}
}

TEST_F(MementoHandlerTest, testAddNewNodeEdit) {
	core::SharedPtr<voxel::RawVolume> first = create(1);
	core::SharedPtr<voxel::RawVolume> second = create(2);
	core::SharedPtr<voxel::RawVolume> third = create(3);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Node 1", scenegraph::SceneGraphNodeType::Max, first.get(),
							 MementoType::Modification);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 2 Added", scenegraph::SceneGraphNodeType::Max, second.get(),
							 MementoType::SceneNodeAdded);
	_mementoHandler.markUndo(0, 1, InvalidNodeId, "Node 2 Modified", scenegraph::SceneGraphNodeType::Max, third.get(),
							 MementoType::Modification);

	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
	EXPECT_EQ(2, _mementoHandler.statePosition());

	MementoState state;

	{
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(1, _mementoHandler.statePosition());
		EXPECT_EQ(core::UUID(1), state.nodeUUID);
		EXPECT_EQ("Node 2 Modified", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canUndo());
	}
	{
		state = firstState(_mementoHandler.undo());
		EXPECT_EQ(0, _mementoHandler.statePosition());
		EXPECT_EQ(core::UUID(1), state.nodeUUID);
		EXPECT_EQ("Node 2 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_FALSE(_mementoHandler.canUndo());
	}
	{
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(1, _mementoHandler.statePosition());
		EXPECT_EQ(core::UUID(1), state.nodeUUID);
		EXPECT_EQ("Node 2 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(_mementoHandler.canRedo());
	}
	{
		state = firstState(_mementoHandler.redo());
		EXPECT_EQ(2, _mementoHandler.statePosition());
		EXPECT_EQ(core::UUID(1), state.nodeUUID);
		EXPECT_EQ("Node 2 Modified", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_FALSE(_mementoHandler.canRedo());
	}
}

TEST_F(MementoHandlerTest, testSceneNodeRenamed) {
	scenegraph::SceneGraphNode *node = _sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	_mementoHandler.markInitialNodeState(_sceneGraph, *node);
	node->setName("Name after");
	_mementoHandler.markNodeRenamed(_sceneGraph, *node);
	EXPECT_EQ(2, (int)_mementoHandler.stateSize());
	EXPECT_TRUE(_mementoHandler.canUndo());
	MementoState stateUndo = firstState(_mementoHandler.undo());
	EXPECT_EQ(stateUndo.name, "Node name");
	EXPECT_FALSE(_mementoHandler.canUndo());
	MementoState stateRedo = firstState(_mementoHandler.redo());
	EXPECT_EQ(stateRedo.name, "Name after");
}

TEST_F(MementoHandlerTest, testSceneNodeIKConstraint) {
	scenegraph::SceneGraphNode *node = _sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	_mementoHandler.markInitialNodeState(_sceneGraph, *node);
	EXPECT_FALSE(node->hasIKConstraint());

	// Set IK constraint on the node
	scenegraph::IKConstraint ik;
	ik.effectorNodeId = 42;
	ik.rollMin = -1.0f;
	ik.rollMax = 1.0f;
	ik.visible = false;
	ik.anchor = true;
	node->setIkConstraint(ik);
	_mementoHandler.markIKConstraintChange(_sceneGraph, *node);

	EXPECT_EQ(2, (int)_mementoHandler.stateSize());
	EXPECT_TRUE(_mementoHandler.canUndo());

	// Undo should restore to no IK constraint
	MementoState stateUndo = firstState(_mementoHandler.undo());
	EXPECT_EQ(MementoType::SceneNodeIKConstraint, stateUndo.type);
	EXPECT_FALSE(stateUndo.ikConstraint.hasValue());

	// Redo should restore the IK constraint
	MementoState stateRedo = firstState(_mementoHandler.redo());
	EXPECT_EQ(MementoType::SceneNodeIKConstraint, stateRedo.type);
	ASSERT_TRUE(stateRedo.ikConstraint.hasValue());
	EXPECT_EQ(42, stateRedo.ikConstraint.value()->effectorNodeId);
	EXPECT_FLOAT_EQ(-1.0f, stateRedo.ikConstraint.value()->rollMin);
	EXPECT_FLOAT_EQ(1.0f, stateRedo.ikConstraint.value()->rollMax);
	EXPECT_FALSE(stateRedo.ikConstraint.value()->visible);
	EXPECT_TRUE(stateRedo.ikConstraint.value()->anchor);
}

TEST_F(MementoHandlerTest, testMementoGroupModificationRename) {
	scenegraph::SceneGraphNode *node = _sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	_mementoHandler.markInitialNodeState(_sceneGraph, *node);
	ASSERT_EQ(1, (int)_mementoHandler.stateSize());
	{
		ScopedMementoGroup mementoGroup(_mementoHandler, "test");
		node->volume()->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		_mementoHandler.markModification(_sceneGraph, *node, voxel::Region(0, 0, 0, 0, 0, 0));
		node->setName("Name after");
		_mementoHandler.markNodeRenamed(_sceneGraph, *node);
	}
	EXPECT_EQ(2, (int)_mementoHandler.stateSize());
	memento::MementoState state = firstState(_mementoHandler.undo());
	EXPECT_EQ(state.name, "Node name");
	voxel::RawVolume volume(voxel::Region(0, 0));
	ASSERT_TRUE(state.data.toVolume(&volume, state.data, state.dataRegion()));
	EXPECT_EQ(voxel::VoxelType::Air, volume.voxel(0, 0, 0).getMaterial());
}

TEST_F(MementoHandlerTest, testSceneNodePaletteChange) {
	scenegraph::SceneGraphNode *node = _sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	_mementoHandler.markInitialNodeState(_sceneGraph, *node);
	EXPECT_EQ("built-in:nippon", node->palette().name());
	const int colorCount = node->palette().colorCount();
	palette::Palette palette;
	palette.commandAndConquer();
	node->setPalette(palette);
	_mementoHandler.markPaletteChange(_sceneGraph, *node);
	EXPECT_EQ(2, (int)_mementoHandler.stateSize());
	MementoState state = firstState(_mementoHandler.undo());
	ASSERT_EQ(state.palette.colorCount(), colorCount);
	EXPECT_EQ(state.palette.name(), "built-in:nippon");
}

TEST_F(MementoHandlerTest, testSceneNodeMove) {
	scenegraph::SceneGraphNode *node = _sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	const core::UUID oldParent = _sceneGraph.uuid(node->parent());
	_mementoHandler.markInitialNodeState(_sceneGraph, *node);
	const auto *parentNode = _sceneGraph.findNodeByUUID(_mementoHandler.stateGroup().states[0].parentUUID);
	ASSERT_TRUE(parentNode != nullptr);
	EXPECT_EQ(parentNode->id(), 0);

	int groupId;
	{
		scenegraph::SceneGraphNode group(scenegraph::SceneGraphNodeType::Group);
		group.setName("Group");
		groupId = _sceneGraph.emplace(core::move(group));
		ASSERT_NE(groupId, InvalidNodeId);
	}
	ASSERT_TRUE(_sceneGraph.changeParent(node->id(), groupId));
	_mementoHandler.markNodeMoved(_sceneGraph, *node);
	EXPECT_EQ(2, (int)_mementoHandler.stateSize());
	EXPECT_EQ(_mementoHandler.stateGroup().states[0].parentUUID, _sceneGraph.node(groupId).uuid());

	MementoState stateUndo = firstState(_mementoHandler.undo());
	EXPECT_EQ(oldParent, stateUndo.parentUUID);

	EXPECT_TRUE(_mementoHandler.canRedo());
	MementoState stateRedo = firstState(_mementoHandler.redo());
	EXPECT_EQ(_sceneGraph.node(groupId).uuid(), stateRedo.parentUUID);
}

TEST_F(MementoHandlerTest, testSceneNodeTransform) {
	scenegraph::SceneGraphNode *node = _sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	const glm::vec3 initial(1.0f, 2.0f, 3.0f);
	glm::vec3 mirrored;
	{
		scenegraph::SceneGraphTransform transform;
		transform.setLocalTranslation(initial);
		transform.update(_sceneGraph, *node, 0, false);
		node->setTransform(0, transform);
	}
	_mementoHandler.markInitialNodeState(_sceneGraph, *node);
	EXPECT_EQ(1, (int)_mementoHandler.stateSize());
	{
		scenegraph::SceneGraphTransform &transform = node->transform(0);
		transform.mirrorX();
		transform.update(_sceneGraph, *node, 0, false);
		mirrored = transform.localTranslation();
		node->setTransform(0, transform);
	}
	_mementoHandler.markNodeTransform(_sceneGraph, *node);
	EXPECT_EQ(2, (int)_mementoHandler.stateSize());

	MementoState stateUndo = firstState(_mementoHandler.undo());
	EXPECT_EQ(MementoType::SceneNodeKeyFrames, stateUndo.type);
	ASSERT_FALSE(stateUndo.keyFrames.empty());

	{
		_sceneGraph.setAllKeyFramesForNode(*node, stateUndo.keyFrames);
		const scenegraph::SceneGraphTransform &transform = node->transform(0);
		EXPECT_VEC_NEAR(transform.localTranslation(), initial, 0.0001f);
	}

	EXPECT_TRUE(_mementoHandler.canRedo());
	MementoState stateRedo = firstState(_mementoHandler.redo());
	EXPECT_EQ(MementoType::SceneNodeKeyFrames, stateRedo.type);
	ASSERT_FALSE(stateRedo.keyFrames.empty());

	{
		_sceneGraph.setAllKeyFramesForNode(*node, stateRedo.keyFrames);
		const scenegraph::SceneGraphTransform &transform = node->transform(0);
		EXPECT_VEC_NEAR(transform.localTranslation(), mirrored, 0.0001f);
	}
}

TEST_F(MementoHandlerTest, testAllAnimations) {
	scenegraph::SceneGraphNode *node = _sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	_mementoHandler.markInitialNodeState(_sceneGraph, *node);
	EXPECT_EQ(1, (int)_mementoHandler.stateSize());

	ASSERT_TRUE(_sceneGraph.addAnimation("foo"));
	EXPECT_EQ(2u, _sceneGraph.animations().size()) << _sceneGraph.animations();
	_mementoHandler.markAnimationAdded(_sceneGraph, "foo");
	EXPECT_EQ(2, (int)_mementoHandler.stateSize());

	ASSERT_TRUE(_sceneGraph.removeAnimation("foo"));
	EXPECT_EQ(1u, _sceneGraph.animations().size()) << _sceneGraph.animations();
	_mementoHandler.markAnimationRemoved(_sceneGraph, "foo");
	EXPECT_EQ(3, (int)_mementoHandler.stateSize());

	MementoState stateUndo = firstState(_mementoHandler.undo());
	EXPECT_EQ(MementoType::SceneGraphAnimation, stateUndo.type);
	ASSERT_TRUE(stateUndo.stringList.hasValue());
	ASSERT_EQ(2u, stateUndo.stringList.value()->size());
	_sceneGraph.setAnimations(*stateUndo.stringList.value());

	MementoState stateRedo = firstState(_mementoHandler.redo());
	EXPECT_EQ(MementoType::SceneGraphAnimation, stateRedo.type);
	ASSERT_TRUE(stateRedo.stringList.hasValue());
	ASSERT_EQ(1u, stateRedo.stringList.value()->size());
	_sceneGraph.setAnimations(*stateRedo.stringList.value());
}

TEST_F(MementoHandlerTest, testMarkModificationWithUndoRedoCycles) {
	// Create volumes with different sizes and specific voxel patterns
	core::SharedPtr<voxel::RawVolume> initialState = create(4);
	core::SharedPtr<voxel::RawVolume> state1 = create(4);
	core::SharedPtr<voxel::RawVolume> state2 = create(4);
	core::SharedPtr<voxel::RawVolume> state3 = create(4);

	// Mark initial state (all air voxels)
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Initial state", scenegraph::SceneGraphNodeType::Model,
							 initialState.get(), MementoType::Modification);
	EXPECT_EQ(1, (int)_mementoHandler.stateSize());
	EXPECT_EQ(0, (int)_mementoHandler.statePosition());

	// Set voxels and mark first modification
	state1->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	state1->setVoxel(1, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 2));
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "First modification", scenegraph::SceneGraphNodeType::Model,
							 state1.get(), MementoType::Modification);
	EXPECT_EQ(2, (int)_mementoHandler.stateSize());
	EXPECT_EQ(1, (int)_mementoHandler.statePosition());

	// Set more voxels and mark second modification (modify existing + add new)
	state2->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 3)); // Change color from 1 to 3
	state2->setVoxel(1, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 4)); // Change color from 2 to 4
	state2->setVoxel(2, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 5)); // New voxel
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Second modification", scenegraph::SceneGraphNodeType::Model,
							 state2.get(), MementoType::Modification);
	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
	EXPECT_EQ(2, (int)_mementoHandler.statePosition());

	// Set even more voxels and mark third modification
	state3->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 6)); // Change color from 3 to 6
	state3->setVoxel(1, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 7)); // Change color from 4 to 7
	state3->setVoxel(2, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 8)); // Change color from 5 to 8
	state3->setVoxel(3, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 9)); // New voxel
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Third modification", scenegraph::SceneGraphNodeType::Model,
							 state3.get(), MementoType::Modification);
	EXPECT_EQ(4, (int)_mementoHandler.stateSize());
	EXPECT_EQ(3, (int)_mementoHandler.statePosition());

	// Test undo/redo cycling multiple times with detailed voxel verification
	for (int cycle = 0; cycle < 3; ++cycle) {
		// Undo all the way back to the beginning
		{
			// From position 3 (third state) to position 2 (second state)
			const MementoState &undoState = firstState(_mementoHandler.undo());
			EXPECT_EQ(MementoType::Modification, undoState.type);
			EXPECT_EQ(2, (int)_mementoHandler.statePosition());

			// Verify we're back to second state voxel colors
			core::DynamicArray<core::Pair<glm::ivec3, uint8_t>> expectedSecond = {
				{glm::ivec3(0, 0, 0), 3}, {glm::ivec3(1, 0, 0), 4}, {glm::ivec3(2, 0, 0), 5}};
			core::DynamicArray<glm::ivec3> expectedAirSecond = {glm::ivec3(3, 0, 0)};
			verifyVoxelState(undoState, "undo to second state", expectedSecond, expectedAirSecond);
		}

		{
			// From position 2 (second state) to position 1 (first state)
			const MementoState &undoState = firstState(_mementoHandler.undo());
			EXPECT_EQ(MementoType::Modification, undoState.type);
			EXPECT_EQ(1, (int)_mementoHandler.statePosition());

			// Verify we're back to first state voxel colors
			core::DynamicArray<core::Pair<glm::ivec3, uint8_t>> expectedFirst = {{glm::ivec3(0, 0, 0), 1},
																				 {glm::ivec3(1, 0, 0), 2}};
			core::DynamicArray<glm::ivec3> expectedAirFirst = {glm::ivec3(2, 0, 0), glm::ivec3(3, 0, 0)};
			verifyVoxelState(undoState, "undo to first state", expectedFirst, expectedAirFirst);
		}

		{
			// From position 1 (first state) to position 0 (initial state)
			const MementoState &undoState = firstState(_mementoHandler.undo());
			EXPECT_EQ(MementoType::Modification, undoState.type);
			EXPECT_EQ(0, (int)_mementoHandler.statePosition());

			// Verify we're back to initial state (all air)
			core::DynamicArray<core::Pair<glm::ivec3, uint8_t>> expectedInitial = {};
			core::DynamicArray<glm::ivec3> expectedAirInitial = {glm::ivec3(0, 0, 0), glm::ivec3(1, 0, 0),
																 glm::ivec3(2, 0, 0), glm::ivec3(3, 0, 0)};
			verifyVoxelState(undoState, "undo to initial state", expectedInitial, expectedAirInitial);
		}

		// Redo everything back up with detailed verification
		{
			// From position 0 (initial state) to position 1 (first state)
			const MementoState &redoState = firstState(_mementoHandler.redo());
			EXPECT_EQ(MementoType::Modification, redoState.type);
			EXPECT_EQ(1, (int)_mementoHandler.statePosition());

			// Verify we're back to first state
			core::DynamicArray<core::Pair<glm::ivec3, uint8_t>> redoFirstExpected = {{glm::ivec3(0, 0, 0), 1},
																					 {glm::ivec3(1, 0, 0), 2}};
			core::DynamicArray<glm::ivec3> redoFirstAir = {glm::ivec3(2, 0, 0), glm::ivec3(3, 0, 0)};
			verifyVoxelState(redoState, "redo to first state", redoFirstExpected, redoFirstAir);
		}

		{
			// From position 1 (first state) to position 2 (second state)
			const MementoState &redoState = firstState(_mementoHandler.redo());
			EXPECT_EQ(MementoType::Modification, redoState.type);
			EXPECT_EQ(2, (int)_mementoHandler.statePosition());

			// Verify we're back to second state with modified colors
			core::DynamicArray<core::Pair<glm::ivec3, uint8_t>> redoSecondExpected = {
				{glm::ivec3(0, 0, 0), 3}, {glm::ivec3(1, 0, 0), 4}, {glm::ivec3(2, 0, 0), 5}};
			core::DynamicArray<glm::ivec3> redoSecondAir = {glm::ivec3(3, 0, 0)};
			verifyVoxelState(redoState, "redo to second state", redoSecondExpected, redoSecondAir);
		}

		{
			// From position 2 (second state) to position 3 (third state)
			const MementoState &redoState = firstState(_mementoHandler.redo());
			EXPECT_EQ(MementoType::Modification, redoState.type);
			EXPECT_EQ(3, (int)_mementoHandler.statePosition());

			// Verify we're back to third state with all final colors
			core::DynamicArray<core::Pair<glm::ivec3, uint8_t>> redoThirdExpected = {
				{glm::ivec3(0, 0, 0), 6}, {glm::ivec3(1, 0, 0), 7}, {glm::ivec3(2, 0, 0), 8}, {glm::ivec3(3, 0, 0), 9}};
			core::DynamicArray<glm::ivec3> redoThirdAir = {};
			verifyVoxelState(redoState, "redo to third state", redoThirdExpected, redoThirdAir);
		}
	}

	// Test partial undo/redo cycles with voxel verification
	_mementoHandler.undo(); // Go back from position 3 to position 2
	_mementoHandler.undo(); // Go back from position 2 to position 1
	EXPECT_EQ(1, (int)_mementoHandler.statePosition());

	// Redo just one step: from position 1 to position 2
	const MementoState &partialRedo = firstState(_mementoHandler.redo());
	EXPECT_EQ(MementoType::Modification, partialRedo.type);
	EXPECT_EQ(2, (int)_mementoHandler.statePosition());

	// Verify partial redo restored second state correctly
	core::DynamicArray<core::Pair<glm::ivec3, uint8_t>> partialRedoExpected = {
		{glm::ivec3(0, 0, 0), 3}, {glm::ivec3(1, 0, 0), 4}, {glm::ivec3(2, 0, 0), 5}};
	core::DynamicArray<glm::ivec3> partialRedoAir = {glm::ivec3(3, 0, 0)};
	verifyVoxelState(partialRedo, "partial redo to second state", partialRedoExpected, partialRedoAir);

	// Undo again: from position 2 to position 1
	const MementoState &partialUndo = firstState(_mementoHandler.undo());
	EXPECT_EQ(MementoType::Modification, partialUndo.type);
	EXPECT_EQ(1, (int)_mementoHandler.statePosition());

	// Verify partial undo restored first state correctly
	core::DynamicArray<core::Pair<glm::ivec3, uint8_t>> partialUndoExpected = {{glm::ivec3(0, 0, 0), 1},
																			   {glm::ivec3(1, 0, 0), 2}};
	core::DynamicArray<glm::ivec3> partialUndoAir = {glm::ivec3(2, 0, 0), glm::ivec3(3, 0, 0)};
	verifyVoxelState(partialUndo, "partial undo to first state", partialUndoExpected, partialUndoAir);
}

TEST_F(MementoHandlerTest, testNodeShiftWithModifiedRegionExceedingVolumeRegion) {
	scenegraph::SceneGraphNode *node = _sceneGraph.findNodeByUUID(core::UUID(1));
	ASSERT_NE(node, nullptr);
	node->volume()->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	_mementoHandler.markInitialSceneState(_sceneGraph);
	ASSERT_NE(node, nullptr);
	voxel::Region regionCopy = node->region();
	voxel::Region modifiedRegion = regionCopy;
	node->volume()->translate({1, 1, 1});
	modifiedRegion.accumulate(node->region());
	ASSERT_TRUE(_mementoHandler.markModification(_sceneGraph, *node, modifiedRegion));
	EXPECT_EQ(2, (int)_mementoHandler.stateSize());
	EXPECT_EQ(1, (int)_mementoHandler.statePosition());

	const MementoState &undoFirst = firstState(_mementoHandler.undo());
	ASSERT_TRUE(undoFirst.hasVolumeData());
	EXPECT_EQ(regionCopy.getWidthInVoxels(), undoFirst.volumeRegion().getWidthInVoxels());
	EXPECT_EQ(regionCopy.getLowerCorner(), undoFirst.volumeRegion().getLowerCorner());
	{
		voxel::RawVolume volume(undoFirst.dataRegion());
		ASSERT_TRUE(undoFirst.data.toVolume(&volume, undoFirst.data, undoFirst.dataRegion()))
			<< "Failed to extract volume";
		EXPECT_EQ(voxel::VoxelType::Generic, volume.voxel(0, 0, 0).getMaterial());
		EXPECT_EQ(voxel::VoxelType::Air, volume.voxel(1, 1, 1).getMaterial());
	}

	EXPECT_TRUE(_mementoHandler.canRedo());
	const MementoState &redoFirst = firstState(_mementoHandler.redo());
	ASSERT_TRUE(redoFirst.hasVolumeData());
	EXPECT_EQ(regionCopy.getLowerCorner() + 1, redoFirst.volumeRegion().getLowerCorner());
	{
		voxel::RawVolume volume(redoFirst.dataRegion());
		ASSERT_TRUE(redoFirst.data.toVolume(&volume, redoFirst.data, redoFirst.dataRegion()))
			<< "Failed to extract volume";
		EXPECT_EQ(voxel::VoxelType::Air, volume.voxel(0, 0, 0).getMaterial());
		EXPECT_EQ(voxel::VoxelType::Generic, volume.voxel(1, 1, 1).getMaterial());
	}
}

TEST_F(MementoHandlerTest, testMarkModificationWithRotatedVolume) {
	const int expectedVoxels = 4;
	{
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model, core::UUID(13));
		node.setVolume(new voxel::RawVolume(voxel::Region(-3, -2, -10, expectedVoxels - 1, 2, 1)), true);
		for (int i = 0; i < expectedVoxels; ++i) {
			node.volume()->setVoxel(i, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		}
		node.setName("Node rotate");
		_sceneGraph.emplace(core::move(node));
	}
	_mementoHandler.markInitialSceneState(_sceneGraph);
	scenegraph::SceneGraphNode *node = _sceneGraph.findNodeByUUID(core::UUID(13));
	ASSERT_NE(node, nullptr);
	const voxel::Region regionCopy = node->region();
	EXPECT_EQ(expectedVoxels, voxelutil::countVoxels(*node->volume()));
	voxel::RawVolume *newVolume = voxelutil::rotateAxis(node->volume(), math::Axis::Z);
	ASSERT_NE(newVolume, nullptr);
	voxel::Region modifiedRegion = newVolume->region();
	ASSERT_NE(newVolume->region(), regionCopy);
	modifiedRegion.accumulate(regionCopy);
	ASSERT_NE(modifiedRegion, regionCopy);
	node->setVolume(newVolume, true);
	ASSERT_TRUE(_mementoHandler.markModification(_sceneGraph, *node, modifiedRegion));
	EXPECT_EQ(expectedVoxels, voxelutil::countVoxels(*node->volume()));

	const MementoState &undoFirst = firstState(_mementoHandler.undo());
	ASSERT_TRUE(undoFirst.hasVolumeData());
	{
		ASSERT_EQ(undoFirst.volumeRegion(), regionCopy);
		ASSERT_EQ(undoFirst.dataRegion(), regionCopy);
		voxel::RawVolume volume(undoFirst.volumeRegion());
		ASSERT_TRUE(undoFirst.data.toVolume(&volume, undoFirst.data, undoFirst.dataRegion()))
			<< "Failed to extract volume";
		EXPECT_EQ(expectedVoxels, voxelutil::countVoxels(volume));

		for (int i = 0; i < expectedVoxels; ++i) {
			EXPECT_EQ(voxel::VoxelType::Generic, volume.voxel(i, 0, 0).getMaterial());
		}
	}

	ASSERT_TRUE(_mementoHandler.canRedo());
	const MementoState &redoFirst = firstState(_mementoHandler.redo());
	ASSERT_TRUE(redoFirst.hasVolumeData());
	{
		ASSERT_EQ(redoFirst.volumeRegion(), newVolume->region());
		ASSERT_EQ(redoFirst.dataRegion(), newVolume->region());
		voxel::RawVolume volume(redoFirst.volumeRegion());
		ASSERT_TRUE(redoFirst.data.toVolume(&volume, redoFirst.data, redoFirst.dataRegion()))
			<< "Failed to extract volume";
		EXPECT_EQ(expectedVoxels, voxelutil::countVoxels(volume));
	}
}

TEST_F(MementoHandlerTest, testCanUndoRedoWhileInGroup) {
	EXPECT_FALSE(_mementoHandler.canUndo());
	EXPECT_FALSE(_mementoHandler.canRedo());

	_mementoHandler.markUndo(0, 1, 0, "node", scenegraph::SceneGraphNodeType::Model, nullptr, MementoType::SceneNodeAdded);
	_mementoHandler.markUndo(0, 1, 0, "node", scenegraph::SceneGraphNodeType::Model, nullptr, MementoType::SceneNodeRenamed);

	EXPECT_EQ(2u, _mementoHandler.stateSize());
	EXPECT_EQ(1u, _mementoHandler.statePosition());

	EXPECT_TRUE(_mementoHandler.canUndo());
	EXPECT_FALSE(_mementoHandler.canRedo());

	_mementoHandler.beginGroup("test");
	EXPECT_FALSE(_mementoHandler.canUndo()) << "Should not be able to undo while in a group";
	EXPECT_FALSE(_mementoHandler.canRedo()) << "Should not be able to redo while in a group";
	_mementoHandler.endGroup();

	EXPECT_TRUE(_mementoHandler.canUndo());
}

// Test that undoing a mirror operation (which creates a combined dirty region spanning two
// disjoint areas) correctly preserves voxels between the mirrored edit regions.
// This is a regression test for https://github.com/vengi-voxel/vengi/issues/732
TEST_F(MementoHandlerTest, testMirrorUndoPreservesIntermediateVoxels) {
	// Create a 16-wide volume with some existing voxels in the middle (x=6..9)
	const voxel::Region volumeRegion(0, 0, 0, 15, 3, 3);
	core::SharedPtr<voxel::RawVolume> initialVolume = core::make_shared<voxel::RawVolume>(volumeRegion);
	// Place voxels in the middle of the volume that should NOT be affected by mirror undo
	for (int x = 6; x <= 9; ++x) {
		initialVolume->setVoxel(x, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 42));
	}

	// State 0: Initial state with existing voxels
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Initial", scenegraph::SceneGraphNodeType::Model,
							 initialVolume.get(), MementoType::Modification);

	// Simulate a mirror brush operation: place voxels at x=1..2 and mirrored at x=13..14
	// The dirty region in ModifierVolumeWrapper would span x=1..14 (bounding box of both sides)
	core::SharedPtr<voxel::RawVolume> afterMirror = core::make_shared<voxel::RawVolume>(initialVolume.get());
	afterMirror->setVoxel(1, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 10));
	afterMirror->setVoxel(2, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 10));
	afterMirror->setVoxel(13, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 10));
	afterMirror->setVoxel(14, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 10));

	// The combined dirty region spans x=1..14 (as the mirror wrapper accumulates both regions)
	const voxel::Region combinedMirrorRegion(1, 0, 0, 14, 0, 0);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Mirror edit", scenegraph::SceneGraphNodeType::Model,
							 afterMirror.get(), MementoType::Modification, combinedMirrorRegion);

	EXPECT_EQ(2, (int)_mementoHandler.stateSize());
	EXPECT_EQ(1, (int)_mementoHandler.statePosition());

	// Undo the mirror edit
	const MementoState &undoState = firstState(_mementoHandler.undo());
	ASSERT_TRUE(undoState.hasVolumeData());

	// Restore the volume to verify undo correctness
	voxel::RawVolume restored(volumeRegion);
	// First fill with initial data
	restored.copyInto(*initialVolume.get());
	// Then apply the undo state
	_mementoHandler.extractVolumeRegion(&restored, undoState);

	// The existing voxels at x=6..9 should still be present (NOT air!)
	for (int x = 6; x <= 9; ++x) {
		const voxel::Voxel v = restored.voxel(x, 0, 0);
		EXPECT_EQ(voxel::VoxelType::Generic, v.getMaterial())
			<< "Voxel at x=" << x << " should be preserved after mirror undo, but was reset to air";
		EXPECT_EQ(42, v.getColor())
			<< "Voxel at x=" << x << " should have color 42 after mirror undo";
	}

	// The mirror voxels at x=1,2,13,14 should be gone (back to air)
	for (int x : {1, 2, 13, 14}) {
		const voxel::Voxel v = restored.voxel(x, 0, 0);
		EXPECT_TRUE(voxel::isAir(v.getMaterial()))
			<< "Voxel at x=" << x << " should be air after mirror undo";
	}
}

// Test that undo works correctly when a partial edit region is followed by a mirror
// operation that creates a larger combined region overlapping the partial region
TEST_F(MementoHandlerTest, testMirrorUndoWithPriorPartialEdit) {
	const voxel::Region volumeRegion(0, 0, 0, 15, 3, 3);
	core::SharedPtr<voxel::RawVolume> initialVolume = core::make_shared<voxel::RawVolume>(volumeRegion);

	// State 0: Initial state (empty volume)
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Initial", scenegraph::SceneGraphNodeType::Model,
							 initialVolume.get(), MementoType::Modification);

	// State 1: Place voxels at x=6..8 (partial edit, only touches a small region)
	core::SharedPtr<voxel::RawVolume> afterEdit1 = core::make_shared<voxel::RawVolume>(initialVolume.get());
	for (int x = 6; x <= 8; ++x) {
		afterEdit1->setVoxel(x, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 20));
	}
	const voxel::Region edit1Region(6, 0, 0, 8, 0, 0);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Edit 1", scenegraph::SceneGraphNodeType::Model,
							 afterEdit1.get(), MementoType::Modification, edit1Region);

	// State 2: Mirror edit at x=1..2 and x=13..14 (combined region x=1..14 overlaps edit1)
	core::SharedPtr<voxel::RawVolume> afterMirror = core::make_shared<voxel::RawVolume>(afterEdit1.get());
	afterMirror->setVoxel(1, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 10));
	afterMirror->setVoxel(2, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 10));
	afterMirror->setVoxel(13, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 10));
	afterMirror->setVoxel(14, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 10));
	const voxel::Region combinedMirrorRegion(1, 0, 0, 14, 0, 0);
	_mementoHandler.markUndo(0, 0, InvalidNodeId, "Mirror edit", scenegraph::SceneGraphNodeType::Model,
							 afterMirror.get(), MementoType::Modification, combinedMirrorRegion);

	EXPECT_EQ(3, (int)_mementoHandler.stateSize());
	EXPECT_EQ(2, (int)_mementoHandler.statePosition());

	// Undo the mirror edit - should restore state 1 (voxels at x=6..8 present, mirror voxels gone)
	const MementoState &undoState = firstState(_mementoHandler.undo());
	ASSERT_TRUE(undoState.hasVolumeData());

	// Apply undo to a volume that starts in the afterMirror state
	voxel::RawVolume restored(volumeRegion);
	restored.copyInto(*afterMirror.get());
	_mementoHandler.extractVolumeRegion(&restored, undoState);

	// The voxels from edit 1 at x=6..8 should still be present
	for (int x = 6; x <= 8; ++x) {
		const voxel::Voxel v = restored.voxel(x, 0, 0);
		EXPECT_EQ(voxel::VoxelType::Generic, v.getMaterial())
			<< "Voxel at x=" << x << " from edit 1 should be preserved after mirror undo";
		EXPECT_EQ(20, v.getColor())
			<< "Voxel at x=" << x << " should have color 20 (from edit 1) after mirror undo";
	}

	// The mirror voxels should be gone
	for (int x : {1, 2, 13, 14}) {
		const voxel::Voxel v = restored.voxel(x, 0, 0);
		EXPECT_TRUE(voxel::isAir(v.getMaterial()))
			<< "Mirror voxel at x=" << x << " should be air after undo";
	}
}

} // namespace memento
