#include "AbstractVoxFormatTest.h"
#include "core/GameConfig.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "io/BufferedReadWriteStream.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "io/Stream.h"
#include "voxel/MaterialColor.h"
#include "voxel/Palette.h"
#include "voxel/RawVolume.h"
#include "voxel/tests/TestHelper.h"
#include "voxelformat/Format.h"
#include "voxelformat/FormatConfig.h"
#include "voxelformat/SceneGraph.h"
#include "voxelformat/SceneGraphNode.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelutil/VolumeVisitor.h"

#define WRITE_TO_FILE 0

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

void AbstractVoxFormatTest::testFirstAndLastPaletteIndex(const core::String &filename, Format *format, voxel::ValidateFlags flags) {
	voxel::Region region(glm::ivec3(0), glm::ivec3(1));
	voxel::RawVolume volume(region);
	EXPECT_TRUE(volume.setVoxel(0, 0, 0, createVoxel(voxel::VoxelType::Generic, 0)));
	EXPECT_TRUE(volume.setVoxel(0, 0, 1, createVoxel(voxel::VoxelType::Generic, 255)));
	io::BufferedReadWriteStream stream((int64_t)(10 * 1024 * 1024));
	SceneGraph sceneGraphsave(2);
	{
		SceneGraphNode node;
		node.setVolume(&volume, false);
		sceneGraphsave.emplace(core::move(node));
	}
	ASSERT_TRUE(format->save(sceneGraphsave, filename, stream));
	stream.seek(0);
	voxelformat::SceneGraph::MergedVolumePalette merged = load(filename, stream, *format);
	ASSERT_NE(nullptr, merged.first);
	core::ScopedPtr<voxel::RawVolume> loaded(merged.first);
	voxel::volumeComparator(volume, voxel::getPalette(), *loaded, merged.second, voxel::ValidateFlags::None);
}

void AbstractVoxFormatTest::testFirstAndLastPaletteIndexConversion(Format &srcFormat, const core::String& destFilename, Format &destFormat, voxel::ValidateFlags flags) {
	voxel::Region region(glm::ivec3(0), glm::ivec3(1));
	voxel::RawVolume original(region);
	EXPECT_TRUE(original.setVoxel(0, 0, 0, createVoxel(voxel::VoxelType::Generic, 0)));
	EXPECT_TRUE(original.setVoxel(0, 0, 1, createVoxel(voxel::VoxelType::Generic, 255)));
	io::BufferedReadWriteStream srcFormatStream((int64_t)(10 * 1024 * 1024));
	{
		SceneGraph sceneGraphsave(2);
		SceneGraphNode node;
		node.setVolume(&original, false);
		sceneGraphsave.emplace(core::move(node));
		EXPECT_TRUE(srcFormat.save(sceneGraphsave, destFilename, srcFormatStream)) << "Could not save " << destFilename;
	}
	srcFormatStream.seek(0);
	voxelformat::SceneGraph::MergedVolumePalette merged = load(destFilename, srcFormatStream, srcFormat);
	ASSERT_NE(nullptr, merged.first);
	core::ScopedPtr<voxel::RawVolume> origReloaded(merged.first);
	if ((flags & voxel::ValidateFlags::Region) == voxel::ValidateFlags::Region) {
		ASSERT_EQ(original.region(), origReloaded->region());
	}
	voxel::volumeComparator(original, voxel::getPalette(), *origReloaded, merged.second, flags);

	io::BufferedReadWriteStream stream((int64_t)(10 * 1024 * 1024));
	{
		SceneGraph sceneGraphsave(2);
		SceneGraphNode node;
		node.setVolume(merged.first, false);
		sceneGraphsave.emplace(core::move(node));
		EXPECT_TRUE(destFormat.save(sceneGraphsave, destFilename, stream)) << "Could not save " << destFilename;
	}
	stream.seek(0);
	voxelformat::SceneGraph::MergedVolumePalette merged2 = load(destFilename, stream, destFormat);
	core::ScopedPtr<voxel::RawVolume> loaded(merged2.first);
	ASSERT_NE(nullptr, loaded) << "Could not load " << destFilename;
	if ((flags & voxel::ValidateFlags::Region) == voxel::ValidateFlags::Region) {
		ASSERT_EQ(original.region(), loaded->region());
	}
	voxel::volumeComparator(original, voxel::getPalette(), *loaded, merged2.second, flags);
}

