#include "AbstractVoxFormatTest.h"
#include "core/StringUtil.h"
#include "io/BufferedReadWriteStream.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "io/Stream.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxelformat/SceneGraph.h"
#include "voxelformat/SceneGraphNode.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelformat {

const voxel::Voxel AbstractVoxFormatTest::Empty;

void AbstractVoxFormatTest::dump(const core::String& srcFilename, const SceneGraph &sceneGraph) {
	int i = 0;
	const core::String& prefix = core::string::extractFilename(srcFilename);
	for (const auto &node : sceneGraph) {
		const core::String& file = core::string::format("%s-%02i-%s.txt", prefix.c_str(), i, node.name().c_str());
		const core::String& structName = core::string::format("model_%i", i);
		dump(structName, node.volume(), core::string::sanitizeFilename(file));
		++i;
	}
}

void AbstractVoxFormatTest::dump(const core::String& structName, voxel::RawVolume* v, const core::String& filename) {
	const io::FilePtr& file = open(filename, io::FileMode::SysWrite);
	ASSERT_TRUE(file->validHandle());
	io::FileStream stream(file);
	stream.writeString(core::string::format("struct %s {\n", structName.c_str()), false);
	stream.writeString("static core::SharedPtr<voxel::RawVolume> create() {\n", false);
	const glm::ivec3 &mins = v->region().getLowerCorner();
	const glm::ivec3 &maxs = v->region().getUpperCorner();
	stream.writeString(core::string::format("\tglm::ivec3 mins(%i, %i, %i);\n", mins.x, mins.y, mins.z), false);
	stream.writeString(core::string::format("\tglm::ivec3 maxs(%i, %i, %i);\n", maxs.x, maxs.y, maxs.z), false);
	stream.writeString("\tvoxel::Region region(mins, maxs);\n", false);
	stream.writeString("\tcore::SharedPtr<voxel::RawVolume> v = core::make_shared<voxel::RawVolume>(region);\n", false);
	voxelutil::visitVolume(*v, [&](int x, int y, int z, const voxel::Voxel &voxel) {
		stream.writeString(
			core::string::format("\tv->setVoxel(%i, %i, %i, voxel::createVoxel(voxel::VoxelType::Generic, %i));\n", x,
								 y, z, voxel.getColor()), false);
	});
	stream.writeString("\treturn v;\n}\n};\n", false);
}

void AbstractVoxFormatTest::testFirstAndLastPaletteIndex(const core::String &filename, Format *format, bool includingColor, bool includingRegion) {
	voxel::Region region(glm::ivec3(0), glm::ivec3(1));
	voxel::RawVolume volume(region);
	EXPECT_TRUE(volume.setVoxel(0, 0, 0, createVoxel(voxel::VoxelType::Generic, 0)));
	EXPECT_TRUE(volume.setVoxel(0, 0, 1, createVoxel(voxel::VoxelType::Generic, 255)));
	io::BufferedReadWriteStream stream(10 * 1024 * 1024);
	ASSERT_TRUE(format->save(&volume, filename, stream));
	stream.seek(0);
	voxelformat::SceneGraph::MergedVolumePalette merged = load(filename, stream, *format);
	ASSERT_NE(nullptr, merged.first);
	std::unique_ptr<voxel::RawVolume> loaded(merged.first);
	volumeComparator(volume, voxel::getPalette(), *loaded, merged.second, false, includingRegion);
}

void AbstractVoxFormatTest::testFirstAndLastPaletteIndexConversion(Format &srcFormat, const core::String& destFilename, Format &destFormat, bool includingColor, bool includingRegion) {
	voxel::Region region(glm::ivec3(0), glm::ivec3(1));
	voxel::RawVolume original(region);
	EXPECT_TRUE(original.setVoxel(0, 0, 0, createVoxel(voxel::VoxelType::Generic, 0)));
	EXPECT_TRUE(original.setVoxel(0, 0, 1, createVoxel(voxel::VoxelType::Generic, 255)));
	io::BufferedReadWriteStream srcFormatStream(10 * 1024 * 1024);
	EXPECT_TRUE(srcFormat.save(&original, destFilename, srcFormatStream)) << "Could not save " << destFilename;
	srcFormatStream.seek(0);
	voxelformat::SceneGraph::MergedVolumePalette merged = load(destFilename, srcFormatStream, srcFormat);
	ASSERT_NE(nullptr, merged.first);
	std::unique_ptr<voxel::RawVolume> origReloaded(merged.first);
	if (includingRegion) {
		ASSERT_EQ(original.region(), origReloaded->region());
	}
	volumeComparator(original, voxel::getPalette(), *origReloaded, merged.second, includingColor, includingRegion);

	io::BufferedReadWriteStream stream(10 * 1024 * 1024);
	EXPECT_TRUE(destFormat.save(origReloaded.get(), destFilename, stream)) << "Could not save " << destFilename;
	stream.seek(0);
	voxelformat::SceneGraph::MergedVolumePalette merged2 = load(destFilename, stream, destFormat);
	std::unique_ptr<voxel::RawVolume> loaded(merged2.first);
	ASSERT_NE(nullptr, loaded) << "Could not load " << destFilename;
	if (includingRegion) {
		ASSERT_EQ(original.region(), loaded->region());
	}
	volumeComparator(original, voxel::getPalette(), *loaded, merged2.second, includingColor, includingRegion);
}

