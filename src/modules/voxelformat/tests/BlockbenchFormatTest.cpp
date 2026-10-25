/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "core/ScopedPtr.h"
#include "core/Unicode.h"
#include "core/collection/Buffer.h"
#include "io/BufferedReadWriteStream.h"
#include "io/MemoryArchive.h"
#include "palette/Palette.h"
#include "math/tests/TestMathHelper.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "scenegraph/SceneGraphKeyFrame.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphTransform.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelformat/private/mesh/BlockbenchFormat.h"
#include "voxelformat/private/mesh/LZUTF8.h"
#include "voxelformat/tests/TestHelper.h"
#include <SDL3/SDL_stdinc.h>

namespace voxelformat {

class BlockbenchFormatTest : public AbstractFormatTest {
protected:
	// Loading the same file in different versions of Blockbench should yield the same results
	// TODO: for gltf/glb some stuff is different in the hierarchy - need to investigate further
	void test_loy_s_goodies_female_template(scenegraph::SceneGraph &sceneGraph, bool isbbmodel) {
		// Test hierarchy structure
		scenegraph::SceneGraphNode *hand_right = sceneGraph.findNodeByName("hand_right");
		ASSERT_NE(hand_right, nullptr);
		EXPECT_EQ(hand_right->type(), scenegraph::SceneGraphNodeType::Group);
		ASSERT_GE(hand_right->children().size(), 3u);

		// GLB child order and voxelized cube sizes can differ from bbmodel.
		scenegraph::SceneGraphNode *firstCube = nullptr;
		scenegraph::SceneGraphNode *secondCube = nullptr;
		scenegraph::SceneGraphNode *fingers_right = nullptr;
		int cubeModels = 0;
		for (const core::UUID &childUUID : hand_right->children()) {
			scenegraph::SceneGraphNode *child = sceneGraph.findNodeByUUID(childUUID);
			ASSERT_NE(nullptr, child);
			if (child->name() == "fingers_right") {
				fingers_right = child;
			}
			if (child->name() == "cube" && child->type() == scenegraph::SceneGraphNodeType::Model) {
				++cubeModels;
				const glm::ivec3 dim = child->region().getDimensionsInVoxels();
				if (firstCube == nullptr && dim == glm::ivec3(1, 2, 2)) {
					firstCube = child;
				} else if (secondCube == nullptr && dim == glm::ivec3(1, 1, 2)) {
					secondCube = child;
				} else if (firstCube == nullptr) {
					firstCube = child;
				} else if (secondCube == nullptr) {
					secondCube = child;
				}
			}
		}
		ASSERT_NE(nullptr, fingers_right);
		EXPECT_EQ(fingers_right->type(), scenegraph::SceneGraphNodeType::Group);
		EXPECT_GE(cubeModels, 2);

		if (isbbmodel) {
			ASSERT_NE(nullptr, firstCube);
			ASSERT_NE(nullptr, secondCube);
			EXPECT_EQ(firstCube->region().getDimensionsInVoxels(), glm::ivec3(1, 2, 2));
			EXPECT_EQ(secondCube->region().getDimensionsInVoxels(), glm::ivec3(1, 1, 2));
			const glm::vec3 expectedFirstPivot(-4.0f, 0.5f, 0.5f);
			EXPECT_VEC_NEAR(firstCube->pivot(), expectedFirstPivot, 0.0001f);
		}
		EXPECT_GT(fingers_right->children().size(), 0u);

		// Test animations
		ASSERT_EQ(sceneGraph.animations().size(), 6u);
		EXPECT_TRUE(sceneGraph.hasAnimation(DEFAULT_ANIMATION));
		EXPECT_TRUE(sceneGraph.hasAnimation("handsdown"));
		EXPECT_TRUE(sceneGraph.hasAnimation("sit"));
		EXPECT_TRUE(sceneGraph.hasAnimation("fist"));
		EXPECT_TRUE(sceneGraph.hasAnimation("walk"));
		EXPECT_TRUE(sceneGraph.hasAnimation("run"));

		// GLB export does not keep the Blockbench "main" group name.
		if (isbbmodel) {
			scenegraph::SceneGraphNode *main = sceneGraph.findNodeByName("main");
			ASSERT_NE(main, nullptr);
			EXPECT_EQ(main->type(), scenegraph::SceneGraphNodeType::Group);
		}

#if 0
		// TODO: VOXELFORMAT: these values were taken directly out of blockbench - need to verify why they don't match
		scenegraph::SceneGraphNode *eyeglossRight = sceneGraph.findNodeByName("eyegloss_right");
		ASSERT_NE(eyeglossRight, nullptr);
		EXPECT_EQ(eyeglossRight->type(), scenegraph::SceneGraphNodeType::Group);
		const scenegraph::SceneGraphTransform &eyeglossRightTransform = eyeglossRight->transform(0);
		EXPECT_VEC_NEAR(glm::vec3(3.0f, 27.4f, -3.91f), eyeglossRightTransform.worldTranslation(), 0.00001f);
		ASSERT_EQ(1u, eyeglossRight->children().size());

		const scenegraph::SceneGraphNode *eyeglossRightCube = sceneGraph.findNodeByUUID(eyeglossRight->children()[0]);
		ASSERT_NE(nullptr, eyeglossRightCube);
		EXPECT_EQ(eyeglossRightCube->type(), scenegraph::SceneGraphNodeType::Model);
		const scenegraph::SceneGraphTransform &eyeglossRightCubeTransform = eyeglossRightCube->transform(0);
		EXPECT_VEC_NEAR(glm::vec3(2.5f, 27.15f, -3.991f), eyeglossRightCubeTransform.worldTranslation(), 0.00001f);
		EXPECT_VEC_NEAR(glm::vec3(-0.25f, 23.9f, 0.0f), eyeglossRightCube->worldPivot(), 0.00001f);
#endif
	}
};

TEST_F(BlockbenchFormatTest, testLZUTF8RoundTrip) {
	const char *json =
		"{\"meta\":{\"format_version\":\"5.0\",\"model_format\":\"free\"},\"elements\":[],\"outliner\":[],\"textures\":[]}";
	const size_t len = SDL_strlen(json);
	core::Buffer<uint16_t> encoded;
	ASSERT_TRUE(lzutf8::encodeStorageBinaryString((const uint8_t *)json, len, encoded));
	core::Buffer<uint8_t> decoded;
	ASSERT_TRUE(lzutf8::decodeStorageBinaryString(encoded.data(), encoded.size(), decoded));
	ASSERT_EQ(decoded.size(), len);
	EXPECT_EQ(core::String((const char *)decoded.data(), decoded.size()), core::String(json));

	core::Buffer<uint8_t> decompressed;
	ASSERT_TRUE(lzutf8::decompress(decoded.data(), decoded.size(), decompressed));
	EXPECT_EQ(core::String((const char *)decompressed.data(), decompressed.size()), core::String(json));

	core::String compressedFile = "<lz>";
	for (size_t i = 0; i < encoded.size(); ++i) {
		char utf8[8];
		const int n = core::unicode::toUtf8(encoded[i], utf8, sizeof(utf8));
		ASSERT_GT(n, 0);
		compressedFile.append(utf8, n);
	}
	const core::String restored = lzutf8::decodeBBModelText(compressedFile);
	EXPECT_EQ(restored, core::String(json));
}

TEST_F(BlockbenchFormatTest, testStripJsonComments) {
	const core::String input = "{\n // comment\n \"a\": 1 /* block */, \"b\": \"keep // inside\"\n}";
	const core::String stripped = lzutf8::stripJsonComments(input);
	EXPECT_NE(stripped.find("\"a\""), core::String::npos);
	EXPECT_EQ(stripped.find("// comment"), core::String::npos);
	EXPECT_NE(stripped.find("keep // inside"), core::String::npos);
}

// the model comes from https://github.com/SL0ANE/Loy-s-Goodies/tree/main/models and was licensed under CC0
// version 4.5 - includes animations and a full scene with a hierarchy of nodes
TEST_F(BlockbenchFormatTest, testLoad_4_5) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "loy_s_goodies_female_template.bbmodel", 53);
	SCOPED_TRACE("loy_s_goodies_female_template.bbmodel");
	test_loy_s_goodies_female_template(sceneGraph, true);
}

