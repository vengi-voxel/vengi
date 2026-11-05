/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "core/ScopedPtr.h"
#include "io/BufferedReadWriteStream.h"
#include "io/MemoryArchive.h"
#include "io/ZipArchive.h"
#include "json/JSON.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"
#include "voxelformat/private/image/PxoFormat.h"

namespace voxelformat {

class PxoFormatTest : public AbstractFormatTest {};

TEST_F(PxoFormatTest, testLoad) {
	testLoad("pixelorama.pxo", 2);
}

TEST_F(PxoFormatTest, testLoadCroppedCels) {
	// Optional Pixelorama example (shrink-to-content cels). Not checked into this repo:
	// https://github.com/Orama-Interactive/Pixelorama/blob/master/Misc/example_pxo_files/animated_layer_effect.pxo
	testLoad("animated_layer_effect.pxo", 6);
}

TEST_F(PxoFormatTest, testLoad3dLayer) {
	// Optional Pixelorama example (Godot 3D SubViewport layer). Not checked into this repo:
	// https://github.com/Orama-Interactive/Pixelorama/blob/master/Misc/example_pxo_files/3d_layer_drawing.pxo
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "3d_layer_drawing.pxo", 1);
	if (HasFatalFailure() || IsSkipped()) {
		return;
	}
	const scenegraph::SceneGraphNode *layer = sceneGraph.findNodeByName("Layer 2");
	ASSERT_NE(nullptr, layer);
	EXPECT_EQ(scenegraph::SceneGraphNodeType::Group, layer->type());
	const scenegraph::SceneGraphNode *box = sceneGraph.findNodeByName("BoxMesh");
	ASSERT_NE(nullptr, box);
	ASSERT_TRUE(box->isModelNode());
	ASSERT_NE(nullptr, box->volume());
	// Unit BoxMesh scaled for Pixelorama albedo detail; must be volumetric, not a flat preview slice.
	EXPECT_GT(box->region().getDepthInVoxels(), 1);
	EXPECT_GT(voxelutil::countVoxels(*box->volume()), 100);
}

TEST_F(PxoFormatTest, testLoadVoxeloramaDepth) {
	// Synthetic Voxelorama VoxelDepth metadata:
	// https://github.com/Orama-Interactive/VoxeloramaExtension
	const int width = 2;
	const int height = 2;
	json::Json root = json::Json::object();
	root.set("pxo_version", 8);
	root.set("size_x", width);
	root.set("size_y", height);
	json::Json layers = json::Json::array();
	json::Json layer = json::Json::object();
	layer.set("name", "Layer 1");
	layer.set("type", 0);
	layer.set("parent", -1);
	layer.set("visible", true);
	layer.set("opacity", 1.0);
	layers.push(layer);
	root.set("layers", layers);

	json::Json depth = json::Json::array();
	json::Json col0 = json::Json::array();
	col0.push(3.0f);
	col0.push(1.0f);
	json::Json col1 = json::Json::array();
	col1.push(1.0f);
	col1.push(1.0f);
	depth.push(col0);
	depth.push(col1);
	json::Json meta = json::Json::object();
	meta.set("VoxelDepth", depth);
	json::Json cel = json::Json::object();
	cel.set("opacity", 1.0);
	cel.set("metadata", meta);
	json::Json cels = json::Json::array();
	cels.push(cel);
	json::Json frame = json::Json::object();
	frame.set("duration", 1.0);
	frame.set("cels", cels);
	json::Json frames = json::Json::array();
	frames.push(frame);
	root.set("frames", frames);

	uint8_t rgba[2 * 2 * 4] = {};
	rgba[0] = 255;
	rgba[1] = 0;
	rgba[2] = 0;
	rgba[3] = 255; // (0,0) opaque red

	io::BufferedReadWriteStream zipFileStream(4096);
	{
		io::ZipArchive zipArchive;
		ASSERT_TRUE(zipArchive.init(&zipFileStream));
		{
			core::ScopedPtr<io::SeekableWriteStream> jsonStream(zipArchive.writeStream("data.json"));
			ASSERT_TRUE(jsonStream);
			ASSERT_TRUE(jsonStream->writeString(root.dump(1), false));
		}
		{
			core::ScopedPtr<io::SeekableWriteStream> mimeStream(zipArchive.writeStream("mimetype"));
			ASSERT_TRUE(mimeStream);
			ASSERT_TRUE(mimeStream->writeString("application/x-pixelorama", false));
		}
		{
			core::ScopedPtr<io::SeekableWriteStream> layerStream(
				zipArchive.writeStream("image_data/frames/1/layer_1"));
			ASSERT_TRUE(layerStream);
			ASSERT_EQ((int)sizeof(rgba), layerStream->write(rgba, sizeof(rgba)));
		}
		zipArchive.shutdown();
	}
	zipFileStream.seek(0);

	const io::MemoryArchivePtr archive = io::openMemoryArchive();
	ASSERT_TRUE(archive->add("voxelorama.pxo", zipFileStream.getBuffer(), (size_t)zipFileStream.size()));

	PxoFormat format;
	scenegraph::SceneGraph sceneGraph;
	ASSERT_TRUE(format.load("voxelorama.pxo", archive, sceneGraph, testLoadCtx));
	ASSERT_EQ(1u, sceneGraph.size());
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	ASSERT_NE(nullptr, node->volume());
	EXPECT_EQ(3, node->region().getDepthInVoxels());
	EXPECT_EQ(3, voxelutil::countVoxels(*node->volume()));
	// Image (0,0) maps to voxel y = height-1.
	EXPECT_FALSE(voxel::isAir(node->volume()->voxel(0, height - 1, 0).getMaterial()));
	EXPECT_FALSE(voxel::isAir(node->volume()->voxel(0, height - 1, 1).getMaterial()));
	EXPECT_FALSE(voxel::isAir(node->volume()->voxel(0, height - 1, 2).getMaterial()));
	EXPECT_TRUE(node->property("voxelorama") == "true");
}

TEST_F(PxoFormatTest, testSaveSingleVoxel) {
	PxoFormat f;
	// Saving maps volume depth to animation frames; a single-voxel volume
	// round-trips as one frame / one model.
	testSaveSingleVoxel("pxo-savetest.pxo", &f);
}

TEST_F(PxoFormatTest, testLoadScreenshot) {
	testLoadScreenshot("pixelorama.pxo", 2, 2, color::RGBA(255, 0, 0, 255), 0, 0);
}

} // namespace voxelformat
