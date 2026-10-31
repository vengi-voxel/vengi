/**
 * @file
 */

#include "voxelformat/private/magicavoxel/VoxFormat.h"
#include "voxelformat/private/magicavoxel/MagicaVoxel.h"
#include "AbstractFormatTest.h"
#include "core/ConfigVar.h"
#include "core/ScopedPtr.h"
#include "core/Var.h"
#include "core/collection/Buffer.h"
#include "io/BufferedReadWriteStream.h"
#include "io/MemoryArchive.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphNodeProperties.h"
#include "scenegraph/SceneGraphTransform.h"
#include "util/VarUtil.h"
#include "voxel/MaterialColor.h"
#include "palette/Palette.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelutil/VolumeVisitor.h"
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

TEST_F(VoxFormatTest, testTeardownLoad) {
	testLoad("teardown.vox", 8);
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
			EXPECT_TRUE(node.hasReference());
			EXPECT_NE(nullptr, sceneGraph.findNodeByUUID(node.referenceUUID()));
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

TEST_F(VoxFormatTest, testFullPalettePreservesUsedColors) {
	palette::Palette palette;
	for (int i = 0; i < palette::PaletteMaxColors; ++i) {
		palette.setColor(i, color::RGBA((uint8_t)i, (uint8_t)(255 - i), 37, 255));
	}
	palette.setMaterial(0, palette::Material());
	palette.setEmit(0, 0.75f);
	scenegraph::SceneGraph sceneGraph;
	for (int i = 0; i < 2; ++i) {
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setName(i == 0 ? "first" : "second");
		node.createVolume(voxel::Region(0, 0, 0, 1, 0, 0));
		node.setPalette(palette);
		node.volume()->setVoxel(0, 0, 0, voxel::createVoxel(palette, i));
		node.volume()->setVoxel(1, 0, 0, voxel::createVoxel(palette, 255));
		sceneGraph.emplace(core::move(node));
	}
	VoxFormat format;
	io::ArchivePtr archive = helper_archive();
	ASSERT_TRUE(format.save(sceneGraph, "full-palette.vox", archive, testSaveCtx));
	scenegraph::SceneGraph loaded;
	ASSERT_TRUE(format.load("full-palette.vox", archive, loaded, testLoadCtx));
	ASSERT_EQ(2u, loaded.size(scenegraph::SceneGraphNodeType::AllModels));
	int checked = 0;
	for (auto iter = loaded.beginModel(); iter != loaded.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		if (!node.isModelNode()) {
			continue;
		}
		const int originalIndex = node.name() == "first" ? 0 : 1;
		bool found = false;
		for (int x = node.region().getLowerX(); x <= node.region().getUpperX(); ++x) {
			const voxel::Voxel &voxel = node.volume()->voxel(x, 0, 0);
			if (voxel::isAir(voxel.getMaterial())) {
				continue;
			}
			const color::RGBA rgba = node.palette().color(voxel.getColor());
			EXPECT_TRUE(rgba == palette.color(originalIndex) || rgba == palette.color(255));
			if (rgba == palette.color(originalIndex)) {
				found = true;
				if (originalIndex == 0) {
					EXPECT_FLOAT_EQ(0.75f, node.palette().material(voxel.getColor()).emit);
				}
			}
		}
		EXPECT_TRUE(found) << "Lost original palette color " << originalIndex;
		++checked;
	}
	EXPECT_EQ(2, checked);
	EXPECT_EQ(0, sceneGraph.firstModelNode()->volume()->voxel(0, 0, 0).getColor());
	EXPECT_EQ(palette.color(0), sceneGraph.firstPalette().color(0));
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

static FroxelVoxData makeFroxelFixture() {
	FroxelVoxData data;
	FroxelBone hip;
	hip.name = "hip";
	hip.pivot[0] = 1.0f;
	hip.pivot[1] = 2.0f;
	hip.pivot[2] = 3.0f;
	data.bones.push_back(hip);

	FroxelBone child;
	child.name = "arm";
	child.parent = "hip";
	child.pivot[0] = 4.0f;
	child.pivot[1] = 2.0f;
	child.pivot[2] = 3.0f;
	data.bones.push_back(child);

	FroxelBind bind;
	bind.trn = -1;
	bind.index = 0;
	bind.name = "body";
	bind.bone = "hip";
	data.binds.push_back(bind);

	FroxelClip clip;
	clip.name = "wave";
	FroxelClipTrack track;
	track.bone = "hip";
	FroxelClipKey key;
	key.time = 1.0f;
	key.hasTranslation = true;
	key.translation[0] = 2.0f;
	track.keys.push_back(key);
	clip.tracks.push_back(core::move(track));
	data.clips.push_back(core::move(clip));

	FroxelScript script;
	script.name = "body";
	script.source = "fn main() {}";
	script.autoRegen = true;
	script.hasLocalMin = true;
	script.localMin[0] = 1;
	script.localMin[1] = 2;
	script.localMin[2] = 3;
	FroxelScriptParam param;
	param.name = "speed";
	param.kind = FroxelParamFloat;
	param.fvalue = 0.5f;
	param.fmin = 0.0f;
	param.fmax = 1.0f;
	script.params.push_back(param);
	data.scripts.push_back(core::move(script));
	return data;
}

TEST_F(VoxFormatTest, testFroxelChunkRoundtrip) {
	const FroxelVoxData src = makeFroxelFixture();
	io::BufferedReadWriteStream mvrg;
	io::BufferedReadWriteStream mvan;
	io::BufferedReadWriteStream mvsc;
	ASSERT_TRUE(encodeFroxelMVRG(src, mvrg));
	ASSERT_TRUE(encodeFroxelMVAN(src, mvan));
	ASSERT_TRUE(encodeFroxelMVSC(src, mvsc));

	FroxelVoxData parsed;
	ASSERT_TRUE(parseFroxelMVRG(mvrg.getBuffer(), (uint32_t)mvrg.size(), parsed));
	ASSERT_TRUE(parseFroxelMVAN(mvan.getBuffer(), (uint32_t)mvan.size(), parsed));
	ASSERT_TRUE(parseFroxelMVSC(mvsc.getBuffer(), (uint32_t)mvsc.size(), parsed));

	ASSERT_EQ(2u, parsed.bones.size());
	EXPECT_EQ("hip", parsed.bones[0].name);
	EXPECT_TRUE(parsed.bones[0].parent.empty());
	EXPECT_FLOAT_EQ(1.0f, parsed.bones[0].pivot[0]);
	EXPECT_EQ("arm", parsed.bones[1].name);
	EXPECT_EQ("hip", parsed.bones[1].parent);
	ASSERT_EQ(1u, parsed.binds.size());
	EXPECT_EQ("body", parsed.binds[0].name);
	EXPECT_EQ("hip", parsed.binds[0].bone);
	ASSERT_EQ(1u, parsed.clips.size());
	EXPECT_EQ("wave", parsed.clips[0].name);
	ASSERT_EQ(1u, parsed.clips[0].tracks.size());
	ASSERT_EQ(1u, parsed.clips[0].tracks[0].keys.size());
	EXPECT_TRUE(parsed.clips[0].tracks[0].keys[0].hasTranslation);
	EXPECT_FALSE(parsed.clips[0].tracks[0].keys[0].hasRotation);
	EXPECT_FLOAT_EQ(1.0f, parsed.clips[0].tracks[0].keys[0].time);
	ASSERT_EQ(1u, parsed.scripts.size());
	EXPECT_EQ("fn main() {}", parsed.scripts[0].source);
	EXPECT_TRUE(parsed.scripts[0].autoRegen);
	ASSERT_EQ(1u, parsed.scripts[0].params.size());
	EXPECT_EQ(FroxelParamFloat, parsed.scripts[0].params[0].kind);
	EXPECT_FLOAT_EQ(0.5f, parsed.scripts[0].params[0].fvalue);
}

TEST_F(VoxFormatTest, testFroxelCustomChunksLoad) {
	VoxFormat format;
	scenegraph::SceneGraph saveGraph;
	{
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setName("body");
		node.createVolume(voxel::Region(0, 0, 0, 0, 0, 0));
		node.volume()->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		ASSERT_NE(InvalidNodeId, saveGraph.emplace(core::move(node)));
	}

	const io::MemoryArchivePtr archive = io::openMemoryArchive();
	ASSERT_TRUE(format.save(saveGraph, "base.vox", archive, testSaveCtx));

	core::ScopedPtr<io::SeekableReadStream> in(archive->readStream("base.vox"));
	ASSERT_TRUE(in);
	core::Buffer<uint8_t> bytes;
	bytes.resize((size_t)in->size());
	ASSERT_EQ((int)bytes.size(), in->read(bytes.data(), bytes.size()));

	const FroxelVoxData fixture = makeFroxelFixture();
	struct ExtraChunks {
		io::BufferedReadWriteStream mvrg;
		io::BufferedReadWriteStream mvan;
		io::BufferedReadWriteStream mvsc;
		ogt_vox_custom_chunk_write chunks[3];
	} extra;
	ASSERT_TRUE(encodeFroxelMVRG(fixture, extra.mvrg));
	ASSERT_TRUE(encodeFroxelMVAN(fixture, extra.mvan));
	ASSERT_TRUE(encodeFroxelMVSC(fixture, extra.mvsc));
	extra.chunks[0].id = FroxelChunkMVRG;
	extra.chunks[0].content_size = (uint32_t)extra.mvrg.size();
	extra.chunks[0].content = extra.mvrg.getBuffer();
	extra.chunks[1].id = FroxelChunkMVAN;
	extra.chunks[1].content_size = (uint32_t)extra.mvan.size();
	extra.chunks[1].content = extra.mvan.getBuffer();
	extra.chunks[2].id = FroxelChunkMVSC;
	extra.chunks[2].content_size = (uint32_t)extra.mvsc.size();
	extra.chunks[2].content = extra.mvsc.getBuffer();

	const ogt_vox_scene *scene = ogt_vox_read_scene_with_flags(bytes.data(), (uint32_t)bytes.size(),
															  k_read_scene_flags_groups | k_read_scene_flags_keyframes);
	ASSERT_NE(nullptr, scene);
	ogt_vox_set_write_custom_chunks_func(
		[](const ogt_vox_custom_chunk_write **outChunks, uint32_t *outCount, void *userData) {
			ExtraChunks *chunks = (ExtraChunks *)userData;
			*outChunks = chunks->chunks;
			*outCount = 3;
		},
		&extra);
	uint32_t outSize = 0;
	uint8_t *outBuffer = ogt_vox_write_scene(scene, &outSize);
	ogt_vox_set_write_custom_chunks_func(nullptr, nullptr);
	ogt_vox_destroy_scene(scene);
	ASSERT_NE(nullptr, outBuffer);
	ASSERT_GT(outSize, 0u);
	ASSERT_TRUE(archive->add("froxel.vox", outBuffer, outSize));
	ogt_vox_free(outBuffer);

	scenegraph::SceneGraph loaded;
	ASSERT_TRUE(format.load("froxel.vox", archive, loaded, testLoadCtx));
	const scenegraph::SceneGraphNode *rig = loaded.findNodeByName("FroxelRig");
	ASSERT_NE(nullptr, rig);
	EXPECT_EQ(scenegraph::SceneGraphNodeType::Group, rig->type());
	const scenegraph::SceneGraphNode *hip = loaded.findNodeByName("hip");
	ASSERT_NE(nullptr, hip);
	EXPECT_EQ(scenegraph::SceneGraphNodeType::Point, hip->type());
	const scenegraph::SceneGraphNode *arm = loaded.findNodeByName("arm");
	ASSERT_NE(nullptr, arm);
	EXPECT_EQ(hip->id(), loaded.parentId(*arm));
	const scenegraph::SceneGraphNode *body = loaded.findNodeByName("body");
	ASSERT_NE(nullptr, body);
	EXPECT_EQ("hip", body->property(scenegraph::PropFroxelBone));
	EXPECT_EQ("fn main() {}", body->property(scenegraph::PropFroxelScript));
	EXPECT_EQ("true", body->property(scenegraph::PropFroxelScriptAuto));
	EXPECT_EQ("1,2,3", body->property(scenegraph::PropFroxelScriptLocalMin));
	EXPECT_TRUE(loaded.hasAnimation("wave"));
}

TEST_F(VoxFormatTest, testTeardownTDCZRoundtrip) {
	const uint32_t sizeX = 2;
	const uint32_t sizeY = 2;
	const uint32_t sizeZ = 2;
	const uint8_t voxels[8] = {1, 0, 0, 0, 0, 0, 0, 2};
	io::BufferedReadWriteStream encoded;
	ASSERT_TRUE(encodeTeardownTDCZ(sizeX, sizeY, sizeZ, voxels, encoded));
	core::Buffer<uint8_t> decoded;
	uint32_t outX = 0;
	uint32_t outY = 0;
	uint32_t outZ = 0;
	ASSERT_TRUE(parseTeardownTDCZ(encoded.getBuffer(), (uint32_t)encoded.size(), decoded, outX, outY, outZ));
	EXPECT_EQ(sizeX, outX);
	EXPECT_EQ(sizeY, outY);
	EXPECT_EQ(sizeZ, outZ);
	ASSERT_EQ(8u, decoded.size());
	EXPECT_EQ(1, decoded[0]);
	EXPECT_EQ(2, decoded[7]);
}

TEST_F(VoxFormatTest, testTeardownTDCZCustomChunk) {
	const uint32_t sizeX = 2;
	const uint32_t sizeY = 1;
	const uint32_t sizeZ = 1;
	uint8_t voxels[2] = {1, 0};
	io::BufferedReadWriteStream tdcz;
	ASSERT_TRUE(encodeTeardownTDCZ(sizeX, sizeY, sizeZ, voxels, tdcz));

	io::BufferedReadWriteStream vox;
	ASSERT_TRUE(vox.writeUInt32(OGT_VOX_MAKE_CHUNK_ID('V', 'O', 'X', ' ')));
	ASSERT_TRUE(vox.writeUInt32(150));
	ASSERT_TRUE(vox.writeUInt32(OGT_VOX_MAKE_CHUNK_ID('M', 'A', 'I', 'N')));
	ASSERT_TRUE(vox.writeUInt32(0));
	ASSERT_TRUE(vox.writeUInt32(12 + (uint32_t)tdcz.size()));
	ASSERT_TRUE(vox.writeUInt32(TeardownChunkTDCZ));
	ASSERT_TRUE(vox.writeUInt32((uint32_t)tdcz.size()));
	ASSERT_TRUE(vox.writeUInt32(0));
	ASSERT_EQ((int)tdcz.size(), vox.write(tdcz.getBuffer(), tdcz.size()));

	const io::MemoryArchivePtr archive = io::openMemoryArchive();
	ASSERT_TRUE(archive->add("tdcz.vox", vox.getBuffer(), (size_t)vox.size()));
	VoxFormat format;
	scenegraph::SceneGraph graph;
	ASSERT_TRUE(format.load("tdcz.vox", archive, graph, testLoadCtx));
	ASSERT_EQ(1u, graph.size(scenegraph::SceneGraphNodeType::AllModels));
	const scenegraph::SceneGraphNode *node = graph.firstModelNode();
	ASSERT_NE(nullptr, node);
	EXPECT_EQ(1, voxelutil::countVoxels(*node->volume()));
}

} // namespace voxelformat
