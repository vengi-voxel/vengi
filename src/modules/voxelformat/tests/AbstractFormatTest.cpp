/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "app/App.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "image/Image.h"
#include "io/Archive.h"
#include "io/BufferedReadWriteStream.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "io/FilesystemArchive.h"
#include "io/FormatDescription.h"
#include "io/MemoryArchive.h"
#include "io/Stream.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphTransform.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelformat/Format.h"
#include "voxelformat/FormatConfig.h"
#include "voxelformat/FormatThumbnail.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/tests/TestHelper.h"
#include "voxelrender/ImageGenerator.h"
#include "voxelutil/VolumeVisitor.h"

#define WRITE_TO_FILE 1

namespace voxelformat {

const voxel::Voxel AbstractFormatTest::Empty;

io::ArchivePtr AbstractFormatTest::helper_archive(const core::String &filename) {
#if WRITE_TO_FILE
	return io::openFilesystemArchive(_testApp->filesystem());
#else
	return io::openMemoryArchive();
#endif
}

io::ArchivePtr AbstractFormatTest::helper_filesystemarchive() {
	return io::openFilesystemArchive(_testApp->filesystem());
}

image::ImagePtr AbstractFormatTest::helper_testThumbnailCreator(const scenegraph::SceneGraph &sceneGraph,
																const ThumbnailContext &ctx) {
	return image::ImagePtr();
}

void AbstractFormatTest::testFirstAndLastPaletteIndex(const core::String &filename, Format *format,
													  voxel::ValidateFlags flags) {
	SCOPED_TRACE(filename.c_str());
	voxel::Region region(glm::ivec3(0), glm::ivec3(1));
	voxel::RawVolume volume(region);
	EXPECT_TRUE(volume.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 0)));
	EXPECT_TRUE(volume.setVoxel(0, 0, 1, voxel::createVoxel(voxel::VoxelType::Generic, 255)));
	const io::ArchivePtr &archive = helper_archive();
	scenegraph::SceneGraph sceneGraphsave;
	{
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setVolume(&volume, false);
		sceneGraphsave.emplace(core::move(node));
	}
	ASSERT_TRUE(format->save(sceneGraphsave, filename, archive, testSaveCtx));

	scenegraph::SceneGraph sceneGraphLoad;
	ASSERT_TRUE(format->load(filename, archive, sceneGraphLoad, testLoadCtx));
	voxel::sceneGraphComparator(sceneGraphsave, sceneGraphLoad, flags, 0.001f);
}

void AbstractFormatTest::testTransform(const core::String &filename) {
	SCOPED_TRACE(filename.c_str());
	scenegraph::SceneGraph sceneGraph;
	io::FileDescription fileDesc;
	fileDesc.set(filename);
	const io::ArchivePtr &archive = helper_filesystemarchive();
	ASSERT_TRUE(voxelformat::loadFormat(fileDesc, archive, sceneGraph, testLoadCtx))
		<< "Failed to load " << filename.c_str();
	EXPECT_EQ(20u, sceneGraph.size()) << "Unexpected scene graph size for " << filename.c_str();
	scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	EXPECT_EQ("original", node->name());
	EXPECT_EQ(node->region().getLowerCorner(), glm::ivec3(0, 0, 0));
	EXPECT_EQ(node->region().getUpperCorner(), glm::ivec3(39, 29, 39));
	EXPECT_TRUE(voxel::isAir(node->volume()->voxel(0, 0, 0).getMaterial())) << *node->volume();
	EXPECT_FALSE(voxel::isAir(node->volume()->voxel(0, 20, 0).getMaterial())) << *node->volume();
	const scenegraph::SceneGraphTransform &transform = node->transform();
	EXPECT_EQ(23.0f, transform.worldTranslation().x);
	EXPECT_EQ(-2.0f, transform.worldTranslation().y);
	EXPECT_EQ(23.0f, transform.worldTranslation().z);
}

