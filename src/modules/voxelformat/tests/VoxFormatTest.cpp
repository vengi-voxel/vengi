/**
 * @file
 */

#include "voxelformat/private/magicavoxel/VoxFormat.h"
#include "voxelformat/private/magicavoxel/MagicaVoxel.h"
#include "AbstractFormatTest.h"
#include "core/ConfigVar.h"
#include "core/ScopedPtr.h"
#include "core/Var.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphTransform.h"
#include "util/VarUtil.h"
#include "voxel/MaterialColor.h"
#include "palette/Palette.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelutil/VoxelUtil.h"
#include "vox_glasses.h"
#include <glm/gtc/quaternion.hpp>

namespace voxelformat {

class VoxFormatTest : public AbstractFormatTest {};

static voxel::ValidateFlags voxIgnoreFlags() {
	// MagicaVoxel pivot is floor(size/2). Transform round-trip can differ by up to one voxel from
	// floor(M*(p+0.5)) vs continuous node TRS; skip full animation/matrix equality.
	return voxel::ValidateFlags::All &
		   ~(voxel::ValidateFlags::Pivot | voxel::ValidateFlags::Translation | voxel::ValidateFlags::Animations);
}

TEST_F(VoxFormatTest, testTransform) {
	testTransform("test-transform.vox");
}

TEST_F(VoxFormatTest, testLoad) {
	testLoad("magicavoxel.vox");
}

TEST_F(VoxFormatTest, DISABLED_testTeardownLoad) {
	testLoad("teardown.vox");
}

TEST_F(VoxFormatTest, testLoadMaterials) {
	VoxFormat f;
	scenegraph::SceneGraph mvSceneGraph;
	{
		testLoad(mvSceneGraph, "test_material.vox", 12u);
	}

	palette::Palette mvPalette;
	{
		scenegraph::SceneGraphNode *node = mvSceneGraph.firstModelNode();
		ASSERT_TRUE(node != nullptr);
		mvPalette = node->palette();
	}

	const core::String name = "test_material_vengi.vox";

	scenegraph::SceneGraph sceneGraph;

	io::ArchivePtr archive = helper_filesystemarchive();
	helper_saveSceneGraph(mvSceneGraph, name);
	testLoad(sceneGraph, name, 12u);

	scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_TRUE(node != nullptr);
	const palette::Palette &palette = node->palette();
	ASSERT_EQ(mvPalette.size(), palette.size());
	for (size_t i = 0; i < palette.size(); ++i) {
		EXPECT_EQ(mvPalette.color(i).r, palette.color(i).r) << "Invalid color at " << i;
		EXPECT_EQ(mvPalette.color(i).g, palette.color(i).g) << "Invalid color at " << i;
		EXPECT_EQ(mvPalette.color(i).b, palette.color(i).b) << "Invalid color at " << i;
		EXPECT_NEAR(mvPalette.color(i).a, palette.color(i).a, 1) << "Invalid alpha at " << i;
		EXPECT_EQ(mvPalette.material(i), palette.material(i)) << "Invalid material at " << i;
	}
}

static void compareWorldVolume(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node,
							   const voxel::RawVolume &golden, float maxDelta) {
	sceneGraph.updateTransforms();
	const glm::mat4 worldMat = sceneGraph.worldMatrix(node, 0);
	core::ScopedPtr<voxel::RawVolume> worldVolume(
		voxelutil::applyTransformToVolume(*sceneGraph.resolveVolume(node), worldMat, node.pivot()));
	ASSERT_NE(nullptr, (voxel::RawVolume *)worldVolume);
	volumeComparator(golden, voxel::getPalette(), *worldVolume, node.palette(), voxel::ValidateFlags::Color, maxDelta);
}

// only compile these tests in debug mode as they are quite big and lto is not a fan of that in terms of run times
#ifdef DEBUG
#include "vox_character.h"
TEST_F(VoxFormatTest, testLoadCharacter) {
	core::SharedPtr<voxel::RawVolume> volumes[] = {
		character_0::create(),	character_1::create(),	character_2::create(),	character_3::create(),
		character_4::create(),	character_5::create(),	character_6::create(),	character_7::create(),
		character_8::create(),	character_9::create(),	character_10::create(), character_11::create(),
		character_12::create(), character_13::create(), character_14::create(), character_15::create()};
	scenegraph::SceneGraph sceneGraph;
	io::FileDescription fileDesc;
	fileDesc.set("vox_character.vox");
	ASSERT_TRUE(voxelformat::loadFormat(fileDesc, helper_filesystemarchive(), sceneGraph, testLoadCtx));
	ASSERT_EQ(lengthof(volumes), (int)sceneGraph.size(scenegraph::SceneGraphNodeType::AllModels));
	auto iter = sceneGraph.begin(scenegraph::SceneGraphNodeType::AllModels);
	for (int i = 0; i < lengthof(volumes); ++i, ++iter) {
		compareWorldVolume(sceneGraph, *iter, *volumes[i].get(), 0.01f);
	}
}

#include "8ontop.h"
TEST_F(VoxFormatTest, testLoad8OnTop) {
	core::SharedPtr<voxel::RawVolume> volumes[] = {
		eightontop_0::create(),	 eightontop_1::create(),  eightontop_2::create(),  eightontop_3::create(),
		eightontop_4::create(),	 eightontop_5::create(),  eightontop_6::create(),  eightontop_7::create(),
		eightontop_8::create(),	 eightontop_9::create(),  eightontop_10::create(), eightontop_11::create(),
		eightontop_12::create(), eightontop_13::create(), eightontop_14::create(), eightontop_15::create(),
		eightontop_16::create(), eightontop_17::create(), eightontop_18::create(), eightontop_19::create(),
		eightontop_20::create(), eightontop_21::create(), eightontop_22::create(), eightontop_23::create(),
		eightontop_24::create(), eightontop_25::create(), eightontop_26::create(), eightontop_27::create(),
		eightontop_28::create(), eightontop_29::create(), eightontop_30::create(), eightontop_31::create(),
		eightontop_32::create(), eightontop_33::create(), eightontop_34::create(), eightontop_35::create(),
		eightontop_36::create(), eightontop_37::create(), eightontop_38::create(), eightontop_39::create(),
		eightontop_40::create(), eightontop_41::create(), eightontop_42::create(), eightontop_43::create(),
		eightontop_44::create(), eightontop_45::create(), eightontop_46::create(), eightontop_47::create(),
		eightontop_48::create(), eightontop_49::create(), eightontop_50::create(), eightontop_51::create(),
		eightontop_52::create(), eightontop_53::create(), eightontop_54::create(), eightontop_55::create(),
		eightontop_56::create(), eightontop_57::create(), eightontop_58::create(), eightontop_59::create(),
		eightontop_60::create(), eightontop_61::create(), eightontop_62::create(), eightontop_63::create(),
		eightontop_64::create(), eightontop_65::create(), eightontop_66::create(), eightontop_67::create(),
		eightontop_68::create(), eightontop_69::create(), eightontop_70::create(), eightontop_71::create(),
	};
	scenegraph::SceneGraph sceneGraph;
	io::FileDescription fileDesc;
	fileDesc.set("8ontop.vox");
	ASSERT_TRUE(voxelformat::loadFormat(fileDesc, helper_filesystemarchive(), sceneGraph, testLoadCtx));
	ASSERT_EQ(lengthof(volumes), (int)sceneGraph.size(scenegraph::SceneGraphNodeType::AllModels));
	auto iter = sceneGraph.begin(scenegraph::SceneGraphNodeType::AllModels);
	for (int i = 0; i < lengthof(volumes); ++i, ++iter) {
		compareWorldVolume(sceneGraph, *iter, *volumes[i].get(), 0.02f);
	}
}
#endif

TEST_F(VoxFormatTest, testLoadGlasses) {
	core::SharedPtr<voxel::RawVolume> volumes[] = {glasses_0::create()};
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "vox_glasses.vox", lengthof(volumes));
	ASSERT_EQ(lengthof(volumes), (int)sceneGraph.size(scenegraph::SceneGraphNodeType::AllModels));
	auto iter = sceneGraph.begin(scenegraph::SceneGraphNodeType::AllModels);
	for (int i = 0; i < lengthof(volumes); ++i, ++iter) {
		compareWorldVolume(sceneGraph, *iter, *volumes[i].get(), 0.011f);
	}
}