void AbstractVoxFormatTest::canLoad(const core::String &filename, size_t expectedVolumes) {
	voxelformat::SceneGraph sceneGraph;
	const io::FilePtr& file = open(filename);
	ASSERT_TRUE(file->validHandle()) << "Could not open " << filename;
	io::FileStream stream(file);
	ASSERT_TRUE(voxelformat::loadFormat(filename, stream, sceneGraph)) << "Could not load " << filename;
	ASSERT_EQ(expectedVolumes, sceneGraph.size());
}

void AbstractVoxFormatTest::checkColor(core::RGBA c1, const voxel::Palette &palette, uint8_t index) {
	const core::RGBA c2 = palette.colors[index];
	const float delta = core::Color::getDistance(c1, c2);
	ASSERT_LT(delta, 0.001f) << "color1[" << core::Color::toHex(c1) << "], color2[" << core::Color::toHex(c2)
							 << "], delta[" << delta << "]";
}

void AbstractVoxFormatTest::testRGB(const core::String &filename) {
	voxelformat::SceneGraph sceneGraph;
	const io::FilePtr& file = open(filename);
	ASSERT_TRUE(file->validHandle());
	io::FileStream stream(file);
	ASSERT_TRUE(voxelformat::loadFormat(filename, stream, sceneGraph));
	EXPECT_EQ(1u, sceneGraph.size());

	const core::RGBA red = core::Color::getRGBA(core::Color::Red);
	const core::RGBA green = core::Color::getRGBA(core::Color::Green);
	const core::RGBA blue = core::Color::getRGBA(core::Color::Blue);

	for (const voxelformat::SceneGraphNode &node : sceneGraph) {
		const voxel::RawVolume *volume = node.volume();
		EXPECT_EQ(voxel::VoxelType::Generic, volume->voxel( 0,  0,  0).getMaterial());
		EXPECT_EQ(voxel::VoxelType::Generic, volume->voxel(31,  0,  0).getMaterial());
		EXPECT_EQ(voxel::VoxelType::Generic, volume->voxel(31,  0, 31).getMaterial());
		EXPECT_EQ(voxel::VoxelType::Generic, volume->voxel( 0,  0, 31).getMaterial());

		EXPECT_EQ(voxel::VoxelType::Generic, volume->voxel( 0, 31,  0).getMaterial());
		EXPECT_EQ(voxel::VoxelType::Generic, volume->voxel(31, 31,  0).getMaterial());
		EXPECT_EQ(voxel::VoxelType::Generic, volume->voxel(31, 31, 31).getMaterial());
		EXPECT_EQ(voxel::VoxelType::Generic, volume->voxel( 0, 31, 31).getMaterial());

		EXPECT_EQ(voxel::VoxelType::Generic, volume->voxel( 9,  0,  4).getMaterial());
		EXPECT_EQ(voxel::VoxelType::Generic, volume->voxel( 9,  0, 12).getMaterial());
		EXPECT_EQ(voxel::VoxelType::Generic, volume->voxel( 9,  0, 19).getMaterial());

		checkColor(red, node.palette(), volume->voxel( 9,  0,  4).getColor());
		checkColor(green, node.palette(), volume->voxel( 9,  0,  12).getColor());
		checkColor(blue, node.palette(), volume->voxel( 9,  0,  19).getColor());
	}
}

