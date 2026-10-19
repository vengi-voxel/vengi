/**
 * @file
 */

#include "AbstractSceneManagerTest.h"
#include "core/GLM.h"
#include "core/collection/DynamicSet.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphTransform.h"
#include "voxedit-util/3dprint/Regrid.h"
#include "voxedit-util/SceneManager.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"

namespace voxedit {

class RegridTest : public AbstractSceneManagerTest {
private:
	using Super = AbstractSceneManagerTest;

protected:
	int addSourceNode(const core::String &name, const voxel::Region &region, const glm::vec3 &worldTranslation,
					  const core::DynamicArray<glm::ivec3> &solidPositions, uint8_t paletteIdx) {
		scenegraph::SceneGraph &graph = _sceneMgr->sceneGraph();
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.createVolume(region);
		voxel::RawVolume *rv = node.volume();
		for (const glm::ivec3 &p : solidPositions) {
			rv->setVoxel(p.x, p.y, p.z, voxel::createVoxel(voxel::VoxelType::Generic, paletteIdx));
		}
		node.setName(name);
		scenegraph::SceneGraphTransform transform;
		transform.setWorldTranslation(worldTranslation);
		node.keyFrame(0).setTransform(transform);
		const int nodeId = graph.emplace(core::move(node));
		graph.updateTransforms();
		return nodeId;
	}

	int countRegridNodes() {
		scenegraph::SceneGraph &graph = _sceneMgr->sceneGraph();
		int n = 0;
		for (auto iter = graph.beginModel(); iter != graph.end(); ++iter) {
			if ((*iter).name().contains("regrid_")) {
				++n;
			}
		}
		return n;
	}

	// Returns the first regrid node whose world-space lower corner (keyframe translation
	// plus volume region lower) falls inside the cell-sized box at world origin
	// (cx*cellSize, cy*cellSize, cz*cellSize).
	scenegraph::SceneGraphNode *findRegridNode(int cx, int cy, int cz, int cellSize = 128) {
		const glm::ivec3 ox(cx * cellSize, cy * cellSize, cz * cellSize);
		scenegraph::SceneGraph &graph = _sceneMgr->sceneGraph();
		for (auto iter = graph.beginModel(); iter != graph.end(); ++iter) {
			scenegraph::SceneGraphNode &node = *iter;
			if (!node.name().contains("regrid_")) {
				continue;
			}
			const glm::vec3 tr = node.keyFrame(0).transform().worldTranslation();
			const glm::ivec3 rlo = node.volume()->region().getLowerCorner();
			const glm::ivec3 worldLo((int)tr.x + rlo.x, (int)tr.y + rlo.y, (int)tr.z + rlo.z);
			if (worldLo.x < ox.x || worldLo.x >= ox.x + cellSize) {
				continue;
			}
			if (worldLo.y < ox.y || worldLo.y >= ox.y + cellSize) {
				continue;
			}
			if (worldLo.z < ox.z || worldLo.z >= ox.z + cellSize) {
				continue;
			}
			return &node;
		}
		return nullptr;
	}

