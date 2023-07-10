/**
 * @file
 */

#include "voxelformat/private/magicavoxel/VoxFormat.h"
#include "AbstractVoxFormatTest.h"
#include "io/BufferedReadWriteStream.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "voxel/MaterialColor.h"
#include "voxel/Palette.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelformat/VolumeFormat.h"
#include "vox_character.h"
#include "vox_glasses.h"
#include "8ontop.h"

namespace voxelformat {

class VoxFormatTest : public AbstractVoxFormatTest {};

// TODO: add a test to check the group handling scene graph layout in general.

TEST_F(VoxFormatTest, testLoad) {
	canLoad("magicavoxel.vox");
}

TEST_F(VoxFormatTest, testLoadCharacter) {
	VoxFormat f;
	const io::FilePtr &file = open("vox_character.vox");
	ASSERT_TRUE(file->validHandle());
	io::FileStream stream(file);
	scenegraph::SceneGraph sceneGraph;
	io::FileDescription fileDesc;
	fileDesc.set(file->name());
	ASSERT_TRUE(voxelformat::loadFormat(fileDesc, stream, sceneGraph, testLoadCtx));
	// dump(file->fileName(), sceneGraph);
	core::SharedPtr<voxel::RawVolume> volumes[] = {
		character_0::create(),	character_1::create(),	character_2::create(),	character_3::create(),
		character_4::create(),	character_5::create(),	character_6::create(),	character_7::create(),
		character_8::create(),	character_9::create(),	character_10::create(), character_11::create(),
		character_12::create(), character_13::create(), character_14::create(), character_15::create()};
	ASSERT_EQ(lengthof(volumes), (int)sceneGraph.size());
	auto iter = sceneGraph.beginModel();
	for (int i = 0; i < lengthof(volumes); ++i, ++iter) {
		const voxel::RawVolume &v1 = *volumes[i].get();
		const voxel::RawVolume &v2 = *(*iter).volume();
		volumeComparator(v1, voxel::getPalette(), v2, (*iter).palette(), voxel::ValidateFlags::All, 0.01f);
		EXPECT_EQ(v1.region().getLowerCornerf(), (*iter).transform(0).worldTranslation());
	}
}

TEST_F(VoxFormatTest, testLoadGlasses) {
	VoxFormat f;
	const io::FilePtr &file = open("vox_glasses.vox");
	ASSERT_TRUE(file->validHandle());
	io::FileStream stream(file);
	scenegraph::SceneGraph sceneGraph;
	io::FileDescription fileDesc;
	fileDesc.set(file->name());
	ASSERT_TRUE(voxelformat::loadFormat(fileDesc, stream, sceneGraph, testLoadCtx));
	ASSERT_EQ(1u, sceneGraph.size());
	// dump(file->fileName(), sceneGraph);
	core::SharedPtr<voxel::RawVolume> volumes[] = {glasses_0::create()};
	ASSERT_EQ(lengthof(volumes), (int)sceneGraph.size());
	auto iter = sceneGraph.beginModel();
	for (int i = 0; i < lengthof(volumes); ++i, ++iter) {
		const voxel::RawVolume &v1 = *volumes[i].get();
		const voxel::RawVolume &v2 = *(*iter).volume();
		volumeComparator(v1, voxel::getPalette(), v2, (*iter).palette(), voxel::ValidateFlags::All, 0.011f);
		EXPECT_EQ(v1.region().getLowerCornerf(), (*iter).transform(0).worldTranslation());
	}
}

TEST_F(VoxFormatTest, testLoad8OnTop) {
	VoxFormat f;
	const io::FilePtr &file = open("8ontop.vox");
	ASSERT_TRUE(file->validHandle());
	io::FileStream stream(file);
	scenegraph::SceneGraph sceneGraph;
	io::FileDescription fileDesc;
	fileDesc.set(file->name());
	ASSERT_TRUE(voxelformat::loadFormat(fileDesc, stream, sceneGraph, testLoadCtx));
	ASSERT_EQ(72u, sceneGraph.size());
	// dump(file->fileName(), sceneGraph);
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
	ASSERT_EQ(lengthof(volumes), (int)sceneGraph.size());
	auto iter = sceneGraph.beginModel();
	for (int i = 0; i < lengthof(volumes); ++i, ++iter) {
		const voxel::RawVolume &v1 = *volumes[i].get();
		const voxel::RawVolume &v2 = *(*iter).volume();
		volumeComparator(v1, voxel::getPalette(), v2, (*iter).palette(), voxel::ValidateFlags::All, 0.02f);
		EXPECT_EQ(v1.region().getLowerCornerf(), (*iter).transform(0).worldTranslation());
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
	testSaveLoadVoxel("mv-smallvolumesavetest.vox", &f);
}

TEST_F(VoxFormatTest, testSaveMultipleLayers) {
	VoxFormat f;
	testSaveMultipleModels("mv-multiplemodelsavetest.vox", &f);
}

TEST_F(VoxFormatTest, testSaveBigVolume) {
	VoxFormat f;
	const voxel::Region region(glm::ivec3(0), glm::ivec3(1023, 0, 0));
	voxel::RawVolume bigVolume(region);
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	bigVolume.setVoxel(0, 0, 0, voxel);
	bigVolume.setVoxel(256, 0, 0, voxel);
	bigVolume.setVoxel(512, 0, 0, voxel);
	const core::String name = "bigvolume.vox";
	scenegraph::SceneGraph sceneGraph;
	scenegraph::SceneGraph sceneGraphsave(2);
	{
		scenegraph::SceneGraphNode node;
		node.setVolume(&bigVolume, false);
		sceneGraphsave.emplace(core::move(node));
	}

#define VOX_TEST_SAVE_TO_FILE 0
#if VOX_TEST_SAVE_TO_FILE
	const io::FilePtr &filesave = open(name, io::FileMode::SysWrite);
	io::FileStream stream(filesave);
	const io::FilePtr &fileLoadAfterSave = open(name);
	io::FileStream streamread(fileLoadAfterSave.get());
	f.load(name, streamread, sceneGraph, testLoadCtx);
#else
	io::BufferedReadWriteStream stream(10 * 1024 * 1024);

	ASSERT_TRUE(f.save(sceneGraphsave, name, stream, testSaveCtx));
	stream.seek(0);
	f.load(name, stream, sceneGraph, testLoadCtx);
#endif
	EXPECT_EQ(3, (int)sceneGraph.size());
}

TEST_F(VoxFormatTest, testSave) {
	VoxFormat f;
	testLoadSaveAndLoad("magicavoxel.vox", f, "magicavoxel-save.vox", f, voxel::ValidateFlags::All);
}

} // namespace voxel