void AbstractVoxFormatTest::canLoad(const core::String &filename, size_t expectedVolumes) {
	voxelformat::SceneGraph sceneGraph;
	const io::FilePtr& file = open(filename);
	ASSERT_TRUE(file->validHandle()) << "Could not open " << filename;
	io::FileStream stream(file);
	ASSERT_TRUE(voxelformat::loadFormat(filename, stream, sceneGraph)) << "Could not load " << filename;
	ASSERT_EQ(expectedVolumes, sceneGraph.size());
}

void AbstractVoxFormatTest::checkColor(core::RGBA c1, const voxel::Palette &palette, uint8_t index, float maxDelta) {
	const core::RGBA c2 = palette.colors[index];
	const float delta = core::Color::getDistance(c1, c2);
	ASSERT_LE(delta, maxDelta) << "color1[" << core::Color::print(c1) << "], color2[" << core::Color::print(c2)
							 << "], delta[" << delta << "]";
}

void AbstractVoxFormatTest::testRGBSmall(const core::String &filename, io::SeekableReadStream &stream, voxelformat::SceneGraph &sceneGraph) {
	ASSERT_TRUE(voxelformat::loadFormat(filename, stream, sceneGraph));
	EXPECT_EQ(1u, sceneGraph.size());

	voxel::Palette palette;
	EXPECT_TRUE(palette.nippon());

	const core::RGBA red(255, 0, 0);
	const core::RGBA green(0, 255, 0);
	const core::RGBA blue(0, 0, 255);

	for (const voxelformat::SceneGraphNode &node : sceneGraph) {
		const voxel::RawVolume *volume = node.volume();
		EXPECT_EQ(3, voxelutil::visitVolume(*volume, [] (int, int, int, const voxel::Voxel&) {}));
		checkColor(blue, node.palette(), volume->voxel( 0,  0,  0).getColor(), 0.0f);
		checkColor(green, node.palette(), volume->voxel( 1,  0,  0).getColor(), 0.0f);
		checkColor(red, node.palette(), volume->voxel( 2,  0, 0).getColor(), 0.0f);
	}
}

void AbstractVoxFormatTest::testRGBSmall(const core::String &filename) {
	voxelformat::SceneGraph sceneGraph;
	const io::FilePtr& file = open(filename);
	ASSERT_TRUE(file->validHandle());
	io::FileStream stream(file);
	testRGBSmall(filename, stream, sceneGraph);
}

void AbstractVoxFormatTest::testRGBSmallSaveLoad(const core::String &filename) {
	const core::String formatExt = core::string::extractExtension(filename);
	const core::String saveFilename = "test." + formatExt;
	testRGBSmallSaveLoad(filename, saveFilename);
}

void AbstractVoxFormatTest::testRGBSmallSaveLoad(const core::String &filename, const core::String &saveFilename) {
	voxelformat::SceneGraph sceneGraph;
	{
		// load and check that the file contains the expected colors
		io::FileStream loadStream(open(filename));
		ASSERT_TRUE(loadStream.valid());
		testRGBSmall(filename, loadStream, sceneGraph);
	}

	io::BufferedReadWriteStream saveStream((int64_t)(10 * 1024 * 1024));
	ASSERT_TRUE(voxelformat::saveFormat(sceneGraph, saveFilename, saveStream));
	saveStream.seek(0);

	SceneGraph loadSceneGraph;
	testRGBSmall(saveFilename, saveStream, loadSceneGraph);
}

