/**
 * @file
 */

#include "voxelformat/private/mesh/GLTFFormat.h"
#include "AbstractFormatTest.h"
#include "core/ConfigVar.h"
#include "core/ScopedPtr.h"
#include "core/String.h"
#include "io/Stream.h"
#include "io/MemoryArchive.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphNodeProperties.h"
#include "util/VarUtil.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelformat/tests/TestHelper.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelformat {

class GLTFFormatTest : public AbstractFormatTest {};

TEST_F(GLTFFormatTest, testSceneVoxelSize) {
	util::ScopedVarChange voxelSize(cfg::VoxformatVoxelSize, "24");
	util::ScopedVarChange voxelMode(cfg::VoxformatVoxelizeMode, "1");
	util::ScopedVarChange createPalette(cfg::VoxelCreatePalette, "true");
	util::ScopedVarChange fillHollow(cfg::VoxformatFillHollow, "false");
	const char json[] = R"({
		"asset":{"version":"2.0"},
		"buffers":[{"uri":"scene.bin","byteLength":104}],
		"bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":36},
			{"buffer":0,"byteOffset":36,"byteLength":36},
			{"buffer":0,"byteOffset":72,"byteLength":8},
			{"buffer":0,"byteOffset":80,"byteLength":24}],
		"accessors":[{"bufferView":0,"componentType":5126,"count":3,"type":"VEC3","min":[0,0,0],"max":[2,2,0]},
			{"bufferView":1,"componentType":5126,"count":3,"type":"VEC3","min":[0,0,0],"max":[4,4,0]},
			{"bufferView":2,"componentType":5126,"count":2,"type":"SCALAR","min":[0],"max":[1]},
			{"bufferView":3,"componentType":5126,"count":2,"type":"VEC3"}],
		"meshes":[{"primitives":[{"attributes":{"POSITION":0}}]},
			{"primitives":[{"attributes":{"POSITION":1}}]}],
		"nodes":[{"name":"parent","translation":[2,0,0],"children":[1]},
			{"name":"small","mesh":0},{"name":"large","mesh":1,"translation":[10,0,0]}],
		"scenes":[{"nodes":[0,2]}],"scene":0,
		"animations":[{"name":"move","samplers":[{"input":2,"output":3}],
			"channels":[{"sampler":0,"target":{"node":2,"path":"translation"}}]}]
	})";
	const float buffer[] = {0, 0, 0, 2, 0, 0, 0, 2, 0,
		0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 1, 10, 0, 0, 12, 0, 0};
	const io::MemoryArchivePtr archive = io::openMemoryArchive();
	ASSERT_TRUE(archive->add("scene.gltf", (const uint8_t *)json, sizeof(json) - 1));
	ASSERT_TRUE(archive->add("scene.bin", (const uint8_t *)buffer, sizeof(buffer)));
	GLTFFormat format;
	scenegraph::SceneGraph graph;
	ASSERT_TRUE(format.loadGroups("scene.gltf", archive, graph, testLoadCtx));
	const scenegraph::SceneGraphNode *small = graph.findNodeByName("small");
	scenegraph::SceneGraphNode *large = graph.findNodeByName("large");
	ASSERT_NE(nullptr, small);
	ASSERT_NE(nullptr, large);
	EXPECT_LE(small->region().getDimensionsInVoxels().x, 5);
	EXPECT_GE(small->region().getDimensionsInVoxels().x, 4);
	EXPECT_LE(large->region().getDimensionsInVoxels().x, 9);
	EXPECT_GE(large->region().getDimensionsInVoxels().x, 8);
	EXPECT_FLOAT_EQ(4.0f, small->transform(0).worldTranslation().x);
	ASSERT_TRUE(large->setAnimation("move"));
	EXPECT_FLOAT_EQ(20.0f, large->transform(0).localTranslation().x);
	EXPECT_FLOAT_EQ(24.0f, large->transform(1).localTranslation().x);
}

TEST_F(GLTFFormatTest, testExportMesh) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "rgb.qb");
	helper_saveSceneGraph(sceneGraph, "exportrgb.gltf");
}

TEST_F(GLTFFormatTest, testImportMeshAnimationCompare) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "chr_oldman.vengi", 10);

	// compare with chr_oldman.gltf which was exported from the same source file
	scenegraph::SceneGraph sceneGraph2;
	testLoad(sceneGraph2, "chr_oldman.gltf", 10);

	const voxel::ValidateFlags flags = (voxel::ValidateFlags::Mesh & ~voxel::ValidateFlags::Color & ~voxel::ValidateFlags::Pivot);
	voxel::sceneGraphComparator(sceneGraph, sceneGraph2, flags);
}

