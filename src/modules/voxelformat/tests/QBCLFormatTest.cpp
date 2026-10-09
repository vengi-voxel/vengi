/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "voxelformat/private/qubicle/QBCLFormat.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraph.h"
#include "io/MemoryArchive.h"
#include "io/BufferedReadWriteStream.h"
#include "io/ZipWriteStream.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

namespace voxelformat {

class QBCLFormatTest: public AbstractFormatTest {
};

TEST_F(QBCLFormatTest, testLoad) {
	testLoad("qubicle.qbcl", 30);
}

TEST_F(QBCLFormatTest, testSaveSmallVoxel) {
	QBCLFormat f;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Palette | voxel::ValidateFlags::SceneGraphModelsParent);
	testSaveLoadVoxel("qubicle-smallvolumesavetest.qbcl", &f, 0, 1, flags);
}

TEST_F(QBCLFormatTest, testLoadRGB) {
	testRGB("rgb.qbcl");
}

TEST_F(QBCLFormatTest, testLoadRGBSmall) {
	testRGBSmall("rgb_small.qbcl");
}

TEST_F(QBCLFormatTest, testLoadRGBSmallSaveLoad) {
	testRGBSmallSaveLoad("rgb_small.qbcl");
}

TEST_F(QBCLFormatTest, testLoadScreenshot) {
	testLoadScreenshot("chr_knight.qbcl", 100, 100, color::RGBA(147, 53, 53), 59, 1);
}

TEST_F(QBCLFormatTest, testLoadCrabby) {
	scenegraph::SceneGraph qbclsceneGraph;
	testLoad(qbclsceneGraph, "crabby.qbcl", 2);
	scenegraph::SceneGraph voxsceneGraph;
	testLoad(voxsceneGraph, "crabby.vox", 2);
	// QBCL preserves the author's pivot; the VOX export bakes it into placement.
	const voxel::ValidateFlags flags =
		(voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Palette | voxel::ValidateFlags::SceneGraphModelsParent |
									 voxel::ValidateFlags::Transform)) |
		voxel::ValidateFlags::OccupiedExact;
	voxel::sceneGraphComparator(qbclsceneGraph, voxsceneGraph, flags);
}

TEST_F(QBCLFormatTest, testPivotAndOpaqueModelDataRoundTrip) {
	scenegraph::SceneGraph graph;
	scenegraph::SceneGraphNode &root = graph.node(graph.root().id());
	root.setName("root metadata");
	root.setProperty("qbcl_model_data", "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIj");
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setName("pivoted");
	node.setPivot(glm::vec3(0.25f, 0.5f, 0.75f));
	voxel::RawVolume *volume = new voxel::RawVolume(voxel::Region(0, 1));
	volume->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	node.setVolume(volume);
	palette::Palette palette;
	palette.setColor(0, color::RGBA(255, 0, 0, 255));
	palette.setSize(1);
	node.setPalette(palette);
	scenegraph::SceneGraphTransform transform;
	transform.setLocalTranslation(glm::vec3(3.5f, 5.0f, 6.5f));
	node.setTransform(0, transform);
	ASSERT_NE(InvalidNodeId, graph.emplace(core::move(node)));
	graph.updateTransforms();
	io::MemoryArchivePtr archive = io::openMemoryArchive();
	QBCLFormat format;
	ASSERT_TRUE(format.save(graph, "pivot.qbcl", archive, testSaveCtx));
	scenegraph::SceneGraph loaded;
	ASSERT_TRUE(format.load("pivot.qbcl", archive, loaded, testLoadCtx));
	loaded.updateTransforms();
	ASSERT_NE(nullptr, loaded.firstModelNode());
	EXPECT_EQ(root.property("qbcl_model_data"), loaded.root().property("qbcl_model_data"));
	EXPECT_EQ(root.name(), loaded.root().name());
	EXPECT_VEC_NEAR(graph.firstModelNode()->pivot(), loaded.firstModelNode()->pivot(), 0.0001f);
	EXPECT_VEC_NEAR(graph.firstModelNode()->transform(0).localTranslation(), loaded.firstModelNode()->transform(0).localTranslation(), 0.0001f);
}

TEST_F(QBCLFormatTest, testRejectsMalformedVoxelRuns) {
	// Each case describes a single column of height two.
	const uint8_t columns[][14] = {
		{2, 0, 2, 0, 0, 2, 255, 0, 0, 255}, // valid run
		{2, 0, 0, 0, 0, 2, 255, 0, 0, 255}, // zero run
		{2, 0, 3, 0, 0, 2, 255, 0, 0, 255}, // run exceeds height
		{1, 0, 2, 0, 0, 2}, // missing run color
		{1, 0, 255, 0, 0, 255}, // incomplete column
		{2, 0, 2, 0, 0, 2, 255, 0, 0, 255, 0, 0}, // extra column
	};
	const int lengths[] = {10, 10, 10, 6, 6, 12};
	for (int test = 0; test < 6; ++test) {
		SCOPED_TRACE(test);
		io::BufferedReadWriteStream compressed;
		{
			io::ZipWriteStream zip(compressed);
			ASSERT_NE(-1, zip.write(columns[test], lengths[test]));
		}
		io::BufferedReadWriteStream file;
		file.write("QBCL", 4);
		file.writeUInt32(131331);
		file.writeUInt32(2);
		file.writeUInt32(0);
		file.writeUInt32(0);
		for (int i = 0; i < 7; ++i) {
			file.writeUInt32(0); // empty metadata strings
		}
		file.writeUInt64(0);
		file.writeUInt64(0);
		for (int type = 1; type >= 0; --type) {
			file.writeUInt32(type);
			file.writeUInt32(1);
			file.writeUInt32(0); // empty name
			file.writeBool(true);
			file.writeBool(true);
			file.writeBool(false);
			if (type == 1) {
				for (int i = 0; i < 9; ++i) {
					file.writeUInt32(i < 3 ? 1 : 0);
				}
				file.writeUInt32(1); // child count
			}
		}
		file.writeUInt32(1);
		file.writeUInt32(2);
		file.writeUInt32(1);
		for (int i = 0; i < 6; ++i) {
			file.writeUInt32(0); // translation and pivot
		}
		file.writeUInt32(compressed.size());
		file.write(compressed.getBuffer(), compressed.size());
		io::MemoryArchivePtr archive = io::openMemoryArchive();
		ASSERT_TRUE(archive->add("runs.qbcl", file.getBuffer(), file.size()));
		QBCLFormat format;
		scenegraph::SceneGraph graph;
		EXPECT_EQ(test == 0, format.load("runs.qbcl", archive, graph, testLoadCtx));
	}
}

}