void AbstractVoxFormatTest::testRGB(const core::String &filename, float maxDelta) {
	voxelformat::SceneGraph sceneGraph;
	const io::FilePtr& file = open(filename);
	ASSERT_TRUE(file->validHandle());
	io::FileStream stream(file);
	ASSERT_TRUE(voxelformat::loadFormat(filename, stream, sceneGraph));
	EXPECT_EQ(1u, sceneGraph.size());

	voxel::Palette palette;
	EXPECT_TRUE(palette.nippon());

	const core::RGBA red = palette.colors[37];
	const core::RGBA green = palette.colors[149];
	const core::RGBA blue = palette.colors[197];

	for (const voxelformat::SceneGraphNode &node : sceneGraph) {
		const voxel::RawVolume *volume = node.volume();
		EXPECT_EQ(99, voxelutil::visitVolume(*volume, [] (int, int, int, const voxel::Voxel&) {}));
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

		checkColor(red, node.palette(), volume->voxel( 9,  0,  4).getColor(), maxDelta);
		checkColor(green, node.palette(), volume->voxel( 9,  0,  12).getColor(), maxDelta);
		checkColor(blue, node.palette(), volume->voxel( 9,  0,  19).getColor(), maxDelta);
	}
}

void AbstractVoxFormatTest::testLoadSaveAndLoad(const core::String &srcFilename, Format &srcFormat,
												const core::String &destFilename, Format &destFormat,
												voxel::ValidateFlags flags, float maxDelta) {
	voxelformat::SceneGraph sceneGraph;
	ASSERT_TRUE(loadGroups(srcFilename, srcFormat, sceneGraph)) << "Failed to load " << srcFilename;
	io::BufferedReadWriteStream stream((int64_t)(10 * 1024 * 1024));
	ASSERT_TRUE(destFormat.save(sceneGraph, destFilename, stream)) << "Could not save " << destFilename;
	stream.seek(0);
	voxelformat::SceneGraph::MergedVolumePalette mergedLoad = load(destFilename, stream, destFormat);
	core::ScopedPtr<voxel::RawVolume> loaded(mergedLoad.first);
	ASSERT_NE(nullptr, loaded) << "Could not load " << destFilename;
	voxelformat::SceneGraph::MergedVolumePalette merged = sceneGraph.merge();
	core::ScopedPtr<voxel::RawVolume> src(merged.first);
	volumeComparator(*src, merged.second, *loaded, mergedLoad.second, flags, maxDelta);
}

void AbstractVoxFormatTest::testLoadSaveAndLoadSceneGraph(const core::String &srcFilename, Format &srcFormat,
												const core::String &destFilename, Format &destFormat,
												voxel::ValidateFlags flags, float maxDelta) {
	voxelformat::SceneGraph srcSceneGraph;
	ASSERT_TRUE(loadGroups(srcFilename, srcFormat, srcSceneGraph)) << "Failed to load " << srcFilename;
#if WRITE_TO_FILE
	{
		io::FileStream stream(open(destFilename, io::FileMode::SysWrite));
		ASSERT_TRUE(destFormat.save(srcSceneGraph, destFilename, stream)) << "Could not save " << destFilename;
	}
#else
	io::BufferedReadWriteStream stream((int64_t)(10 * 1024 * 1024));
	ASSERT_TRUE(destFormat.save(srcSceneGraph, destFilename, stream)) << "Could not save " << destFilename;
	stream.seek(0);
#endif
	voxelformat::SceneGraph destSceneGraph;
#if WRITE_TO_FILE
	{
		io::FileStream stream(open(destFilename));
		ASSERT_TRUE(destFormat.load(destFilename, stream, destSceneGraph)) << "Failed to load the target format";
	}
#else
	ASSERT_TRUE(destFormat.load(destFilename, stream, destSceneGraph)) << "Failed to load the target format";
#endif
	voxel::sceneGraphComparator(srcSceneGraph, destSceneGraph, flags, maxDelta);
}

