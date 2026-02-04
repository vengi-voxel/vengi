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

} // namespace scenegraph