void AbstractFormatTest::testFirstAndLastPaletteIndexConversion(Format &srcFormat, const core::String &srcFilename,
																Format &destFormat, const core::String &destFilename,
																voxel::ValidateFlags flags) {
	SCOPED_TRACE(destFilename.c_str());
	voxel::Region region(glm::ivec3(0), glm::ivec3(1));
	voxel::RawVolume original(region);
	const palette::Palette pal1 = voxel::getPalette();
	EXPECT_TRUE(original.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 0u)));
	EXPECT_TRUE(original.setVoxel(0, 0, 1, voxel::createVoxel(voxel::VoxelType::Generic, 255u)));
	const io::ArchivePtr &archive = helper_archive();
	scenegraph::SceneGraph sceneGraphsave1	;
	{
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setVolume(&original, false);
		node.setPalette(pal1);
		sceneGraphsave1.emplace(core::move(node));
		EXPECT_TRUE(srcFormat.save(sceneGraphsave1, srcFilename, archive, testSaveCtx))
			<< "Could not save " << srcFilename;
	}

	scenegraph::SceneGraph sceneGraphLoad1;
	ASSERT_TRUE(srcFormat.load(srcFilename, archive, sceneGraphLoad1, testLoadCtx));
	voxel::sceneGraphComparator(sceneGraphsave1, sceneGraphLoad1, flags, 0.001f);

	io::BufferedReadWriteStream stream((int64_t)(10 * 1024 * 1024));
	{
		EXPECT_TRUE(destFormat.save(sceneGraphLoad1, destFilename, archive, testSaveCtx))
			<< "Could not save " << destFilename;
	}

	scenegraph::SceneGraph sceneGraphLoad;
	ASSERT_TRUE(destFormat.load(destFilename, archive, sceneGraphLoad, testLoadCtx));
	voxel::sceneGraphComparator(sceneGraphsave1, sceneGraphLoad, flags, 0.001f);
}

void AbstractFormatTest::testMaterial(scenegraph::SceneGraph &sceneGraph, const core::String &filename) {
	const io::ArchivePtr &archive = helper_filesystemarchive();
	ASSERT_TRUE(archive->exists("test_material.vox"));
	SCOPED_TRACE(filename.c_str());
	scenegraph::SceneGraph voxSceneGraph;
	{
		io::FileDescription fileDesc;
		fileDesc.set("test_material.vox");
		ASSERT_TRUE(voxelformat::loadFormat(fileDesc, archive, voxSceneGraph, testLoadCtx));
		EXPECT_EQ(12u, voxSceneGraph.size());
	}
	ASSERT_TRUE(voxelformat::saveFormat(voxSceneGraph, filename, nullptr, archive, testSaveCtx));
	{
		io::FileDescription fileDesc;
		fileDesc.set(filename);
		ASSERT_TRUE(voxelformat::loadFormat(fileDesc, archive, sceneGraph, testLoadCtx));
		EXPECT_EQ(12u, sceneGraph.size());
	}
	voxel::materialComparator(voxSceneGraph, sceneGraph);
}

void AbstractFormatTest::testLoad(scenegraph::SceneGraph &sceneGraph, const core::String &filename,
								  size_t expectedVolumes) {
	const io::ArchivePtr &archive = helper_filesystemarchive();
	if (!archive->exists(filename)) {
		GTEST_SKIP() << "Could not open " << filename;
	} else {
		SCOPED_TRACE(filename.c_str());
		io::FileDescription fileDesc;
		fileDesc.set(filename);
		if (expectedVolumes <= 0) {
			ASSERT_FALSE(voxelformat::loadFormat(fileDesc, archive, sceneGraph, testLoadCtx))
				<< "Unexpected success to load " << filename;
		} else {
			ASSERT_TRUE(voxelformat::loadFormat(fileDesc, archive, sceneGraph, testLoadCtx))
				<< "Could not load " << filename;
			ASSERT_EQ(expectedVolumes, sceneGraph.size());
		}
	}
}

void AbstractFormatTest::checkColor(core::RGBA c1, const palette::Palette &palette, uint8_t index, float maxDelta) {
	const core::RGBA c2 = palette.color(index);
	const float delta = core::Color::getDistance(c1, c2, core::Color::Distance::HSB);
	ASSERT_LE(delta, maxDelta) << "color1[" << core::Color::print(c1) << "], color2[" << core::Color::print(c2)
							   << "], delta[" << delta << "]";
}