void AbstractVoxFormatTest::testSaveSingleVoxel(const core::String& filename, Format* format) {
	voxel::Region region(glm::ivec3(0), glm::ivec3(0));
	voxel::RawVolume original(region);
	original.setVoxel(0, 0, 0, createVoxel(voxel::VoxelType::Generic, 1));
	io::BufferedReadWriteStream bufferedStream((int64_t)(10 * 1024 * 1024));
	SceneGraph sceneGraphsave(2);
	{
		SceneGraphNode node;
		node.setVolume(&original, false);
		sceneGraphsave.emplace(core::move(node));
	}
	ASSERT_TRUE(format->save(sceneGraphsave, filename, bufferedStream));
	bufferedStream.seek(0);
	voxelformat::SceneGraph::MergedVolumePalette mergedLoad = load(filename, bufferedStream, *format);
	core::ScopedPtr<voxel::RawVolume> loaded(mergedLoad.first);
	ASSERT_NE(nullptr, loaded) << "Could not load single voxel file " << filename;
	voxel::volumeComparator(original, voxel::getPalette(), *loaded, mergedLoad.second, voxel::ValidateFlags::Color | voxel::ValidateFlags::Region);
}

void AbstractVoxFormatTest::testSaveSmallVolume(const core::String& filename, Format* format) {
	voxel::Region region(glm::ivec3(0), glm::ivec3(0, 1, 1));
	voxel::RawVolume original(region);
	ASSERT_TRUE(original.setVoxel(0, 0, 0, createVoxel(voxel::VoxelType::Generic, 0)));
	ASSERT_TRUE(original.setVoxel(0, 0, 1, createVoxel(voxel::VoxelType::Generic, 200)));
	ASSERT_TRUE(original.setVoxel(0, 1, 1, createVoxel(voxel::VoxelType::Generic, 201)));
	ASSERT_TRUE(original.setVoxel(0, 0, 0, createVoxel(voxel::VoxelType::Generic, 255)));
	io::BufferedReadWriteStream bufferedStream((int64_t)(10 * 1024 * 1024));
	SceneGraph sceneGraphsave(2);
	{
		SceneGraphNode node;
		node.setVolume(&original, false);
		sceneGraphsave.emplace(core::move(node));
	}
	ASSERT_TRUE(format->save(sceneGraphsave, filename, bufferedStream));
	bufferedStream.seek(0);
	voxelformat::SceneGraph::MergedVolumePalette mergedLoad = load(filename, bufferedStream, *format);
	core::ScopedPtr<voxel::RawVolume> loaded(mergedLoad.first);
	ASSERT_NE(nullptr, loaded) << "Could not load single voxel file " << filename;
	voxel::volumeComparator(original, voxel::getPalette(), *loaded, mergedLoad.second, voxel::ValidateFlags::Color | voxel::ValidateFlags::Region);
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
	io::BufferedReadWriteStream bufferedStream((int64_t)(10 * 1024 * 1024));
	ASSERT_TRUE(format->save(sceneGraph, filename, bufferedStream));
	bufferedStream.seek(0);
	voxelformat::SceneGraph sceneGraphLoad;
	EXPECT_TRUE(format->load(filename, bufferedStream, sceneGraphLoad));
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
	io::BufferedReadWriteStream bufferedStream((int64_t)(10 * 1024 * 1024));
	ASSERT_TRUE(format->save(sceneGraph, filename, bufferedStream));
	voxelformat::SceneGraph sceneGraphLoad;
	bufferedStream.seek(0);
	EXPECT_TRUE(format->load(filename, bufferedStream, sceneGraphLoad));
	EXPECT_EQ(sceneGraphLoad.size(), sceneGraph.size());
}

void AbstractVoxFormatTest::testSaveLoadVoxel(const core::String &filename, Format *format, int mins, int maxs, voxel::ValidateFlags flags) {
	const voxel::Region region(mins, maxs);
	voxel::RawVolume original(region);

	original.setVoxel(mins, mins, mins, createVoxel(voxel::VoxelType::Generic, 0));
	original.setVoxel(mins, mins, maxs, createVoxel(voxel::VoxelType::Generic, 244));
	original.setVoxel(mins, maxs, maxs, createVoxel(voxel::VoxelType::Generic, 126));
	original.setVoxel(mins, maxs, mins, createVoxel(voxel::VoxelType::Generic, 255));

	original.setVoxel(maxs, maxs, maxs, createVoxel(voxel::VoxelType::Generic, 1));
	original.setVoxel(maxs, maxs, mins, createVoxel(voxel::VoxelType::Generic, 245));
	original.setVoxel(maxs, mins, mins, createVoxel(voxel::VoxelType::Generic, 127));
	original.setVoxel(maxs, mins, maxs, createVoxel(voxel::VoxelType::Generic, 200));

	testSaveLoadVolume(filename, original, format, flags);
}