TEST_F(GLTFFormatTest, testSaveChrKnight) {
	GLTFFormat format;
	const voxel::ValidateFlags flags = (voxel::ValidateFlags::Mesh & ~voxel::ValidateFlags::Color & ~voxel::ValidateFlags::Pivot);
	testSaveMesh("chr_knight.qbcl", "chr_knight.gltf", &format, flags);
}

TEST_F(GLTFFormatTest, testSaveCC) {
	GLTFFormat format;
	const voxel::ValidateFlags flags = (voxel::ValidateFlags::Mesh & ~voxel::ValidateFlags::Color & ~voxel::ValidateFlags::Pivot);
	testSaveMesh("cc.vxl", "cc.gltf", &format, flags);
}

TEST_F(GLTFFormatTest, testSaveLoadPointCloud) {
	GLTFFormat format;
	testSaveLoadPointCloud("pointcloud-saveload.gltf", &format);
}

TEST_F(GLTFFormatTest, testImportAnimation) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "glTF/BoxAnimated.glb", 2);
	auto iter = sceneGraph.beginModel();
	++iter;
	scenegraph::SceneGraphNode &node = *iter;
	EXPECT_GE(sceneGraph.animations().size(), 1u);
	EXPECT_EQ("animation 0", sceneGraph.animations().back());
	EXPECT_TRUE(sceneGraph.setAnimation(sceneGraph.animations().back()));
	ASSERT_FALSE(node.keyFrames()->empty());
	ASSERT_GE(node.keyFrames()->size(), 2u);
}

TEST_F(GLTFFormatTest, testAnimationRoundTrip) {
	GLTFFormat f;
	const io::ArchivePtr &archive = helper_filesystemarchive();
	scenegraph::SceneGraph srcGraph;
	ASSERT_TRUE(f.load("chr_oldman.gltf", archive, srcGraph, testLoadCtx));
	ASSERT_EQ(srcGraph.size(scenegraph::SceneGraphNodeType::AllModels), 10u);
	ASSERT_GE(srcGraph.animations().size(), 1u);

	const core::String outFile = "chr_oldman-roundtrip.gltf";
	ASSERT_TRUE(f.save(srcGraph, outFile, archive, testSaveCtx));

	scenegraph::SceneGraph dstGraph;
	ASSERT_TRUE(f.load(outFile, archive, dstGraph, testLoadCtx));

	const auto &srcAnims = srcGraph.animations();
	const auto &dstAnims = dstGraph.animations();
	ASSERT_EQ(srcAnims.size(), dstAnims.size()) << "Animation count mismatch";
	for (const core::String &anim : srcAnims) {
		bool found = false;
		for (const core::String &dstAnim : dstAnims) {
			if (dstAnim == anim) {
				found = true;
				break;
			}
		}
		EXPECT_TRUE(found) << "Animation '" << anim.c_str() << "' not found after round-trip";
	}

	int srcAnimatedNodes = 0;
	int dstAnimatedNodes = 0;
	for (const auto &srcEntry : srcGraph.nodes()) {
		const scenegraph::SceneGraphNode &srcNode = srcEntry->second;
		if (!srcNode.isModelNode() && !srcNode.isGroupNode()) {
			continue;
		}
		for (const core::String &anim : srcAnims) {
			if (!srcNode.allKeyFrames().hasKey(anim)) {
				continue;
			}
			const auto &srcKfs = srcNode.keyFrames(anim);
			if (srcKfs.size() <= 1) {
				continue;
			}
			++srcAnimatedNodes;
			for (const auto &dstEntry : dstGraph.nodes()) {
				const scenegraph::SceneGraphNode &dstNode = dstEntry->second;
				if (dstNode.name() != srcNode.name()) {
					continue;
				}
				if (dstNode.allKeyFrames().hasKey(anim) && dstNode.keyFrames(anim).size() > 1) {
					++dstAnimatedNodes;
				}
				break;
			}
			break;
		}
	}
	EXPECT_GT(srcAnimatedNodes, 0) << "Source should have animated nodes";
	EXPECT_EQ(srcAnimatedNodes, dstAnimatedNodes) << "Animated nodes lost keyframes during round-trip";

	// Exported glTF must use TRS (not matrix) on animated nodes - matrix + animation is invalid.
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(outFile));
	ASSERT_TRUE(stream);
	const int64_t size = stream->size();
	core::String json;
	json.reserve((size_t)size);
	for (;;) {
		char chunk[4096];
		const int n = stream->read(chunk, sizeof(chunk));
		if (n <= 0) {
			break;
		}
		json.append(chunk, n);
	}
	EXPECT_EQ(core::String::npos, json.find("\"matrix\""))
		<< "Animated glTF export must not write node matrices";
}