void AbstractVoxFormatTest::testLoadSaveAndLoad(const core::String& srcFilename, Format &srcFormat, const core::String& destFilename, Format &destFormat, bool includingColor, bool includingRegion) {
	voxelformat::SceneGraph sceneGraph;
	EXPECT_TRUE(loadGroups(srcFilename, srcFormat, sceneGraph));
	io::BufferedReadWriteStream stream(10 * 1024 * 1024);
	EXPECT_TRUE(destFormat.saveGroups(sceneGraph, destFilename, stream)) << "Could not save " << destFilename;
	stream.seek(0);
	voxelformat::SceneGraph::MergedVolumePalette mergedLoad = load(destFilename, stream, destFormat);
	std::unique_ptr<voxel::RawVolume> loaded(mergedLoad.first);
	ASSERT_NE(nullptr, loaded) << "Could not load " << destFilename;
	voxelformat::SceneGraph::MergedVolumePalette merged = sceneGraph.merge();
	std::unique_ptr<voxel::RawVolume> src(merged.first);
	if (includingRegion) {
		ASSERT_EQ(src->region(), loaded->region());
	}
	volumeComparator(*src, merged.second, *loaded, mergedLoad.second, includingColor, includingRegion);
}

void AbstractVoxFormatTest::testSaveSingleVoxel(const core::String& filename, Format* format) {
	voxel::Region region(glm::ivec3(0), glm::ivec3(0));
	voxel::RawVolume original(region);
	original.setVoxel(0, 0, 0, createVoxel(voxel::VoxelType::Generic, 1));
	io::BufferedReadWriteStream bufferedStream(10 * 1024 * 1024);
	ASSERT_TRUE(format->save(&original, filename, bufferedStream));
	bufferedStream.seek(0);
	voxelformat::SceneGraph::MergedVolumePalette mergedLoad = load(filename, bufferedStream, *format);
	std::unique_ptr<voxel::RawVolume> loaded(mergedLoad.first);
	ASSERT_NE(nullptr, loaded) << "Could not load single voxel file " << filename;
	volumeComparator(original, voxel::getPalette(), *loaded, mergedLoad.second, true, true);
}

void AbstractVoxFormatTest::testSaveSmallVolume(const core::String& filename, Format* format) {
	voxel::Region region(glm::ivec3(0), glm::ivec3(0, 1, 1));
	voxel::RawVolume original(region);
	ASSERT_TRUE(original.setVoxel(0, 0, 0, createVoxel(voxel::VoxelType::Generic, 1)));
	ASSERT_TRUE(original.setVoxel(0, 0, 1, createVoxel(voxel::VoxelType::Generic, 200)));
	ASSERT_TRUE(original.setVoxel(0, 1, 1, createVoxel(voxel::VoxelType::Generic, 201)));
	ASSERT_TRUE(original.setVoxel(0, 0, 0, createVoxel(voxel::VoxelType::Generic, 202)));
	io::BufferedReadWriteStream bufferedStream(10 * 1024 * 1024);
	ASSERT_TRUE(format->save(&original, filename, bufferedStream));
	bufferedStream.seek(0);
	voxelformat::SceneGraph::MergedVolumePalette mergedLoad = load(filename, bufferedStream, *format);
	std::unique_ptr<voxel::RawVolume> loaded(mergedLoad.first);
	ASSERT_NE(nullptr, loaded) << "Could not load single voxel file " << filename;
	volumeComparator(original, voxel::getPalette(), *loaded, mergedLoad.second, true, true);
}

void AbstractVoxFormatTest::testSaveMultipleLayers(const core::String &filename, Format *format) {
	voxel::Region region(glm::ivec3(0), glm::ivec3(0));
	voxel::RawVolume layer1(region);
	voxel::RawVolume layer2(region);
	voxel::RawVolume layer3(region);
	voxel::RawVolume layer4(region);
	EXPECT_TRUE(layer1.setVoxel(0, 0, 0, createVoxel(voxel::VoxelType::Generic, 1)));
	EXPECT_TRUE(layer2.setVoxel(0, 0, 0, createVoxel(voxel::VoxelType::Generic, 1)));
	EXPECT_TRUE(layer3.setVoxel(0, 0, 0, createVoxel(voxel::VoxelType::Generic, 1)));
	EXPECT_TRUE(layer4.setVoxel(0, 0, 0, createVoxel(voxel::VoxelType::Generic, 1)));
	voxelformat::SceneGraph sceneGraph;
	voxelformat::SceneGraphNode node1;
	node1.setVolume(&layer1, false);
	voxelformat::SceneGraphNode node2;
	node2.setVolume(&layer2, false);
	voxelformat::SceneGraphNode node3;
	node3.setVolume(&layer3, false);
	voxelformat::SceneGraphNode node4;
	node4.setVolume(&layer4, false);
	sceneGraph.emplace(core::move(node1));
	sceneGraph.emplace(core::move(node2));
	sceneGraph.emplace(core::move(node3));
	sceneGraph.emplace(core::move(node4));
	io::BufferedReadWriteStream bufferedStream(10 * 1024 * 1024);
	ASSERT_TRUE(format->saveGroups(sceneGraph, filename, bufferedStream));
	bufferedStream.seek(0);
	voxelformat::SceneGraph sceneGraphLoad;
	EXPECT_TRUE(format->loadGroups(filename, bufferedStream, sceneGraphLoad));
	EXPECT_EQ(sceneGraphLoad.size(), sceneGraph.size());
}

