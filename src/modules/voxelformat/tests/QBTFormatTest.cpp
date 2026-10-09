/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "core/ConfigVar.h"
#include "core/ScopedPtr.h"
#include "core/Var.h"
#include "io/BufferedReadWriteStream.h"
#include "io/MemoryArchive.h"
#include "io/ZipWriteStream.h"
#include "voxel/RawVolume.h"
#include "util/VarUtil.h"
#include "voxelformat/tests/TestHelper.h"
#include "voxelformat/private/qubicle/QBTFormat.h"

namespace voxelformat {

class QBTFormatTest: public AbstractFormatTest {
protected:
	io::MemoryArchivePtr fixture(const glm::uvec3 &scale, const glm::vec3 &pivot,
								 const glm::vec3 &globalScale = glm::vec3(1.0f), bool compound = false) {
		io::BufferedReadWriteStream file;
		file.write("QB 2", 4);
		file.writeUInt8(1);
		file.writeUInt8(0);
		for (int axis = 0; axis < 3; ++axis) {
			file.writeFloat(globalScale[axis]);
		}
		file.write("COLORMAP", 8);
		file.writeUInt32(0);
		file.write("DATATREE", 8);
		writeMatrix(file, scale, pivot, glm::ivec3(10, -20, 30), compound);
		io::MemoryArchivePtr archive = io::openMemoryArchive();
		archive->add("synthetic.qbt", file.getBuffer(), file.size());
		return archive;
	}

	void writeMatrix(io::BufferedReadWriteStream &file, const glm::uvec3 &scale,
					 const glm::vec3 &pivot, const glm::ivec3 &position, bool compound) {
		file.writeUInt32(compound ? 2 : 0);
		const int64_t sizePos = file.pos();
		file.writeUInt32(0);
		file.writePascalStringUInt32LE(compound ? "parent" : "matrix");
		for (int axis = 0; axis < 3; ++axis) {
			file.writeInt32(position[axis]);
		}
		for (int axis = 0; axis < 3; ++axis) {
			file.writeUInt32(scale[axis]);
		}
		for (int axis = 0; axis < 3; ++axis) {
			file.writeFloat(pivot[axis]);
		}
		// A non-cubic grid catches accidental pivot-axis or unit mismatches.
		file.writeUInt32(2);
		file.writeUInt32(3);
		file.writeUInt32(4);
		io::BufferedReadWriteStream data;
		{
			io::ZipWriteStream zip(data);
			for (int i = 0; i < 24; ++i) {
				zip.writeUInt32(i == 0 ? 0xff0000ffu : 0u);
			}
		}
		file.writeUInt32(data.size());
		file.write(data.getBuffer(), data.size());
		if (compound) {
			file.writeUInt32(1);
			writeMatrix(file, glm::uvec3(3, 2, 1), glm::vec3(0.5f, 1.0f, 2.0f), glm::ivec3(4, 5, 6), false);
		}
		const int64_t endPos = file.pos();
		file.seek(sizePos);
		file.writeUInt32(endPos - sizePos - 4);
		file.seek(endPos);
	}
};

TEST_F(QBTFormatTest, testLoad) {
	testLoad("qubicle.qbt", 17);
}

TEST_F(QBTFormatTest, testLoadRGBSmall) {
	testRGBSmall("rgb_small.qbt");
}

TEST_F(QBTFormatTest, testLoadRGBSmallSaveLoad) {
	testRGBSmallSaveLoad("rgb_small.qbt");
}

TEST_F(QBTFormatTest, testSaveSingleVoxel) {
	QBTFormat f;
	testSaveSingleVoxel("qubicle-singlevoxelsavetest.qb", &f);
}

TEST_F(QBTFormatTest, testSaveSmallVoxel) {
	QBTFormat f;
	voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::SceneGraphModelsParent);
	testSaveLoadVoxel("qubicle-smallvolumesavetest.qbt", &f, 0, 1, flags);
}

TEST_F(QBTFormatTest, testSaveMultipleModels) {
	QBTFormat f;
	voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::SceneGraphModelsParent);
	testSaveMultipleModels("qubicle-multiplemodelsavetest.qbt", &f, flags);
}

TEST_F(QBTFormatTest, testSave) {
	QBTFormat f;
	testConvert("qubicle.qbt", f, "qubicle-savetest.qbt", f);
}

TEST_F(QBTFormatTest, testResaveMultipleModels) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "qubicle.qbt", 17);
	helper_saveSceneGraph(sceneGraph, "qubicle-savetest.qbt");
	sceneGraph.clear();
	testLoad(sceneGraph, "qubicle-savetest.qbt", 17);
}