TEST_F(VoxFormatTest, testLoadRGB) {
	testRGB("rgb.vox");
}

TEST_F(VoxFormatTest, testLoadRGBSmall) {
	testRGBSmall("rgb_small.vox");
}

TEST_F(VoxFormatTest, testLoadRGBSmallSaveLoad) {
	testRGBSmallSaveLoad("rgb_small.vox");
}

TEST_F(VoxFormatTest, testSaveSmallVoxel) {
	VoxFormat f;
	const voxel::ValidateFlags flags =
		voxIgnoreFlags() & ~(voxel::ValidateFlags::SceneGraphModelsParent);
	testSaveLoadVoxel("mv-smallvolumesavetest.vox", &f, 0, 1, flags);
}

TEST_F(VoxFormatTest, testSaveMultipleModels) {
	VoxFormat f;
	testSaveMultipleModels("mv-multiplemodelsavetest.vox", &f, voxIgnoreFlags() & ~voxel::ValidateFlags::Palette);
}

TEST_F(VoxFormatTest, testSaveBigVolume) {
	VoxFormat f;
	const voxel::Region &region = voxel::Region::fromSize(glm::ivec3(1024, 1, 1));
	voxel::RawVolume bigVolume(region);
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	bigVolume.setVoxel(0, 0, 0, voxel);
	bigVolume.setVoxel(256, 0, 0, voxel);
	bigVolume.setVoxel(512, 0, 0, voxel);
	const core::String name = "bigvolume.vox";
	scenegraph::SceneGraph sceneGraph;
	scenegraph::SceneGraph sceneGraphsave;
	{
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setUnownedVolume(&bigVolume);
		sceneGraphsave.emplace(core::move(node));
	}

	io::ArchivePtr archive = helper_archive();
	ASSERT_TRUE(f.save(sceneGraphsave, name, archive, testSaveCtx));
	f.load(name, archive, sceneGraph, testLoadCtx);
	EXPECT_EQ(3, (int)sceneGraph.size(scenegraph::SceneGraphNodeType::AllModels));
}