// TODO: GLB hierarchy differs from bbmodel - needs investigation
TEST_F(BlockbenchFormatTest, testLoad_5_0_3_glb) {
	scenegraph::SceneGraph sceneGraphGLB;
	testLoad(sceneGraphGLB, "loy_s_goodies_female_template_5_0_3.glb", 53);
	SCOPED_TRACE("loy_s_goodies_female_template_5_0_3.glb");
	test_loy_s_goodies_female_template(sceneGraphGLB, false);
}

TEST_F(BlockbenchFormatTest, testLoad_5_0_3) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "loy_s_goodies_female_template_5_0_3.bbmodel", 53);
	SCOPED_TRACE("loy_s_goodies_female_template_5_0_3.bbmodel");
	test_loy_s_goodies_female_template(sceneGraph, true);
}

// this model is based on a model from https://github.com/SL0ANE/Loy-s-Goodies/tree/main/models - but only one cube was
// extracted to simplify the scene this was done in the web based version of blockbench - https://web.blockbench.net/ -
// on saving the file the version was updated to 4.10
TEST_F(BlockbenchFormatTest, testLoad_4_10) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "female_template_head_4_10.bbmodel", 1);
	scenegraph::SceneGraphNode *cube = sceneGraph.findNodeByName("cube");
	ASSERT_NE(cube, nullptr);
	ASSERT_EQ(cube->children().size(), 0u);
	voxel::RawVolume *volume = cube->volume();
	ASSERT_NE(volume, nullptr);
	const voxel::Region &region = volume->region();
	const glm::ivec3 &dim = region.getDimensionsInVoxels();
	EXPECT_EQ(dim.x, 8);
	EXPECT_EQ(dim.y, 8);
	EXPECT_EQ(dim.z, 8);
	const voxel::Voxel v1 = volume->voxel(region.getLowerCenter());
	EXPECT_TRUE(voxel::isBlocked(v1.getMaterial()));
	// TODO: VOXELFORMAT: compare colors - needed because we overwrite the color while loading all faces for a cube
}