void AbstractFormatTest::testRGBSmall(const core::String &filename, const io::ArchivePtr &archive,
									  scenegraph::SceneGraph &sceneGraph) {
	SCOPED_TRACE(filename.c_str());
	io::FileDescription fileDesc;
	fileDesc.set(filename);
	ASSERT_TRUE(voxelformat::loadFormat(fileDesc, archive, sceneGraph, testLoadCtx));
	EXPECT_EQ(1u, sceneGraph.size());

	palette::Palette palette;
	EXPECT_TRUE(palette.nippon());

	const core::RGBA red(255, 0, 0);
	const core::RGBA green(0, 255, 0);
	const core::RGBA blue(0, 0, 255);

	for (auto iter = sceneGraph.beginModel(); iter != sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		const voxel::RawVolume *volume = node.volume();
		EXPECT_EQ(3, voxelutil::visitVolumeParallel(*volume, [](int, int, int, const voxel::Voxel &) {}));
		checkColor(blue, node.palette(), volume->voxel(0, 0, 0).getColor(), 0.0f);
		checkColor(green, node.palette(), volume->voxel(1, 0, 0).getColor(), 0.0f);
		checkColor(red, node.palette(), volume->voxel(2, 0, 0).getColor(), 0.0f);
	}
}

void AbstractFormatTest::testRGBSmall(const core::String &filename) {
	const io::ArchivePtr &archive = helper_filesystemarchive();
	scenegraph::SceneGraph sceneGraph;
	testRGBSmall(filename, archive, sceneGraph);
}

void AbstractFormatTest::testRGBSmallSaveLoad(const core::String &filename) {
	const core::String formatExt = core::string::extractExtension(filename);
	const core::String saveFilename = "test." + formatExt;
	testRGBSmallSaveLoad(filename, saveFilename);
}

void AbstractFormatTest::testLoadScreenshot(const core::String &filename, int width, int height,
											const core::RGBA expectedColor, int expectedX, int expectedY) {
	SCOPED_TRACE(filename.c_str());
	const image::ImagePtr &image = voxelformat::loadScreenshot(filename, helper_filesystemarchive(), testLoadCtx);
	ASSERT_TRUE(image);
	ASSERT_EQ(image->width(), width) << image::print(image);
	ASSERT_EQ(image->height(), height) << image::print(image);
	const core::RGBA color = image->colorAt(expectedX, expectedY);
	ASSERT_EQ(color, expectedColor) << "expected " << core::Color::print(expectedColor) << " but got "
									<< core::Color::print(color) << "at " << expectedX << ":" << expectedY << "\n"
									<< image::print(image);
}

void AbstractFormatTest::testRGBSmallSaveLoad(const core::String &filename, const core::String &saveFilename) {
	SCOPED_TRACE(filename.c_str());
	scenegraph::SceneGraph sceneGraph;
	io::ArchivePtr archive = helper_filesystemarchive();
	testRGBSmall(filename, archive, sceneGraph);

	ASSERT_TRUE(voxelformat::saveFormat(sceneGraph, saveFilename, nullptr, archive, testSaveCtx));

	scenegraph::SceneGraph loadSceneGraph;
	testRGBSmall(saveFilename, archive, loadSceneGraph);
}

bool AbstractFormatTest::helper_saveSceneGraph(scenegraph::SceneGraph &sceneGraph, const core::String &filename) {
	const io::ArchivePtr &archive = helper_filesystemarchive();
	voxelformat::SaveContext saveCtx;
	return voxelformat::saveFormat(sceneGraph, filename, nullptr, archive, saveCtx);
}