TEST_F(VoxFormatTest, testSave) {
	VoxFormat f;
	testConvert("magicavoxel.vox", f, "magicavoxel-save.vox", f, voxIgnoreFlags());
}

TEST_F(VoxFormatTest, testAnimAsNodes) {
	core::getVar(cfg::VoxformatVOXAnimAsNodes)->setVal("true");
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "magicavoxel.vox");
	core::getVar(cfg::VoxformatVOXAnimAsNodes)->setVal("false");
}

TEST_F(VoxFormatTest, testAnimAsNodesSaveLoad) {
	scenegraph::SceneGraph saveGraph;
	{
		scenegraph::SceneGraphNode groupNode(scenegraph::SceneGraphNodeType::Group);
		groupNode.setName("anim");
		const int groupId = saveGraph.emplace(core::move(groupNode));
		ASSERT_NE(InvalidNodeId, groupId);

		for (int i = 0; i < 3; ++i) {
			scenegraph::SceneGraphNode modelNode(scenegraph::SceneGraphNodeType::Model);
			modelNode.setName(core::String::format("anim_frame_%i", i));
			const voxel::Region region(0, 0, 0, 1, 1, 1);
			modelNode.createVolume(region);
			modelNode.volume()->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
			ASSERT_NE(InvalidNodeId, saveGraph.emplace(core::move(modelNode), groupId));
		}
	}

	core::getVar(cfg::VoxformatVOXAnimAsNodes)->setVal("true");
	const core::String filename = "animasnodes-test.vox";
	ASSERT_TRUE(helper_saveSceneGraph(saveGraph, filename));

	scenegraph::SceneGraph loadGraph;
	testLoad(loadGraph, filename, 3);
	EXPECT_EQ(3u, loadGraph.size(scenegraph::SceneGraphNodeType::AllModels));
	core::getVar(cfg::VoxformatVOXAnimAsNodes)->setVal("false");
}

TEST_F(VoxFormatTest, testReferencesShareVolume) {
	util::ScopedVarChange apply(cfg::VoxformatMVApplyTransform, "false");
	scenegraph::SceneGraph sceneGraph;
	io::FileDescription fileDesc;
	fileDesc.set("test-transform.vox");
	ASSERT_TRUE(voxelformat::loadFormat(fileDesc, helper_filesystemarchive(), sceneGraph, testLoadCtx));
	int models = 0;
	int refs = 0;
	for (auto iter = sceneGraph.begin(scenegraph::SceneGraphNodeType::AllModels); iter != sceneGraph.end(); ++iter) {
		scenegraph::SceneGraphNode &node = *iter;
		if (node.isReferenceNode()) {
			++refs;
			EXPECT_NE(InvalidNodeId, node.reference());
			EXPECT_NE(nullptr, sceneGraph.resolveVolume(node));
		} else if (node.isModelNode()) {
			++models;
			EXPECT_NE(nullptr, node.volume());
		}
	}
	EXPECT_GT(models, 0);
	EXPECT_GT(refs, 0);
	EXPECT_EQ(20u, sceneGraph.size(scenegraph::SceneGraphNodeType::AllModels));
	EXPECT_EQ((size_t)models, sceneGraph.size(scenegraph::SceneGraphNodeType::Model));
}