// this model was created in the blockbench web edition and includes all mesh types
TEST_F(BlockbenchFormatTest, testLoadMeshTypes) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "blockbench_meshtypes.bbmodel", 10);
}

TEST_F(BlockbenchFormatTest, testSaveLoadVoxel) {
	BlockbenchFormat f;
	testSaveLoadVoxel("testSaveLoadVoxel.bbmodel", &f, 0, 1, voxel::ValidateFlags::Region);
}

TEST_F(BlockbenchFormatTest, testSave) {
	BlockbenchFormat f;
	palette::Palette pal;
	pal.magicaVoxel();
	testSave("testSave.bbmodel", &f, pal,
			 voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Palette | voxel::ValidateFlags::Color));
}

TEST_F(BlockbenchFormatTest, testSaveLoadInterpolation) {
	scenegraph::SceneGraph graph;
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setName("bone");
	node.createVolume(voxel::Region(0, 0));
	node.volume()->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	const int nodeId = graph.emplace(core::move(node));
	ASSERT_NE(InvalidNodeId, nodeId);

	const core::String clip = "wave";
	ASSERT_TRUE(graph.addAnimation(clip));
	for (const auto &entry : graph.nodes()) {
		scenegraph::SceneGraphNode &n = entry->value;
		n.setAnimation(clip);
		n.setKeyFrames(n.keyFrames(DEFAULT_ANIMATION));
	}
	ASSERT_TRUE(graph.setAnimation(clip));

	scenegraph::SceneGraphNode &animNode = graph.node(nodeId);
	const scenegraph::SceneGraphTransform rest = animNode.transform(0);
	struct KeySpec {
		scenegraph::FrameIndex frame;
		scenegraph::InterpolationType interpolation;
		glm::vec3 delta;
	};
	const KeySpec specs[] = {
		{24, scenegraph::InterpolationType::CatmullRom, glm::vec3(4.0f, 0.0f, 0.0f)},
		{48, scenegraph::InterpolationType::Instant, glm::vec3(8.0f, 0.0f, 0.0f)},
		{72, scenegraph::InterpolationType::QuadEaseIn, glm::vec3(12.0f, 0.0f, 0.0f)},
	};
	for (const KeySpec &spec : specs) {
		const int key = animNode.addKeyFrame(spec.frame);
		ASSERT_NE(InvalidKeyFrame, key);
		scenegraph::SceneGraphTransform t = rest;
		t.setLocalTranslation(rest.localTranslation() + spec.delta);
		animNode.setTransform(key, t);
		animNode.keyFrame(key).interpolation = spec.interpolation;
	}
	animNode.keyFrame(0).interpolation = scenegraph::InterpolationType::CatmullRom;
	graph.updateTransforms();

	BlockbenchFormat format;
	const io::MemoryArchivePtr archive = io::openMemoryArchive();
	ASSERT_TRUE(format.save(graph, "interp.bbmodel", archive, testSaveCtx));

	core::ScopedPtr<io::SeekableReadStream> jsonStream(archive->readStream("interp.bbmodel"));
	ASSERT_TRUE(jsonStream);
	core::String json;
	ASSERT_TRUE(jsonStream->readString((int)jsonStream->size(), json));
	EXPECT_NE(core::String::npos, json.find("\"catmullrom\""));
	EXPECT_NE(core::String::npos, json.find("\"step\""));
	EXPECT_NE(core::String::npos, json.find("\"ease_in\""));

	scenegraph::SceneGraph loaded;
	ASSERT_TRUE(format.load("interp.bbmodel", archive, loaded, testLoadCtx));
	ASSERT_TRUE(loaded.hasAnimation(clip));
	ASSERT_TRUE(loaded.setAnimation(clip));
	scenegraph::SceneGraphNode *loadedNode = loaded.findNodeByName("bone");
	ASSERT_NE(nullptr, loadedNode);
	auto expectInterpolation = [&](scenegraph::FrameIndex frame, scenegraph::InterpolationType type) {
		scenegraph::KeyFrameIndex kfIdx = InvalidKeyFrame;
		ASSERT_TRUE(loadedNode->hasKeyFrameForFrame(frame, &kfIdx));
		EXPECT_EQ(type, loadedNode->keyFrame(kfIdx).interpolation) << frame;
	};
	expectInterpolation(24, scenegraph::InterpolationType::CatmullRom);
	expectInterpolation(48, scenegraph::InterpolationType::Instant);
	expectInterpolation(72, scenegraph::InterpolationType::QuadEaseIn);
}

} // namespace voxelformat