TEST_F(QBTFormatTest, testLoadPivotAndScale) {
	QBTFormat format;
	const glm::vec3 globalScale(0.5f, 1.5f, 2.0f);
	const io::MemoryArchivePtr archive = fixture(glm::uvec3(2, 3, 4), glm::vec3(1, 1.5f, 2), globalScale);
	scenegraph::SceneGraph graph;
	ASSERT_TRUE(format.load("synthetic.qbt", archive, graph, testLoadCtx));
	const scenegraph::SceneGraphNode *node = graph.firstModelNode();
	ASSERT_NE(nullptr, node);
	EXPECT_VEC_NEAR(node->pivot(), glm::vec3(0.5f), 0.0001f);
	EXPECT_VEC_NEAR(node->transform(0).localScale(), glm::vec3(2, 3, 4), 0.0001f);
	EXPECT_VEC_NEAR(graph.root().transform(0).localScale(), globalScale, 0.0001f);
	// Pivot metadata must not move the grid origin or its individual cells.
	const glm::mat4 world = graph.worldMatrix(*node, 0);
	EXPECT_VEC_NEAR(glm::vec3(world * glm::vec4(0, 0, 0, 1)), glm::vec3(5, -30, 60), 0.0001f);
	EXPECT_VEC_NEAR(glm::vec3(world * glm::vec4(1, 2, 3, 1)), glm::vec3(6, -21, 84), 0.0001f);

	ASSERT_TRUE(format.save(graph, "roundtrip.qbt", archive, testSaveCtx));
	scenegraph::SceneGraph loaded;
	ASSERT_TRUE(format.load("roundtrip.qbt", archive, loaded, testLoadCtx));
	ASSERT_NE(nullptr, loaded.firstModelNode());
	EXPECT_VEC_NEAR(loaded.firstModelNode()->pivot(), node->pivot(), 0.0001f);
	EXPECT_VEC_NEAR(loaded.firstModelNode()->transform(0).localScale(), glm::vec3(2, 3, 4), 0.0001f);
	EXPECT_VEC_NEAR(loaded.root().transform(0).localScale(), globalScale, 0.0001f);
	const glm::mat4 reloadedWorld = loaded.worldMatrix(*loaded.firstModelNode(), 0);
	EXPECT_VEC_NEAR(glm::vec3(reloadedWorld * glm::vec4(1, 2, 3, 1)), glm::vec3(6, -21, 84), 0.0001f);
}

TEST_F(QBTFormatTest, testCompoundRelativePlacement) {
	util::ScopedVarChange mergeCompounds(cfg::VoxformatQBTMergeCompounds, "false");
	const io::MemoryArchivePtr archive = fixture(glm::uvec3(2, 3, 4), glm::vec3(1, 1.5f, 2), glm::vec3(1), true);
	QBTFormat format;
	scenegraph::SceneGraph graph;
	ASSERT_TRUE(format.load("synthetic.qbt", archive, graph, testLoadCtx));
	ASSERT_EQ(2u, graph.size(scenegraph::SceneGraphNodeType::Model));
	const scenegraph::SceneGraphNode *parent = graph.firstModelNode();
	ASSERT_NE(nullptr, parent);
	ASSERT_EQ(1u, parent->children().size());
	const scenegraph::SceneGraphNode *child = graph.findNodeByUUID(parent->children().front());
	ASSERT_NE(nullptr, child);
	EXPECT_EQ(parent, graph.parentNode(*child));
	const glm::mat4 world = graph.worldMatrix(*child, 0);
	EXPECT_VEC_NEAR(glm::vec3(world * glm::vec4(0, 0, 0, 1)), glm::vec3(18, -5, 54), 0.0001f);
	EXPECT_VEC_NEAR(glm::vec3(world * glm::vec4(1, 2, 3, 1)), glm::vec3(24, 7, 66), 0.0001f);
	ASSERT_TRUE(format.save(graph, "compound.qbt", archive, testSaveCtx));
	scenegraph::SceneGraph loaded;
	ASSERT_TRUE(format.load("compound.qbt", archive, loaded, testLoadCtx));
	voxel::sceneGraphComparator(graph, loaded, voxel::ValidateFlags::OccupiedExact | voxel::ValidateFlags::Color);

	util::ScopedVarChange mergedCompounds(cfg::VoxformatQBTMergeCompounds, "true");
	scenegraph::SceneGraph merged;
	ASSERT_TRUE(format.load("synthetic.qbt", archive, merged, testLoadCtx));
	ASSERT_EQ(1u, merged.size(scenegraph::SceneGraphNodeType::Model));
	const scenegraph::SceneGraphNode *mergedNode = merged.firstModelNode();
	ASSERT_NE(nullptr, mergedNode);
	EXPECT_VEC_NEAR(glm::vec3(merged.worldMatrix(*mergedNode, 0) * glm::vec4(0, 0, 0, 1)), glm::vec3(10, -20, 30), 0.0001f);
}

