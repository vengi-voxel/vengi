/**
 * @file
 */

#include "scenegraph/JsonExporter.h"
#include "app/tests/AbstractTest.h"
#include "io/BufferedReadWriteStream.h"
#include "scenegraph/SceneGraph.h"
#include "voxel/RawVolume.h"
#include <gtest/gtest.h>

namespace scenegraph {

class JsonExporterTest : public app::AbstractTest {};

TEST_F(JsonExporterTest, testExportToBufferedStream) {
	SceneGraph sceneGraph;
	voxel::RawVolume v(voxel::Region(0, 1));
	SceneGraphNode node(SceneGraphNodeType::Model);
	node.setVolume(&v, false);
	node.setName("model1");
	sceneGraph.emplace(core::move(node), 0);

	io::BufferedReadWriteStream stream;
	const uint32_t flags = JSONEXPORTER_PALETTE | JSONEXPORTER_NODEDETAILS | JSONEXPORTER_CHILDREN;
	sceneGraphJson(sceneGraph, stream, flags);
	stream.seek(0);
	std::string json((const char *)stream.getBuffer(), (size_t)stream.size());
	EXPECT_FALSE(json.empty());
	EXPECT_NE(json.find("model1"), std::string::npos);
	EXPECT_NE(json.find("volume"), std::string::npos);
	EXPECT_NE(json.find("voxel_count"), std::string::npos);
}

class JsonExporterFlagsTest : public app::AbstractTest {
protected:
	SceneGraph _sceneGraph;
	voxel::RawVolume *_parentVolume = nullptr;
	voxel::RawVolume *_childVolume = nullptr;

	void SetUp() override {
		app::AbstractTest::SetUp();
		_parentVolume = new voxel::RawVolume(voxel::Region(0, 1));
		_childVolume = new voxel::RawVolume(voxel::Region(0, 1));

		SceneGraphNode parent(SceneGraphNodeType::Model);
		parent.setVolume(_parentVolume, true);
		parent.setName("parent");
		int parentId = _sceneGraph.emplace(core::move(parent), 0);

		SceneGraphNode child(SceneGraphNodeType::Model);
		child.setVolume(_childVolume, true);
		child.setName("child");
		_sceneGraph.emplace(core::move(child), parentId);
	}

	std::string exportJson(uint32_t flags) {
		io::BufferedReadWriteStream stream;
		sceneGraphJson(_sceneGraph, stream, flags);
		return std::string((const char *)stream.getBuffer(), (size_t)stream.size());
	}
};

TEST_F(JsonExporterFlagsTest, testSkipNodedetails) {
	uint32_t flags = JSONEXPORTER_NODEDETAILS | JSONEXPORTER_CHILDREN;
	std::string withFlag = exportJson(flags);
	std::string withoutFlag = exportJson(flags & ~JSONEXPORTER_NODEDETAILS);
	EXPECT_NE(withFlag.find("\"volume\""), std::string::npos);
	EXPECT_EQ(withoutFlag.find("\"volume\""), std::string::npos);
}

TEST_F(JsonExporterFlagsTest, testSkipChildren) {
	uint32_t flags = JSONEXPORTER_NODEDETAILS | JSONEXPORTER_CHILDREN;
	std::string withFlag = exportJson(flags);
	std::string withoutFlag = exportJson(flags & ~JSONEXPORTER_CHILDREN);
	EXPECT_NE(withFlag.find("\"children\""), std::string::npos);
	EXPECT_EQ(withoutFlag.find("\"children\""), std::string::npos);
}

TEST_F(JsonExporterFlagsTest, testChildrenWithSkipNodedetails) {
	// This tests the bug fix: children should appear even when nodedetails is skipped
	std::string json = exportJson(JSONEXPORTER_CHILDREN);
	EXPECT_NE(json.find("\"children\""), std::string::npos);
	EXPECT_NE(json.find("child"), std::string::npos);
}

} // namespace scenegraph
