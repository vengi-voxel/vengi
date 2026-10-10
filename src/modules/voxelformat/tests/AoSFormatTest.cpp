/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "core/FourCC.h"
#include "core/ScopedPtr.h"
#include "io/MemoryArchive.h"
#include "io/Stream.h"
#include "scenegraph/SceneGraphNodeProperties.h"
#include "voxelformat/private/aceofspades/AoSFormat.h"

namespace voxelformat {

class AoSFormatTest : public AbstractFormatTest {
protected:
	const core::String metadata = R"({"title":"Workshop map","author":"Tester","description":"Test map","ugc_entities":[{"type":"spawn","position":[1,2,3]}],"tags":["map","tdm"],"aos_ugc_handle":18446744073709551615})";

	void writeChunk(io::SeekableWriteStream &stream, uint32_t tag, const core::String &data) {
		ASSERT_TRUE(stream.writeUInt32(tag));
		ASSERT_TRUE(stream.writeUInt32((uint32_t)data.size()));
		ASSERT_EQ(stream.write(data.c_str(), data.size()), (int)data.size());
	}

	io::MemoryArchivePtr container(bool ugcFirst, const core::String &ugc) {
		io::MemoryArchivePtr archive = io::openMemoryArchive();
		core::ScopedPtr<io::SeekableReadStream> terrain(helper_filesystemarchive()->readStream("aceofspades.vxl"));
		if (!terrain) {
			return {};
		}
		core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream("test.aos"));
		if (ugcFirst) {
			writeChunk(*stream, FourCC('U', 'G', 'C', '\0'), ugc);
		}
		stream->writeUInt32(FourCC('V', 'X', 'L', '\0'));
		stream->writeUInt32((uint32_t)terrain->size());
		stream->writeStream(*terrain);
		if (!ugcFirst) {
			writeChunk(*stream, FourCC('U', 'G', 'C', '\0'), ugc);
		}
		return archive;
	}
};

TEST_F(AoSFormatTest, testReadWrite) {
	for (bool ugcFirst : {false, true}) {
		const io::MemoryArchivePtr archive = container(ugcFirst, metadata);
		ASSERT_TRUE(archive);
		AoSFormat format;
		scenegraph::SceneGraph graph;
		ASSERT_TRUE(format.load("test.aos", archive, graph, testLoadCtx));
		ASSERT_EQ(graph.size(), 1u);
		EXPECT_EQ(graph.root().property(scenegraph::PropTitle), "Workshop map");
		EXPECT_EQ(graph.root().property(scenegraph::PropAuthor), "Tester");
		EXPECT_EQ(graph.root().property(scenegraph::PropDescription), "Test map");
		EXPECT_EQ(graph.root().property("aos_ugc"), metadata);
		ASSERT_TRUE(format.save(graph, "roundtrip.aos", archive, testSaveCtx));
		scenegraph::SceneGraph loaded;
		ASSERT_TRUE(format.load("roundtrip.aos", archive, loaded, testLoadCtx));
		EXPECT_EQ(loaded.root().property("aos_ugc"), metadata);
		voxel::sceneGraphComparator(graph, loaded, voxel::ValidateFlags::None);
	}
}

TEST_F(AoSFormatTest, testInvalidContainer) {
	const char *invalid[] = {"", "VXL", "VXL\0\xff\xff\xff\xff", "PNG\0\0\0\0\0", "UGC\0\x02\0\0\0[]"};
	const size_t lengths[] = {0, 3, 8, 8, 10};
	for (size_t i = 0; i < lengthof(lengths); ++i) {
		const io::MemoryArchivePtr archive = io::openMemoryArchive();
		ASSERT_TRUE(archive->add("invalid.aos", (const uint8_t *)invalid[i], lengths[i]));
		AoSFormat format;
		scenegraph::SceneGraph graph;
		EXPECT_FALSE(format.load("invalid.aos", archive, graph, testLoadCtx));
	}
}

TEST_F(AoSFormatTest, testInvalidMetadata) {
	for (const char *ugc : {"[]", "null", "not json", ""}) {
		const io::MemoryArchivePtr archive = container(false, ugc);
		ASSERT_TRUE(archive);
		AoSFormat format;
		scenegraph::SceneGraph graph;
		EXPECT_FALSE(format.load("test.aos", archive, graph, testLoadCtx));
	}
}

TEST_F(AoSFormatTest, testSaveNewMap) {
	AoSFormat format;
	const io::MemoryArchivePtr archive = io::openMemoryArchive();
	scenegraph::SceneGraph graph;
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(new voxel::RawVolume(voxel::Region(0, 0, 0, 1, 1, 1)));
	node.volume()->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	graph.emplace(core::move(node));
	graph.node(graph.root().id()).setProperty(scenegraph::PropTitle, "New map");
	ASSERT_TRUE(format.save(graph, "new.aos", archive, testSaveCtx));
	scenegraph::SceneGraph loaded;
	ASSERT_TRUE(format.load("new.aos", archive, loaded, testLoadCtx));
	EXPECT_EQ(loaded.root().property(scenegraph::PropTitle), "New map");
}

} // namespace voxelformat