void AbstractVoxFormatTest::testSave(const core::String &filename, Format *format) {
	voxel::Region region(glm::ivec3(0), glm::ivec3(0));
	voxel::RawVolume layer1(region);
	EXPECT_TRUE(layer1.setVoxel(0, 0, 0, createVoxel(voxel::VoxelType::Generic, 1)));
	voxelformat::SceneGraph sceneGraph;
	voxelformat::SceneGraphNode node1;
	node1.setVolume(&layer1, false);
	sceneGraph.emplace(core::move(node1));
	io::BufferedReadWriteStream bufferedStream(10 * 1024 * 1024);
	ASSERT_TRUE(format->saveGroups(sceneGraph, filename, bufferedStream));
	voxelformat::SceneGraph sceneGraphLoad;
	bufferedStream.seek(0);
	EXPECT_TRUE(format->loadGroups(filename, bufferedStream, sceneGraphLoad));
	EXPECT_EQ(sceneGraphLoad.size(), sceneGraph.size());
}

void AbstractVoxFormatTest::testSaveLoadVoxel(const core::String &filename, Format *format, int mins, int maxs) {
	const voxel::Region region(mins, maxs);
	voxel::RawVolume original(region);

	original.setVoxel(mins, mins, mins, createVoxel(voxel::VoxelType::Generic, 1));
	original.setVoxel(mins, mins, maxs, createVoxel(voxel::VoxelType::Generic, 245));
	original.setVoxel(mins, maxs, maxs, createVoxel(voxel::VoxelType::Generic, 127));
	original.setVoxel(mins, maxs, mins, createVoxel(voxel::VoxelType::Generic, 200));

	original.setVoxel(maxs, maxs, maxs, createVoxel(voxel::VoxelType::Generic, 1));
	original.setVoxel(maxs, maxs, mins, createVoxel(voxel::VoxelType::Generic, 245));
	original.setVoxel(maxs, mins, mins, createVoxel(voxel::VoxelType::Generic, 127));
	original.setVoxel(maxs, mins, maxs, createVoxel(voxel::VoxelType::Generic, 200));

	SceneGraph sceneGraph;
	int nodeId = 0;
	{
		SceneGraphNode node;
		node.setName("first level #1");
		node.setVolume(&original, false);
		nodeId = sceneGraph.emplace(core::move(node), nodeId);
	}
	{
		SceneGraphNode node;
		node.setName("second level #1");
		node.setVolume(&original, false);
		sceneGraph.emplace(core::move(node), nodeId);
	}
	{
		SceneGraphNode node;
		node.setName("second level #2");
		node.setVolume(&original, false);
		nodeId = sceneGraph.emplace(core::move(node), nodeId);
	}

	io::SeekableReadStream *readStream;
	io::SeekableWriteStream *writeStream;
#define WRITE_TO_FILE 0

#if WRITE_TO_FILE
	io::FilePtr sfile = open(filename, io::FileMode::SysWrite);
	io::FileStream fileWriteStream(sfile);
	writeStream = &fileWriteStream;
#else
	io::BufferedReadWriteStream bufferedStream(10 * 1024 * 1024);
	writeStream = &bufferedStream;
#endif

	ASSERT_TRUE(format->saveGroups(sceneGraph, filename, *writeStream));

#if WRITE_TO_FILE
	io::FilePtr readfile = open(filename);
	io::FileStream fileReadStream(readfile);
	readStream = &fileReadStream;
#else
	readStream = &bufferedStream;
#endif

	readStream->seek(0);

	voxelformat::SceneGraph::MergedVolumePalette merged = load(filename, *readStream, *format);
	std::unique_ptr<voxel::RawVolume> loaded(merged.first);
	ASSERT_NE(nullptr, loaded);
	volumeComparator(original, voxel::getPalette(), *loaded, merged.second, true, true);
}

} // namespace voxel