TEST_F(QBTFormatTest, testRejectsInvalidScalesAndPivot) {
	QBTFormat format;
	const io::MemoryArchivePtr invalidLocal = fixture(glm::uvec3(1, 0, 1), glm::vec3(0));
	scenegraph::SceneGraph graph;
	EXPECT_FALSE(format.load("synthetic.qbt", invalidLocal, graph, testLoadCtx));
	const io::MemoryArchivePtr invalidGlobal = fixture(glm::uvec3(1), glm::vec3(0), glm::vec3(0));
	graph.clear();
	EXPECT_FALSE(format.load("synthetic.qbt", invalidGlobal, graph, testLoadCtx));
	const io::MemoryArchivePtr invalidPivot = fixture(glm::uvec3(1), glm::vec3(nanf("")));
	graph.clear();
	EXPECT_FALSE(format.load("synthetic.qbt", invalidPivot, graph, testLoadCtx));
}

TEST_F(QBTFormatTest, testRejectsUnrepresentableExportScale) {
	QBTFormat format;
	const io::MemoryArchivePtr archive = fixture(glm::uvec3(1), glm::vec3(0));
	scenegraph::SceneGraph graph;
	ASSERT_TRUE(format.load("synthetic.qbt", archive, graph, testLoadCtx));
	scenegraph::SceneGraphNode *node = graph.firstModelNode();
	ASSERT_NE(nullptr, node);
	const glm::vec3 scales[] = {glm::vec3(1.5f, 1, 1), glm::vec3(-1, 1, 1), glm::vec3(0, 1, 1)};
	for (const glm::vec3 &scale : scales) {
		node->transform(0).setLocalScale(scale);
		EXPECT_FALSE(format.save(graph, "invalid-scale.qbt", archive, testSaveCtx));
	}
	node->transform(0).setLocalScale(glm::vec3(1));
	graph.node(graph.root().id()).transform(0).setLocalScale(glm::vec3(0));
	EXPECT_FALSE(format.save(graph, "invalid-global-scale.qbt", archive, testSaveCtx));
}

TEST_F(QBTFormatTest, testExportShiftedVolumeAndGroupScale) {
	QBTFormat format;
	io::MemoryArchivePtr archive = io::openMemoryArchive();
	scenegraph::SceneGraph graph;
	scenegraph::SceneGraphNode group(scenegraph::SceneGraphNodeType::Group);
	group.transform(0).setLocalTranslation(glm::vec3(10, 20, 30));
	group.transform(0).setLocalScale(glm::vec3(2, 3, 4));
	const int parent = graph.emplace(core::move(group));
	ASSERT_NE(InvalidNodeId, parent);
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	voxel::RawVolume *volume = new voxel::RawVolume(voxel::Region(3, 4, 5, 4, 6, 8));
	palette::Palette palette;
	palette.setColor(0, color::RGBA(255, 0, 0, 255));
	palette.setSize(1);
	volume->setVoxel(3, 4, 5, voxel::createVoxel(palette, 0));
	node.setVolume(volume);
	node.setPalette(palette);
	node.setPivot(glm::vec3(0.5f));
	node.transform(0).setLocalTranslation(glm::vec3(7, 8.5f, 9));
	node.transform(0).setLocalScale(glm::vec3(2, 1, 3));
	const int id = graph.emplace(core::move(node), parent);
	ASSERT_NE(InvalidNodeId, id);
	graph.updateTransforms();
	const glm::mat4 world = graph.worldMatrix(graph.node(id), 0);
	const glm::vec3 expected = glm::vec3(world * glm::vec4(3, 4, 5, 1));
	ASSERT_TRUE(format.save(graph, "shifted.qbt", archive, testSaveCtx));
	scenegraph::SceneGraph loaded;
	ASSERT_TRUE(format.load("shifted.qbt", archive, loaded, testLoadCtx));
	const scenegraph::SceneGraphNode *model = loaded.firstModelNode();
	ASSERT_NE(nullptr, model);
	EXPECT_VEC_NEAR(model->transform(0).localScale(), glm::vec3(4, 3, 12), 0.0001f);
	EXPECT_VEC_NEAR(glm::vec3(loaded.worldMatrix(*model, 0) * glm::vec4(0, 0, 0, 1)), expected, 0.0001f);
}

}