TEST_F(GLTFFormatTest, testVoxelizeCube) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "glTF/cube/Cube.gltf", 1);
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	const voxel::RawVolume *v = node->volume();
	ASSERT_NE(nullptr, v);
	EXPECT_TRUE(voxel::isBlocked(v->voxel(-1, -1, -1).getMaterial()));
	EXPECT_TRUE(voxel::isBlocked(v->voxel(-1, 0, -1).getMaterial()));
	EXPECT_TRUE(voxel::isBlocked(v->voxel(0, 0, 0).getMaterial()));
	EXPECT_TRUE(voxel::isBlocked(v->voxel(0, -1, -1).getMaterial()));
}

TEST_F(GLTFFormatTest, testRGB) {
	testRGB("rgb.gltf");
}

TEST_F(GLTFFormatTest, testSaveLoadVoxel) {
	GLTFFormat f;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~voxel::ValidateFlags::Palette;
	testSaveLoadVoxel("bv-smallvolumesavetest.gltf", &f, 0, 10, flags);
}

TEST_F(GLTFFormatTest, testNodeProperties) {
	GLTFFormat format;
	palette::Palette palette;
	palette.nippon();

	scenegraph::SceneGraph sceneGraph;
	sceneGraph.node(0).setProperty(scenegraph::PropAuthor, "vengi");
	sceneGraph.node(0).setProperty(scenegraph::PropTitle, "property-roundtrip");
	{
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setName("prop-node");
		voxel::RawVolume *volume = new voxel::RawVolume(voxel::Region(glm::ivec3(0), glm::ivec3(0)));
		volume->setVoxel(0, 0, 0, voxel::createVoxel(palette, 1));
		node.setVolume(volume);
		node.setPalette(palette);
		node.setProperty(scenegraph::PropAuthor, "node-author");
		node.setProperty("custom", "value");
		sceneGraph.emplace(core::move(node));
	}

	const core::String filename = "node-properties.gltf";
	const io::ArchivePtr &archive = helper_filesystemarchive();
	ASSERT_TRUE(format.save(sceneGraph, filename, archive, testSaveCtx));

	{
		core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
		ASSERT_TRUE(stream);
		core::String json;
		json.reserve((size_t)stream->size());
		for (;;) {
			char chunk[4096];
			const int n = stream->read(chunk, sizeof(chunk));
			if (n <= 0) {
				break;
			}
			json.append(chunk, n);
		}
		EXPECT_NE(core::String::npos, json.find("VENGI_materials"));
		EXPECT_NE(core::String::npos, json.find("VENGI_properties"));
		EXPECT_NE(core::String::npos, json.find("extensionsUsed"));
	}

	scenegraph::SceneGraph loaded;
	ASSERT_TRUE(format.load(filename, archive, loaded, testLoadCtx));
	EXPECT_EQ("vengi", loaded.node(0).property(scenegraph::PropAuthor));
	EXPECT_EQ("property-roundtrip", loaded.node(0).property(scenegraph::PropTitle));
	const scenegraph::SceneGraphNode *loadedNode = loaded.firstModelNode();
	ASSERT_NE(nullptr, loadedNode);
	EXPECT_EQ("node-author", loadedNode->property(scenegraph::PropAuthor));
	EXPECT_EQ("value", loadedNode->property("custom"));
}

class VoxelizeLantern : public AbstractFormatTest, public ::testing::WithParamInterface<bool> {};

TEST_P(VoxelizeLantern, exec) {
	bool createPalette = GetParam();
	util::ScopedVarChange var(cfg::VoxelCreatePalette, createPalette);
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "glTF/lantern/Lantern.gltf", 3u);
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	EXPECT_EQ("LanternPole_Body", node->name());
	const voxel::RawVolume *v = node->volume();
	ASSERT_NE(nullptr, v);
	const voxel::Region &region = v->region();
	EXPECT_EQ(-9, region.getLowerX());
	EXPECT_EQ(-14, region.getLowerY());
	EXPECT_EQ(-4, region.getLowerZ());
	EXPECT_EQ(8, region.getUpperX());
	EXPECT_EQ(13, region.getUpperY());
	EXPECT_EQ(3, region.getUpperZ());
	EXPECT_EQ(286, voxelutil::countVoxels(*v));
	// TODO: VOXELFORMAT: https://github.com/vengi-voxel/vengi/issues/620
	// EXPECT_EQ(89, v->voxel(-8, 9, 0).getColor());
	const color::RGBA expected(69, 58, 46, 255);
	const color::RGBA is = node->palette().color(v->voxel(-8, 9, 0).getColor());
	// when not creating a palette from the mesh, the default palette quantization has more color error
	const float maxDelta = createPalette ? 0.01f : 0.03f;
	voxel::colorComparatorDistance(expected, is, maxDelta);
}

INSTANTIATE_TEST_SUITE_P(
	GLTFFormatTest,
	VoxelizeLantern,
	::testing::Values(true, false),
	[](const testing::TestParamInfo<bool>& nfo) {
		if (nfo.param) {
			return "createpalette";
		}
		return "nocreatepalette";
	}
);

} // namespace voxelformat