void AbstractFormatTest::testRGB(const core::String &filename, float maxDelta) {
	SCOPED_TRACE(filename.c_str());
	scenegraph::SceneGraph sceneGraph;
	io::FileDescription fileDesc;
	fileDesc.set(filename);
	const io::ArchivePtr &archive = helper_filesystemarchive();
	ASSERT_TRUE(voxelformat::loadFormat(fileDesc, archive, sceneGraph, testLoadCtx))
		<< "Failed to load " << filename.c_str();
	EXPECT_EQ(1u, sceneGraph.size()) << "Unexpected scene graph size for " << filename.c_str();

	palette::Palette palette;
	EXPECT_TRUE(palette.nippon());

	const core::RGBA red = palette.color(37);
	const core::RGBA green = palette.color(149);
	const core::RGBA blue = palette.color(197);

	for (auto iter = sceneGraph.beginModel(); iter != sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		const voxel::RawVolume *volume = node.volume();
		EXPECT_EQ(99, voxelutil::visitVolumeParallel(*volume, [](int, int, int, const voxel::Voxel &) {}));
		EXPECT_EQ(voxel::VoxelType::Generic, volume->voxel(0, 0, 0).getMaterial())
			<< "Failed rgb check for " << filename.c_str();
		EXPECT_EQ(voxel::VoxelType::Generic, volume->voxel(31, 0, 0).getMaterial())
			<< "Failed rgb check for " << filename.c_str();
		EXPECT_EQ(voxel::VoxelType::Generic, volume->voxel(31, 0, 31).getMaterial())
			<< "Failed rgb check for " << filename.c_str();
		EXPECT_EQ(voxel::VoxelType::Generic, volume->voxel(0, 0, 31).getMaterial())
			<< "Failed rgb check for " << filename.c_str();

		EXPECT_EQ(voxel::VoxelType::Generic, volume->voxel(0, 31, 0).getMaterial())
			<< "Failed rgb check for " << filename.c_str();
		EXPECT_EQ(voxel::VoxelType::Generic, volume->voxel(31, 31, 0).getMaterial())
			<< "Failed rgb check for " << filename.c_str();
		EXPECT_EQ(voxel::VoxelType::Generic, volume->voxel(31, 31, 31).getMaterial())
			<< "Failed rgb check for " << filename.c_str();
		EXPECT_EQ(voxel::VoxelType::Generic, volume->voxel(0, 31, 31).getMaterial())
			<< "Failed rgb check for " << filename.c_str();

		EXPECT_EQ(voxel::VoxelType::Generic, volume->voxel(9, 0, 4).getMaterial())
			<< "Failed rgb check for " << filename.c_str();
		EXPECT_EQ(voxel::VoxelType::Generic, volume->voxel(9, 0, 12).getMaterial())
			<< "Failed rgb check for " << filename.c_str();
		EXPECT_EQ(voxel::VoxelType::Generic, volume->voxel(9, 0, 19).getMaterial())
			<< "Failed rgb check for " << filename.c_str();

		checkColor(red, node.palette(), volume->voxel(9, 0, 4).getColor(), maxDelta);
		checkColor(green, node.palette(), volume->voxel(9, 0, 12).getColor(), maxDelta);
		checkColor(blue, node.palette(), volume->voxel(9, 0, 19).getColor(), maxDelta);
	}
}

void AbstractFormatTest::testConvert(const core::String &srcFilename, Format &srcFormat,
									 const core::String &destFilename, Format &destFormat, voxel::ValidateFlags flags,
									 float maxDelta) {
	SCOPED_TRACE("src: " + srcFilename);
	io::ArchivePtr archive = helper_filesystemarchive();
	scenegraph::SceneGraph sceneGraph;
	ASSERT_TRUE(helper_loadIntoSceneGraph(srcFilename, archive, srcFormat, sceneGraph))
		<< "Failed to load " << srcFilename;

	SCOPED_TRACE("target: " + destFilename);

	ASSERT_TRUE(destFormat.save(sceneGraph, destFilename, archive, testSaveCtx)) << "Could not save " << destFilename;

	scenegraph::SceneGraph sceneGraphLoad;
	ASSERT_TRUE(destFormat.load(destFilename, archive, sceneGraphLoad, testLoadCtx));
	voxel::sceneGraphComparator(sceneGraph, sceneGraphLoad, flags, maxDelta);
}

void AbstractFormatTest::testConvertSceneGraph(const core::String &srcFilename1, Format &srcFormat1,
											   const core::String &srcFilename2, Format &srcFormat2,
											   voxel::ValidateFlags flags, float maxDelta) {
	SCOPED_TRACE("src1: " + srcFilename1);
	scenegraph::SceneGraph srcSceneGraph1;
	io::ArchivePtr archive = helper_filesystemarchive();
	ASSERT_TRUE(helper_loadIntoSceneGraph(srcFilename1, archive, srcFormat1, srcSceneGraph1))
		<< "Failed to load " << srcFilename1;
	SCOPED_TRACE("src2: " + srcFilename2);
	scenegraph::SceneGraph srcSceneGraph2;
	ASSERT_TRUE(helper_loadIntoSceneGraph(srcFilename2, archive, srcFormat2, srcSceneGraph2))
		<< "Failed to load " << srcFilename2;
	voxel::sceneGraphComparator(srcSceneGraph1, srcSceneGraph2, flags, maxDelta);
}