	// Returns whether the regrid node has a solid voxel at the given world position.
	bool regridNodeHasWorldVoxel(scenegraph::SceneGraphNode *node, const glm::ivec3 &worldPos) {
		const glm::vec3 tr = node->keyFrame(0).transform().worldTranslation();
		const glm::ivec3 local(worldPos.x - (int)tr.x, worldPos.y - (int)tr.y, worldPos.z - (int)tr.z);
		const voxel::Region &r = node->volume()->region();
		if (!r.containsPoint(local)) {
			return false;
		}
		return !voxel::isAir(node->volume()->voxel(local).getMaterial());
	}
};

TEST_F(RegridTest, testSingleNodeAtOriginFitsOneCell) {
	ASSERT_TRUE(_sceneMgr->newScene(true, "regrid", voxel::Region(0, 0, 0, 0, 0, 0)));
	const int srcId = addSourceNode("source", voxel::Region(0, 0, 0, 3, 3, 3), glm::vec3(0),
									{{0, 0, 0}, {2, 2, 2}}, 1);

	printing::runRegrid(_sceneMgr.get(), 128);

	EXPECT_FALSE(_sceneMgr->sceneGraph().hasNode(srcId));
	EXPECT_EQ(1, countRegridNodes());
	scenegraph::SceneGraphNode *cell = findRegridNode(0, 0, 0);
	ASSERT_NE(nullptr, cell);
	EXPECT_TRUE(regridNodeHasWorldVoxel(cell, glm::ivec3(0, 0, 0)));
	EXPECT_TRUE(regridNodeHasWorldVoxel(cell, glm::ivec3(2, 2, 2)));
	EXPECT_FALSE(regridNodeHasWorldVoxel(cell, glm::ivec3(1, 1, 1)));
}

TEST_F(RegridTest, testTranslationPlacesVoxelInCorrectCell) {
	ASSERT_TRUE(_sceneMgr->newScene(true, "regrid", voxel::Region(0, 0, 0, 0, 0, 0)));
	// Source local region is (0..3)^3, translated to world +200 along X.
	// Cell 128: voxels end up in world X in [200..203] -> cell X = 1.
	const int srcId = addSourceNode("source", voxel::Region(0, 0, 0, 3, 3, 3), glm::vec3(200, 0, 0),
									{{0, 0, 0}, {3, 0, 0}}, 1);

	printing::runRegrid(_sceneMgr.get(), 128);

	EXPECT_FALSE(_sceneMgr->sceneGraph().hasNode(srcId));
	scenegraph::SceneGraphNode *cell = findRegridNode(1, 0, 0);
	ASSERT_NE(nullptr, cell) << "expected regrid node at cell (1,0,0)";
	EXPECT_TRUE(regridNodeHasWorldVoxel(cell, glm::ivec3(200, 0, 0)));
	EXPECT_TRUE(regridNodeHasWorldVoxel(cell, glm::ivec3(203, 0, 0)));
}

TEST_F(RegridTest, testNodeSpanningCellBoundaryEmitsTwoCells) {
	ASSERT_TRUE(_sceneMgr->newScene(true, "regrid", voxel::Region(0, 0, 0, 0, 0, 0)));
	// Region 0..130 at world origin with cellSize=128 straddles cells 0 and 1 on X.
	const int srcId = addSourceNode("source", voxel::Region(0, 0, 0, 130, 0, 0), glm::vec3(0),
									{{0, 0, 0}, {130, 0, 0}}, 1);

	printing::runRegrid(_sceneMgr.get(), 128);

	EXPECT_FALSE(_sceneMgr->sceneGraph().hasNode(srcId));
	scenegraph::SceneGraphNode *cell0 = findRegridNode(0, 0, 0);
	scenegraph::SceneGraphNode *cell1 = findRegridNode(1, 0, 0);
	ASSERT_NE(nullptr, cell0);
	ASSERT_NE(nullptr, cell1);
	EXPECT_TRUE(regridNodeHasWorldVoxel(cell0, glm::ivec3(0, 0, 0)));
	EXPECT_TRUE(regridNodeHasWorldVoxel(cell1, glm::ivec3(130, 0, 0)));
}

TEST_F(RegridTest, testOverlappingSourcesLastWins) {
	ASSERT_TRUE(_sceneMgr->newScene(true, "regrid", voxel::Region(0, 0, 0, 0, 0, 0)));
	// Two sources at the same world position; last-write wins.
	addSourceNode("a", voxel::Region(0, 0, 0, 0, 0, 0), glm::vec3(0), {{0, 0, 0}}, 1);
	addSourceNode("b", voxel::Region(0, 0, 0, 0, 0, 0), glm::vec3(0), {{0, 0, 0}}, 7);

	printing::runRegrid(_sceneMgr.get(), 128);

	scenegraph::SceneGraphNode *cell = findRegridNode(0, 0, 0);
	ASSERT_NE(nullptr, cell);
	EXPECT_TRUE(regridNodeHasWorldVoxel(cell, glm::ivec3(0, 0, 0)));
}

TEST_F(RegridTest, testEmptyCellsAreNotEmitted) {
	ASSERT_TRUE(_sceneMgr->newScene(true, "regrid", voxel::Region(0, 0, 0, 0, 0, 0)));
	// A source whose region spans two cells but only cell 0 actually has a solid voxel.
	addSourceNode("sparse", voxel::Region(0, 0, 0, 130, 0, 0), glm::vec3(0), {{0, 0, 0}}, 1);

	printing::runRegrid(_sceneMgr.get(), 128);

	EXPECT_NE(nullptr, findRegridNode(0, 0, 0));
	EXPECT_EQ(nullptr, findRegridNode(1, 0, 0)) << "empty cell must not produce a regrid node";
}

// Collect all solid voxel world positions across all regrid cells.
static core::DynamicSet<glm::ivec3, 1031, glm::hash<glm::ivec3>> collectRegridWorldVoxels(
		scenegraph::SceneGraph &graph) {
	core::DynamicSet<glm::ivec3, 1031, glm::hash<glm::ivec3>> result;
	for (auto iter = graph.beginModel(); iter != graph.end(); ++iter) {
		scenegraph::SceneGraphNode &node = *iter;
		if (!node.name().contains("regrid_")) {
			continue;
		}
		const glm::vec3 tr = node.keyFrame(0).transform().worldTranslation();
		const voxel::RawVolume *vol = node.volume();
		const voxel::Region &r = vol->region();
		const glm::ivec3 lo = r.getLowerCorner();
		const glm::ivec3 hi = r.getUpperCorner();
		for (int y = lo.y; y <= hi.y; ++y) {
			for (int z = lo.z; z <= hi.z; ++z) {
				for (int x = lo.x; x <= hi.x; ++x) {
					if (!voxel::isAir(vol->voxel(x, y, z).getMaterial())) {
						result.insert(glm::ivec3((int)tr.x + x, (int)tr.y + y, (int)tr.z + z));
					}
				}
			}
		}
	}
	return result;
}

// Regression: non-zero region lower corner + large world translation must preserve world positions.
TEST_F(RegridTest, testNonZeroRegionLowerCornerPreservesWorldPositions) {
	ASSERT_TRUE(_sceneMgr->newScene(true, "regrid", voxel::Region(0, 0, 0, 0, 0, 0)));
	const glm::vec3 worldTranslation(325.0f, 178.0f, -1174.0f);
	const voxel::Region srcRegion(54, 0, 0, 98, 127, 127);
	const core::DynamicArray<glm::ivec3> srcVoxels = {{54, 0, 0}, {98, 127, 127}, {70, 64, 32}};
	addSourceNode("source", srcRegion, worldTranslation, srcVoxels, 1);

	core::DynamicSet<glm::ivec3, 1031, glm::hash<glm::ivec3>> expected;
	for (const glm::ivec3 &lp : srcVoxels) {
		expected.insert(glm::ivec3((int)worldTranslation.x + lp.x,
								   (int)worldTranslation.y + lp.y,
								   (int)worldTranslation.z + lp.z));
	}

	printing::runRegrid(_sceneMgr.get(), 128);

	const auto actual = collectRegridWorldVoxels(_sceneMgr->sceneGraph());
	EXPECT_EQ(expected.size(), actual.size());
	for (const auto &kv : expected) {
		const glm::ivec3 &wp = kv->key;
		EXPECT_TRUE(actual.has(wp)) << "missing world voxel (" << wp.x << "," << wp.y << "," << wp.z << ")";
	}
}

// Regression: negative region lower corner (real nodes have Y starting at -48).
TEST_F(RegridTest, testNegativeRegionLowerCornerPreservesWorldPositions) {
	ASSERT_TRUE(_sceneMgr->newScene(true, "regrid", voxel::Region(0, 0, 0, 0, 0, 0)));
	const glm::vec3 worldTranslation(-443.0f, -1742.0f, 1258.0f);
	const voxel::Region srcRegion(0, -48, 0, 127, 127, 22);
	const core::DynamicArray<glm::ivec3> srcVoxels = {{0, -48, 0}, {127, 127, 22}, {63, 0, 11}};
	addSourceNode("source", srcRegion, worldTranslation, srcVoxels, 1);

	core::DynamicSet<glm::ivec3, 1031, glm::hash<glm::ivec3>> expected;
	for (const glm::ivec3 &lp : srcVoxels) {
		expected.insert(glm::ivec3((int)worldTranslation.x + lp.x,
								   (int)worldTranslation.y + lp.y,
								   (int)worldTranslation.z + lp.z));
	}

	printing::runRegrid(_sceneMgr.get(), 128);

	const auto actual = collectRegridWorldVoxels(_sceneMgr->sceneGraph());
	EXPECT_EQ(expected.size(), actual.size());
	for (const auto &kv : expected) {
		const glm::ivec3 &wp = kv->key;
		EXPECT_TRUE(actual.has(wp)) << "missing world voxel (" << wp.x << "," << wp.y << "," << wp.z << ")";
	}
}

TEST_F(RegridTest, testMultipleSourcesAllDeleted) {
	ASSERT_TRUE(_sceneMgr->newScene(true, "regrid", voxel::Region(0, 0, 0, 0, 0, 0)));
	const int a = addSourceNode("a", voxel::Region(0, 0, 0, 3, 3, 3), glm::vec3(0), {{1, 1, 1}}, 1);
	const int b = addSourceNode("b", voxel::Region(0, 0, 0, 3, 3, 3), glm::vec3(300, 0, 0), {{1, 1, 1}}, 1);
	const int c = addSourceNode("c", voxel::Region(0, 0, 0, 3, 3, 3), glm::vec3(0, 300, 0), {{1, 1, 1}}, 1);

	printing::runRegrid(_sceneMgr.get(), 128);

	EXPECT_FALSE(_sceneMgr->sceneGraph().hasNode(a));
	EXPECT_FALSE(_sceneMgr->sceneGraph().hasNode(b));
	EXPECT_FALSE(_sceneMgr->sceneGraph().hasNode(c));
	EXPECT_EQ(3, countRegridNodes());
	EXPECT_NE(nullptr, findRegridNode(0, 0, 0));
	EXPECT_NE(nullptr, findRegridNode(2, 0, 0));
	EXPECT_NE(nullptr, findRegridNode(0, 2, 0));
}

} // namespace voxedit