TEST_F(VoxFormatTest, testApplyTransformBakesVoxels) {
	util::ScopedVarChange apply(cfg::VoxformatMVApplyTransform, "true");
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "test-transform.vox", 20u);

	int refs = 0;
	for (auto iter = sceneGraph.begin(scenegraph::SceneGraphNodeType::AllModels); iter != sceneGraph.end(); ++iter) {
		if ((*iter).isReferenceNode()) {
			++refs;
		}
	}
	EXPECT_EQ(0, refs);

	scenegraph::SceneGraphNode *node = nullptr;
	for (auto iter = sceneGraph.begin(scenegraph::SceneGraphNodeType::Model); iter != sceneGraph.end(); ++iter) {
		if ((*iter).name() == "original") {
			node = &(*iter);
			break;
		}
	}
	ASSERT_NE(nullptr, node);
	ASSERT_NE(nullptr, node->volume());
	EXPECT_EQ(glm::ivec3(0), node->region().getLowerCorner());
	EXPECT_TRUE(glm::all(glm::epsilonEqual(node->pivot(), glm::vec3(0.0f), 0.001f)));
	const scenegraph::SceneGraphTransform &transform = node->transform(0);
	EXPECT_NEAR(23.0f, transform.worldTranslation().x, 0.01f);
	EXPECT_NEAR(-2.0f, transform.worldTranslation().y, 0.01f);
	EXPECT_NEAR(23.0f, transform.worldTranslation().z, 0.01f);
	EXPECT_FALSE(voxel::isAir(node->volume()->voxel(0, 20, 0).getMaterial())) << *node->volume();
}

TEST_F(VoxFormatTest, testPaletteRemapPreservesSemiTransparentColors) {
	VoxFormat f;
	palette::Palette pal;
	for (int i = 0; i < 255; ++i) {
		pal.setColor(i, color::RGBA((uint8_t)(i + 1), (uint8_t)(i * 2), (uint8_t)(i + 50), 255));
	}
	const color::RGBA semiTransparent(0x2e, 0x5c, 0x6e, 0x7e);
	pal.setColor(189, semiTransparent);

	scenegraph::SceneGraph saveGraph;
	{
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		const voxel::Region region(0, 0, 0, 1, 1, 1);
		node.createVolume(region);
		node.volume()->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Transparent, 189));
		node.setPalette(pal);
		saveGraph.emplace(core::move(node));
	}

	const core::String filename = "test-semitransparent-remap.vox";
	io::ArchivePtr archive = helper_archive();
	ASSERT_TRUE(f.save(saveGraph, filename, archive, testSaveCtx));

	scenegraph::SceneGraph loadGraph;
	ASSERT_TRUE(f.load(filename, archive, loadGraph, testLoadCtx));

	const palette::Palette &loadedPal = loadGraph.firstPalette();
	bool found = false;
	for (int i = 0; i < loadedPal.colorCount(); ++i) {
		const color::RGBA c = loadedPal.color(i);
		if (c.r == semiTransparent.r && c.g == semiTransparent.g && c.b == semiTransparent.b &&
			c.a == semiTransparent.a) {
			found = true;
			break;
		}
	}
	EXPECT_TRUE(found) << "Semi-transparent color " << color::print(semiTransparent)
					   << " was lost during VOX save/load palette remap";
}

TEST_F(VoxFormatTest, testOgtPivotConversion) {
	// MagicaVoxel size 40x40x30 -> vengi region 40x30x40, pivot floor(size/2) with X flip.
	const glm::vec3 pivot = ogtNormalizedPivot(40, 40, 30);
	EXPECT_NEAR(19.0f / 40.0f, pivot.x, 0.0001f);
	EXPECT_NEAR(15.0f / 30.0f, pivot.y, 0.0001f);
	EXPECT_NEAR(20.0f / 40.0f, pivot.z, 0.0001f);
}

TEST_F(VoxFormatTest, testMatToOgtSnapsNonCardinalRotation) {
	// 45-degree bases used to snap two columns onto the same axis and produce invalid _r packs.
	const glm::quat q(glm::radians(glm::vec3(45.0f, 0.0f, 0.0f)));
	const glm::mat4 mat = glm::mat4_cast(q);
	const ogt_vox_transform t = matToOgt(mat);
	checkRotation(t);
	EXPECT_NE(glm::ivec3((int)t.m00, (int)t.m10, (int)t.m20), glm::ivec3(0));
	EXPECT_NE(glm::ivec3((int)t.m01, (int)t.m11, (int)t.m21), glm::ivec3(0));
	EXPECT_NE(glm::ivec3((int)t.m02, (int)t.m12, (int)t.m22), glm::ivec3(0));
	// Columns must be distinct axes.
	const int ax = (t.m00 != 0.0f) ? 0 : ((t.m01 != 0.0f) ? 1 : 2);
	const int ay = (t.m10 != 0.0f) ? 0 : ((t.m11 != 0.0f) ? 1 : 2);
	const int az = (t.m20 != 0.0f) ? 0 : ((t.m21 != 0.0f) ? 1 : 2);
	EXPECT_NE(ax, ay);
	EXPECT_NE(ax, az);
	EXPECT_NE(ay, az);
}

} // namespace voxelformat