void AbstractVoxFormatTest::testSaveLoadVolume(const core::String &filename, const voxel::RawVolume &original, Format *format, voxel::ValidateFlags flags) {
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
		/*nodeId =*/ sceneGraph.emplace(core::move(node), nodeId);
	}

	io::SeekableReadStream *readStream;
	io::SeekableWriteStream *writeStream;

#if WRITE_TO_FILE
	io::FilePtr sfile = open(filename, io::FileMode::SysWrite);
	io::FileStream fileWriteStream(sfile);
	writeStream = &fileWriteStream;
#else
	io::BufferedReadWriteStream bufferedStream((int64_t)(10 * 1024 * 1024));
	writeStream = &bufferedStream;
#endif

	ASSERT_TRUE(format->save(sceneGraph, filename, *writeStream)) << "Could not save the scene graph";

#if WRITE_TO_FILE
	io::FilePtr readfile = open(filename);
	io::FileStream fileReadStream(readfile);
	readStream = &fileReadStream;
#else
	readStream = &bufferedStream;
#endif

	readStream->seek(0);

	voxelformat::SceneGraph::MergedVolumePalette merged = load(filename, *readStream, *format);
	core::ScopedPtr<voxel::RawVolume> loaded(merged.first);
	ASSERT_NE(nullptr, loaded) << "Could not load the merged volumes";
	voxel::volumeComparator(original, voxel::getPalette(), *loaded, merged.second, flags);
}

#undef WRITE_TO_FILE

io::FilePtr AbstractVoxFormatTest::open(const core::String &filename, io::FileMode mode) {
	const io::FilePtr& file = io::filesystem()->open(core::String(filename), mode);
	return file;
}

SceneGraph::MergedVolumePalette AbstractVoxFormatTest::load(const core::String& filename, io::SeekableReadStream& stream, Format& format) {
	SceneGraph sceneGraph;
	if (!format.load(filename, stream, sceneGraph)) {
		Log::error("Failed to load %s", filename.c_str());
		return SceneGraph::MergedVolumePalette{};
	}
	if (sceneGraph.empty()) {
		Log::error("Success - but no nodes");
		return SceneGraph::MergedVolumePalette{};
	}
	Log::debug("Loaded %s - merging", filename.c_str());
	return sceneGraph.merge();
}

SceneGraph::MergedVolumePalette AbstractVoxFormatTest::load(const core::String& filename, Format& format) {
	SceneGraph sceneGraph;
	if (!loadGroups(filename, format, sceneGraph)) {
		return SceneGraph::MergedVolumePalette{};
	}
	if (sceneGraph.empty()) {
		Log::error("Success - but no nodes");
		return SceneGraph::MergedVolumePalette{};
	}
	return sceneGraph.merge();
}

bool AbstractVoxFormatTest::loadGroups(const core::String& filename, Format& format, voxelformat::SceneGraph &sceneGraph) {
	const io::FilePtr& file = open(filename);
	if (!file->validHandle()) {
		return false;
	}
	io::FileStream stream(file);
	return format.load(filename, stream, sceneGraph);
}

int AbstractVoxFormatTest::loadPalette(const core::String& filename, Format& format, voxel::Palette &palette) {
	const io::FilePtr& file = open(filename);
	if (!file->validHandle()) {
		return 0;
	}
	io::FileStream stream(file);
	const int size = (int)format.loadPalette(filename, stream, palette);
	const core::String paletteFilename = core::string::extractFilename(filename) + ".png";
	palette.save(paletteFilename.c_str());
	return size;
}

bool AbstractVoxFormatTest::onInitApp() {
	if (!AbstractVoxelTest::onInitApp()) {
		return false;
	}
	FormatConfig::init();
	return true;
}

} // namespace voxel