void AbstractFormatTest::testLoadSaveAndLoadSceneGraph(const core::String &srcFilename, Format &srcFormat,
													   const core::String &destFilename, Format &destFormat,
													   voxel::ValidateFlags flags, float maxDelta) {
	SCOPED_TRACE("src: " + srcFilename);
	SCOPED_TRACE("target: " + destFilename);
	const io::ArchivePtr &archive = helper_filesystemarchive();
	scenegraph::SceneGraph srcSceneGraph;
	ASSERT_TRUE(helper_loadIntoSceneGraph(srcFilename, archive, srcFormat, srcSceneGraph))
		<< "Failed to load " << srcFilename;
	ASSERT_TRUE(destFormat.save(srcSceneGraph, destFilename, archive, testSaveCtx))
		<< "Could not save " << destFilename;
	scenegraph::SceneGraph destSceneGraph;
	ASSERT_TRUE(destFormat.load(destFilename, archive, destSceneGraph, testLoadCtx))
		<< "Failed to load the target format";
	voxel::sceneGraphComparator(srcSceneGraph, destSceneGraph, flags, maxDelta);
}

void AbstractFormatTest::testSaveSingleVoxel(const core::String &filename, Format *format, voxel::ValidateFlags flags) {
	SCOPED_TRACE(filename.c_str());
	voxel::Region region(glm::ivec3(0), glm::ivec3(0));
	voxel::RawVolume original(region);
	original.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	scenegraph::SceneGraph sceneGraphsave;
	{
		palette::Palette pal;
		pal.tryAdd(core::RGBA(127, 127, 255, 255));
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setVolume(&original, false);
		node.setPalette(pal);
		sceneGraphsave.emplace(core::move(node));
	}
	io::ArchivePtr archive = helper_archive();
	ASSERT_TRUE(format->save(sceneGraphsave, filename, archive, testSaveCtx));

	scenegraph::SceneGraph sceneGraph;
	ASSERT_TRUE(format->load(filename, archive, sceneGraph, testLoadCtx));

	voxel::sceneGraphComparator(sceneGraph, sceneGraphsave, flags);
}

void AbstractFormatTest::testSaveSmallVolume(const core::String &filename, Format *format, voxel::ValidateFlags flags) {
	SCOPED_TRACE(filename.c_str());
	palette::Palette pal;
	pal.magicaVoxel();
	voxel::Region region(glm::ivec3(0), glm::ivec3(0, 1, 1));
	voxel::RawVolume original(region);
	ASSERT_TRUE(original.setVoxel(0, 0, 1, voxel::createVoxel(pal, 200)));
	ASSERT_TRUE(original.setVoxel(0, 1, 1, voxel::createVoxel(pal, 201)));
	ASSERT_TRUE(original.setVoxel(0, 0, 0, voxel::createVoxel(pal, pal.colorCount() - 1)));
	scenegraph::SceneGraph sceneGraphsave;
	int modelNodeId;
	{
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setVolume(&original, false);
		node.setPalette(pal);
		modelNodeId = sceneGraphsave.emplace(core::move(node));
	}
	if (!format->singleVolume()) {
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::ModelReference);
		node.setReference(modelNodeId);
		node.setPalette(pal);
		scenegraph::KeyFrameIndex keyFrameIdx = 0;
		scenegraph::SceneGraphTransform transform;
		transform.setWorldTranslation(glm::ivec3(2, 0, 0));
		node.setTransform(keyFrameIdx, transform);
		ASSERT_NE(InvalidNodeId, sceneGraphsave.emplace(core::move(node)));
	}
	io::ArchivePtr archive = helper_archive();
	ASSERT_TRUE(format->save(sceneGraphsave, filename, archive, testSaveCtx));

	scenegraph::SceneGraph sceneGraph;
	ASSERT_TRUE(format->load(filename, archive, sceneGraph, testLoadCtx));

	voxel::sceneGraphComparator(sceneGraph, sceneGraphsave, flags);
}

