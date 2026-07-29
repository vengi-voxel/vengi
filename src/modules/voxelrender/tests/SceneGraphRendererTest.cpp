/**
 * @file
 */

#include "voxelrender/SceneGraphRenderer.h"
#include "core/SharedPtr.h"
#include "core/TimeProvider.h"
#include "core/UUID.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphUtil.h"
#include "video/tests/AbstractGLTest.h"
#include "voxel/MeshState.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelrender/RenderContext.h"

namespace voxelrender {

class TestableSceneGraphRenderer : public SceneGraphRenderer {
public:
	TestableSceneGraphRenderer(const core::TimeProviderPtr &timeProvider) : SceneGraphRenderer(timeProvider) {
	}
	using SceneGraphRenderer::prepare;
};

class SceneGraphRendererTest : public video::AbstractGLTest {
protected:
	voxel::MeshStatePtr _meshState;
	TestableSceneGraphRenderer _renderer;

	SceneGraphRendererTest() : _renderer(core::make_shared<core::TimeProvider>()) {
	}

	void SetUp() override {
		Super::SetUp();
		if (IsSkipped()) {
			return;
		}
		video::ShaderVarState state;
		setShaderVars(state);
		_meshState = core::make_shared<voxel::MeshState>();
		_meshState->construct();
		ASSERT_TRUE(_meshState->init());
		_renderer.construct();
		ASSERT_TRUE(_renderer.init(false));
	}

	void TearDown() override {
		if (!IsSkipped()) {
			_renderer.shutdown();
			if (_meshState) {
				(void)_meshState->shutdown();
				_meshState = voxel::MeshStatePtr();
			}
		}
		Super::TearDown();
	}

private:
	using Super = video::AbstractGLTest;
};

TEST_F(SceneGraphRendererTest, testUuidVolumeIdxStableAndReused) {
	const core::UUID uuidA = core::UUID::generate();
	const core::UUID uuidB = core::UUID::generate();

	const int idxA = _renderer.getOrAssignVolumeIdx(uuidA);
	EXPECT_GE(idxA, 0);
	EXPECT_EQ(idxA, _renderer.getOrAssignVolumeIdx(uuidA));
	EXPECT_EQ(idxA, _renderer.getVolumeIdx(uuidA));

	const int idxB = _renderer.getOrAssignVolumeIdx(uuidB);
	EXPECT_GE(idxB, 0);
	EXPECT_NE(idxA, idxB);

	_renderer.nodeRemove(_meshState, uuidA);
	EXPECT_EQ(-1, _renderer.getVolumeIdx(uuidA));
	EXPECT_EQ(idxB, _renderer.getVolumeIdx(uuidB));
	EXPECT_EQ(1, _renderer.freeVolumeIndexCount());

	const core::UUID uuidC = core::UUID::generate();
	const int idxC = _renderer.getOrAssignVolumeIdx(uuidC);
	EXPECT_EQ(idxA, idxC) << "freed compact volume index should be reused";
	EXPECT_EQ(0, _renderer.freeVolumeIndexCount());
}

TEST_F(SceneGraphRendererTest, testNodeRemoveWithoutVolumeIsSafe) {
	const core::UUID uuid = core::UUID::generate();
	const int idx = _renderer.getOrAssignVolumeIdx(uuid);
	EXPECT_GE(idx, 0);
	// Never called setVolume / scheduleRegionExtraction - deleteMesh path must tolerate unset buffers.
	_renderer.nodeRemove(_meshState, uuid);
	EXPECT_EQ(-1, _renderer.getVolumeIdx(uuid));
	_renderer.nodeRemove(_meshState, uuid);
}

TEST_F(SceneGraphRendererTest, testClearResetsUuidMapping) {
	const core::UUID uuid = core::UUID::generate();
	EXPECT_GE(_renderer.getOrAssignVolumeIdx(uuid), 0);
	_renderer.clear(_meshState);
	EXPECT_EQ(-1, _renderer.getVolumeIdx(uuid));
	EXPECT_EQ(0, _renderer.mappedNodeCount());
	EXPECT_EQ(0, _renderer.nextVolumeIdx());
}

TEST_F(SceneGraphRendererTest, testModelReferenceLinksToTargetMeshSlot) {
	scenegraph::SceneGraph sceneGraph;
	int modelId = InvalidNodeId;
	{
		scenegraph::SceneGraphNode model(scenegraph::SceneGraphNodeType::Model);
		model.setName("model");
		voxel::RawVolume *v = new voxel::RawVolume(voxel::Region(0, 0));
		v->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		model.setVolume(v);
		modelId = sceneGraph.emplace(core::move(model));
		ASSERT_NE(InvalidNodeId, modelId);
	}
	const int refId = scenegraph::createNodeReference(sceneGraph, sceneGraph.node(modelId));
	ASSERT_NE(InvalidNodeId, refId);
	sceneGraph.updateTransforms();

	RenderContext renderContext;
	renderContext.sceneGraph = &sceneGraph;
	renderContext.renderMode = RenderMode::Scene;
	_renderer.prepare(_meshState, renderContext);

	const scenegraph::SceneGraphNode &model = sceneGraph.node(modelId);
	const scenegraph::SceneGraphNode &ref = sceneGraph.node(refId);
	const int modelIdx = _renderer.getVolumeIdx(model);
	const int refIdx = _renderer.getVolumeIdx(ref);
	ASSERT_GE(modelIdx, 0);
	ASSERT_GE(refIdx, 0);
	EXPECT_NE(modelIdx, refIdx);
	EXPECT_EQ(modelIdx, _meshState->reference(refIdx));
	EXPECT_EQ(-1, _meshState->reference(modelIdx));
}

} // namespace voxelrender