void AbstractFormatTest::testSaveMultipleModels(const core::String &filename, Format *format,
												voxel::ValidateFlags flags) {
	SCOPED_TRACE(filename.c_str());
	voxel::Region region(glm::ivec3(0), glm::ivec3(0));
	palette::Palette pal;
	pal.tryAdd(core::RGBA(127, 127, 255, 255));
	voxel::RawVolume model1(region);
	voxel::RawVolume model2(region);
	voxel::RawVolume model3(region);
	voxel::RawVolume model4(region);
	EXPECT_TRUE(model1.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 0)));
	EXPECT_TRUE(model2.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 0)));
	EXPECT_TRUE(model3.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 0)));
	EXPECT_TRUE(model4.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 0)));
	scenegraph::SceneGraph sceneGraph;
	scenegraph::SceneGraphNode node1(scenegraph::SceneGraphNodeType::Model);
	node1.setVolume(&model1, false);
	node1.setPalette(pal);
	scenegraph::SceneGraphNode node2(scenegraph::SceneGraphNodeType::Model);
	node2.setVolume(&model2, false);
	node2.setPalette(pal);
	scenegraph::SceneGraphNode node3(scenegraph::SceneGraphNodeType::Model);
	node3.setVolume(&model3, false);
	node3.setPalette(pal);
	scenegraph::SceneGraphNode node4(scenegraph::SceneGraphNodeType::Model);
	node4.setVolume(&model4, false);
	node4.setPalette(pal);
	scenegraph::SceneGraphNode node5(scenegraph::SceneGraphNodeType::Group);
	sceneGraph.emplace(core::move(node1));
	sceneGraph.emplace(core::move(node2));
	sceneGraph.emplace(core::move(node3));
	sceneGraph.emplace(core::move(node4));
	sceneGraph.emplace(core::move(node5));

	io::ArchivePtr archive = helper_archive();
	ASSERT_TRUE(format->save(sceneGraph, filename, archive, testSaveCtx));
	scenegraph::SceneGraph sceneGraphLoad;
	EXPECT_TRUE(format->load(filename, archive, sceneGraphLoad, testLoadCtx));
	voxel::sceneGraphComparator(sceneGraph, sceneGraphLoad, flags);
}

void AbstractFormatTest::testSave(const core::String &filename, Format *format, const palette::Palette &palette,
								  voxel::ValidateFlags flags) {
	SCOPED_TRACE(filename.c_str());
	voxel::Region region(glm::ivec3(0), glm::ivec3(0));
	voxel::RawVolume model1(region);
	EXPECT_TRUE(model1.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
	scenegraph::SceneGraph sceneGraph;
	scenegraph::SceneGraphNode node1(scenegraph::SceneGraphNodeType::Model);
	node1.setPalette(palette);
	node1.setVolume(&model1, false);
	sceneGraph.emplace(core::move(node1));
	io::ArchivePtr archive = helper_archive();
	ASSERT_TRUE(format->save(sceneGraph, filename, archive, testSaveCtx));

	scenegraph::SceneGraph sceneGraphLoad;
	EXPECT_TRUE(format->load(filename, archive, sceneGraphLoad, testLoadCtx));
	voxel::sceneGraphComparator(sceneGraph, sceneGraphLoad, flags);
}

void AbstractFormatTest::testSaveLoadVoxel(const core::String &filename, Format *format, int mins, int maxs,
										   voxel::ValidateFlags flags, float maxDelta) {
	SCOPED_TRACE(filename.c_str());
	const voxel::Region region(mins, maxs);
	voxel::RawVolume original(region);

	original.setVoxel(mins, mins, mins, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	original.setVoxel(mins, mins, maxs, voxel::createVoxel(voxel::VoxelType::Generic, 244));
	original.setVoxel(mins, maxs, maxs, voxel::createVoxel(voxel::VoxelType::Generic, 126));
	original.setVoxel(mins, maxs, mins, voxel::createVoxel(voxel::VoxelType::Generic, 254));

	original.setVoxel(maxs, maxs, maxs, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	original.setVoxel(maxs, maxs, mins, voxel::createVoxel(voxel::VoxelType::Generic, 245));
	original.setVoxel(maxs, mins, mins, voxel::createVoxel(voxel::VoxelType::Generic, 127));
	original.setVoxel(maxs, mins, maxs, voxel::createVoxel(voxel::VoxelType::Generic, 200));

	testSaveLoadVolumes(filename, original, format, flags, maxDelta);
}

void AbstractFormatTest::testSaveLoadCube(const core::String &filename, Format *format, voxel::ValidateFlags flags,
										  float maxDelta) {
	SCOPED_TRACE(filename.c_str());
	glm::ivec3 mins(0, 0, 0);
	glm::ivec3 maxs(9, 9, 9);
	voxel::Region region(mins, maxs);
	voxel::RawVolume original(region);
	original.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	original.setVoxel(1, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	original.setVoxel(8, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	original.setVoxel(9, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	original.setVoxel(0, 1, 0, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	original.setVoxel(9, 1, 0, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	original.setVoxel(0, 8, 0, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	original.setVoxel(9, 8, 0, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	original.setVoxel(0, 9, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	original.setVoxel(1, 9, 0, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	original.setVoxel(8, 9, 0, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	original.setVoxel(9, 9, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	original.setVoxel(0, 0, 1, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	original.setVoxel(9, 0, 1, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	original.setVoxel(0, 9, 1, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	original.setVoxel(9, 9, 1, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	original.setVoxel(0, 0, 8, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	original.setVoxel(9, 0, 8, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	original.setVoxel(0, 9, 8, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	original.setVoxel(9, 9, 8, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	original.setVoxel(0, 0, 9, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	original.setVoxel(1, 0, 9, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	original.setVoxel(8, 0, 9, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	original.setVoxel(9, 0, 9, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	original.setVoxel(0, 1, 9, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	original.setVoxel(9, 1, 9, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	original.setVoxel(0, 8, 9, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	original.setVoxel(9, 8, 9, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	original.setVoxel(0, 9, 9, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	original.setVoxel(1, 9, 9, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	original.setVoxel(8, 9, 9, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	original.setVoxel(9, 9, 9, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	testSaveLoadVolumes(filename, original, format, flags, maxDelta);
}

void AbstractFormatTest::testSaveLoadVolumes(const core::String &filename, const voxel::RawVolume &original,
											 Format *format, voxel::ValidateFlags flags, float maxDelta) {
	palette::Palette pal;
	pal.magicaVoxel();

	scenegraph::SceneGraph sceneGraph;
	int nodeId = 0;
	{
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setName("first level #1");
		node.setVolume(&original);
		node.setPalette(pal);
		nodeId = sceneGraph.emplace(core::move(node), nodeId);
	}
	if (!format->singleVolume()) {
		{
			scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
			node.setName("second level #1");
			node.setVolume(&original);
			node.setPalette(pal);
			sceneGraph.emplace(core::move(node), nodeId);
		}
		{
			scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
			node.setName("second level #2");
			node.setVolume(&original);
			node.setPalette(pal);
			sceneGraph.emplace(core::move(node), nodeId);
		}
		{
			scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::ModelReference);
			node.setName("reference node");
			node.setPalette(pal);
			node.setReference(nodeId);
			sceneGraph.emplace(core::move(node));
		}
	}
	io::ArchivePtr archive = helper_archive();
	ASSERT_TRUE(format->save(sceneGraph, filename, archive, testSaveCtx)) << "Could not save the scene graph";

	scenegraph::SceneGraph sceneGraphLoad;
	ASSERT_TRUE(format->load(filename, archive, sceneGraphLoad, testLoadCtx)) << "Failed to load the scene grpah";
	voxel::sceneGraphComparator(sceneGraph, sceneGraphLoad, flags, maxDelta);
}

#undef WRITE_TO_FILE

io::FilePtr AbstractFormatTest::open(const core::String &filename, io::FileMode mode) {
	const io::FilePtr &file = _testApp->filesystem()->open(core::String(filename), mode);
	return file;
}

bool AbstractFormatTest::helper_loadIntoSceneGraph(const core::String &filename, const io::ArchivePtr &archive,
												   Format &format, scenegraph::SceneGraph &sceneGraph) {
	return format.load(filename, archive, sceneGraph, testLoadCtx);
}

int AbstractFormatTest::helper_loadPalette(const core::String &filename, const io::ArchivePtr &archive, Format &format,
										   palette::Palette &palette) {
	return (int)format.loadPalette(filename, archive, palette, testLoadCtx);
}

bool AbstractFormatTest::onInitApp() {
	if (!AbstractVoxelTest::onInitApp()) {
		return false;
	}
	FormatConfig::init();
	voxel::getPalette().nippon();
	return true;
}

} // namespace voxelformat
