/**
 * @file
 */
#include "voxelformat/private/pnd3d/KVOFormat.h"
#include "AbstractFormatTest.h"
#include "io/BufferedReadWriteStream.h"
#include "io/MemoryArchive.h"
#include "palette/NormalPalette.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelformat {

class KVOFormatTest : public AbstractFormatTest {};

TEST_F(KVOFormatTest, testCaco) {
	scenegraph::SceneGraph graph;
	testLoad(graph, "caco.kvo", 1);
	if (HasFatalFailure() || IsSkipped()) {
		return;
	}
	const scenegraph::SceneGraphNode *node = graph.firstModelNode();
	ASSERT_NE(nullptr, node);
	EXPECT_EQ(voxel::Region(0, 0, 0, 127, 127, 127), node->region());
	EXPECT_EQ(132804, voxelutil::countVoxels(*node->volume()));
	EXPECT_GT(node->palette().size(), 1);
	EXPECT_EQ(color::RGBA(252, 196, 152, 255), node->palette().color(node->volume()->voxel(52, 96, 53).getColor()));
	EXPECT_EQ(color::RGBA(76, 56, 40, 255), node->palette().color(node->volume()->voxel(63, 90, 80).getColor()));
	EXPECT_EQ(color::RGBA(88, 96, 68, 255), node->palette().color(node->volume()->voxel(73, 47, 96).getColor()));
}

// A 2x2x2 tree, with two diagonally opposite solid voxels and one-bit palette indices.
static void writeFixture(io::BufferedReadWriteStream &stream, uint16_t flags = 0) {
	stream.writeString("KVOx", false);
	stream.writeUInt8(1);
	stream.writeUInt8(1);
	stream.writeUInt16(flags);
	stream.writeUInt32(0);
	for (int i = 0; i < 13; ++i) {
		stream.writeFloat(0.0f);
	}
	stream.writeUInt32(0);
	stream.writeUInt32(2);
	stream.writeUInt32(1);
	stream.writeUInt8(0x81);
	if ((flags & 384) == 256) {
		// chi=0x81 has one solid-mask bit: 0 means no additional solid voxels.
		stream.writeUInt8(0);
	}
	if (flags & 2) {
		// Direct BGR with signed normal, alpha and texture bytes for each voxel.
		for (int i = 0; i < 2; ++i) {
			stream.writeUInt8(i == 1 ? 255 : 0);
			stream.writeUInt8(0);
			stream.writeUInt8(i == 0 ? 255 : 0);
			stream.writeUInt8(127);
			stream.writeUInt8(0);
			stream.writeUInt8(0);
			stream.writeUInt8(255);
			stream.writeUInt8(42);
		}
		return;
	}
	stream.writeUInt16(2);
	stream.writeUInt8(0);
	stream.writeUInt8(0);
	stream.writeUInt8(255);
	stream.writeUInt8(255);
	stream.writeUInt8(0);
	stream.writeUInt8(0);
	stream.writeUInt8(2);
}

TEST_F(KVOFormatTest, testPaletteAndCoordinates) {
	io::BufferedReadWriteStream stream;
	writeFixture(stream);
	const io::MemoryArchivePtr archive = io::openMemoryArchive();
	ASSERT_TRUE(archive->add("test.kvo", stream.getBuffer(), stream.size()));
	KVOFormat format;
	scenegraph::SceneGraph graph;
	ASSERT_TRUE(format.load("test.kvo", archive, graph, testLoadCtx));
	const scenegraph::SceneGraphNode *node = graph.firstModelNode();
	ASSERT_NE(nullptr, node);
	EXPECT_EQ(2, voxelutil::countVoxels(*node->volume()));
	EXPECT_EQ(color::RGBA(255, 0, 0, 255), node->palette().color(node->volume()->voxel(0, 1, 0).getColor()));
	EXPECT_EQ(color::RGBA(0, 0, 255, 255), node->palette().color(node->volume()->voxel(1, 0, 1).getColor()));
}

TEST_F(KVOFormatTest, testSurfaceVariantsAndRGB) {
	for (uint16_t flags : {128, 256, 58}) {
		io::BufferedReadWriteStream stream;
		writeFixture(stream, flags);
		const io::MemoryArchivePtr archive = io::openMemoryArchive();
		ASSERT_TRUE(archive->add("test.kvo", stream.getBuffer(), stream.size()));
		KVOFormat format;
		scenegraph::SceneGraph graph;
		ASSERT_TRUE(format.load("test.kvo", archive, graph, testLoadCtx)) << flags;
		const scenegraph::SceneGraphNode *node = graph.firstModelNode();
		ASSERT_NE(nullptr, node);
		EXPECT_EQ(2, voxelutil::countVoxels(*node->volume()));
		EXPECT_EQ(color::RGBA(255, 0, 0, 255), node->palette().color(node->volume()->voxel(0, 1, 0).getColor()));
		EXPECT_EQ(color::RGBA(0, 0, 255, 255), node->palette().color(node->volume()->voxel(1, 0, 1).getColor()));
	}
}

TEST_F(KVOFormatTest, testMultipleLevelsAndSolidBlocks) {
	for (uint16_t flags : {0, 128, 256}) {
		io::BufferedReadWriteStream stream;
		stream.writeString("KVOx", false);
		stream.writeUInt8(1);
		stream.writeUInt8(2);
		stream.writeUInt16(flags);
		stream.writeUInt32(0);
		for (int i = 0; i < 13; ++i) {
			stream.writeFloat(0.0f);
		}
		stream.writeUInt32(1);
		stream.writeUInt32(9);
		stream.writeUInt32(2);
		stream.writeUInt8(1); // Root child 0 has a leaf node.
		if (flags == 0) {
			stream.writeUInt8(0xc0); // Root child 7 is solid, leaf child 0 is solid.
			stream.writeUInt8(0);
		} else {
			stream.writeUInt8(1); // Leaf surface mask.
			if (flags == 256) {
				stream.writeUInt8(128); // Root child 7 is a solid 2x2x2 block.
				stream.writeUInt8(0);	// Leaf has no additional solid voxels.
			}
		}
		stream.writeUInt16(1);
		stream.writeUInt8(0);
		stream.writeUInt8(0);
		stream.writeUInt8(255);
		stream.writeUInt16(0); // Nine one-bit palette indices (one for surface-only).
		const io::MemoryArchivePtr archive = io::openMemoryArchive();
		ASSERT_TRUE(archive->add("test.kvo", stream.getBuffer(), stream.size()));
		KVOFormat format;
		scenegraph::SceneGraph graph;
		ASSERT_TRUE(format.load("test.kvo", archive, graph, testLoadCtx)) << flags;
		const scenegraph::SceneGraphNode *node = graph.firstModelNode();
		ASSERT_NE(nullptr, node);
		EXPECT_EQ(flags == 128 ? 1 : 9, voxelutil::countVoxels(*node->volume()));
		EXPECT_TRUE(voxel::isAir(node->volume()->voxel(1, 3, 0).getMaterial()));
	}
}

TEST_F(KVOFormatTest, testSignedNormals) {
	io::BufferedReadWriteStream stream;
	writeFixture(stream, 58);
	stream.seek(80);
	stream.writeUInt8(0);
	stream.writeUInt8(127); // Down in KVO becomes negative Y in vengi.
	stream.writeUInt8(0);
	stream.seek(88);
	stream.writeUInt8(0);
	stream.writeUInt8(0);
	stream.writeUInt8(0); // Zero means no normal.
	const io::MemoryArchivePtr archive = io::openMemoryArchive();
	ASSERT_TRUE(archive->add("test.kvo", stream.getBuffer(), stream.size()));
	KVOFormat format;
	scenegraph::SceneGraph graph;
	ASSERT_TRUE(format.load("test.kvo", archive, graph, testLoadCtx));
	const scenegraph::SceneGraphNode *node = graph.firstModelNode();
	ASSERT_TRUE(node->hasNormalPalette());
	const uint8_t normal = node->volume()->voxel(0, 1, 0).getNormal();
	ASSERT_NE(NO_NORMAL, normal);
	EXPECT_GT(
		glm::dot(glm::normalize(node->normalPalette().normal3f(normal - NORMAL_PALETTE_OFFSET)), glm::vec3(0, -1, 0)),
		0.999f);
	EXPECT_EQ(NO_NORMAL, node->volume()->voxel(1, 0, 1).getNormal());
}

TEST_F(KVOFormatTest, testPackedEquivecNormalsAndAlpha) {
	io::BufferedReadWriteStream stream;
	writeFixture(stream, 20);
	stream.seek(10);
	stream.writeUInt8(3); // Eight generated directions, encoded in three bits.
	stream.seek(stream.size() - 1);
	// Each record has 1 color bit, 3 normal bits, 8 alpha bits, crossing byte boundaries.
	const uint32_t records = (255u << 4) | (1u << 12) | (7u << 13) | (128u << 16);
	for (int i = 0; i < 3; ++i) {
		stream.writeUInt8((records >> (8 * i)) & 255);
	}
	const io::MemoryArchivePtr archive = io::openMemoryArchive();
	ASSERT_TRUE(archive->add("test.kvo", stream.getBuffer(), stream.size()));
	KVOFormat format;
	scenegraph::SceneGraph graph;
	ASSERT_TRUE(format.load("test.kvo", archive, graph, testLoadCtx));
	const scenegraph::SceneGraphNode *node = graph.firstModelNode();
	ASSERT_TRUE(node->hasNormalPalette());
	const voxel::Voxel &first = node->volume()->voxel(0, 1, 0);
	const voxel::Voxel &last = node->volume()->voxel(1, 0, 1);
	ASSERT_NE(NO_NORMAL, first.getNormal());
	ASSERT_NE(NO_NORMAL, last.getNormal());
	const glm::vec3 n0 = glm::normalize(node->normalPalette().normal3f(first.getNormal() - NORMAL_PALETTE_OFFSET));
	const glm::vec3 n7 = glm::normalize(node->normalPalette().normal3f(last.getNormal() - NORMAL_PALETTE_OFFSET));
	EXPECT_GT(glm::dot(n0, glm::vec3(0.4841229f, 0.0f, -0.875f)), 0.999f);
	EXPECT_NEAR(0.875f, n7.z, 0.01f);
	EXPECT_GT(n7.y, 0.4f);
	EXPECT_EQ(255, node->palette().color(first.getColor()).a);
	EXPECT_EQ(128, node->palette().color(last.getColor()).a);
}

TEST_F(KVOFormatTest, testSaveSolidCubeAndNormals) {
	scenegraph::SceneGraph graph;
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.createVolume(voxel::Region(0, 0, 0, 3, 3, 3));
	palette::Palette pal;
	pal.tryAdd(color::RGBA(255, 0, 0, 255));
	pal.tryAdd(color::RGBA(0, 0, 255, 128));
	node.setPalette(pal);
	palette::NormalPalette normals;
	normals.setNormal(0, glm::vec3(0, 1, 0));
	node.setNormalPalette(normals);
	node.setPivot(glm::vec3(0.25f, 0.5f, 0.75f));
	for (int z = 0; z < 4; ++z) {
		for (int y = 0; y < 4; ++y) {
			for (int x = 0; x < 4; ++x) {
				node.volume()->setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, 0));
			}
		}
	}
	node.volume()->setVoxel(0, 0, 0, voxel::Voxel());
	node.volume()->setVoxel(3, 3, 3, voxel::createVoxel(voxel::VoxelType::Generic, 1, NORMAL_PALETTE_OFFSET));
	graph.emplace(core::move(node));
	graph.updateTransforms();
	KVOFormat format;
	for (const char *filename : {"test.kvo", "test.kvs", "test.kvn"}) {
		const io::MemoryArchivePtr archive = io::openMemoryArchive();
		ASSERT_TRUE(format.save(graph, filename, archive, testSaveCtx));
		scenegraph::SceneGraph loaded;
		ASSERT_TRUE(format.load(filename, archive, loaded, testLoadCtx));
		const scenegraph::SceneGraphNode *model = loaded.firstModelNode();
		ASSERT_NE(nullptr, model);
		EXPECT_EQ(63, voxelutil::countVoxels(*model->volume()));
		EXPECT_EQ(graph.firstModelNode()->pivot(), model->pivot());
		EXPECT_TRUE(voxel::isAir(model->volume()->voxel(0, 0, 0).getMaterial()));
		const voxel::Voxel &v = model->volume()->voxel(3, 3, 3);
		EXPECT_EQ(color::RGBA(0, 0, 255, 128), model->palette().color(v.getColor()));
		ASSERT_TRUE(model->hasNormalPalette());
		ASSERT_NE(NO_NORMAL, v.getNormal());
		EXPECT_GT(glm::normalize(model->normalPalette().normal3f(v.getNormal() - NORMAL_PALETTE_OFFSET)).y, 0.999f);
		EXPECT_EQ(NO_NORMAL, model->volume()->voxel(1, 1, 1).getNormal());
	}
}

TEST_F(KVOFormatTest, testSavePaddedVolume) {
	scenegraph::SceneGraph graph;
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.createVolume(voxel::Region(-1, 2, 3, 1, 3, 3));
	node.volume()->setVoxel(-1, 2, 3, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	node.volume()->setVoxel(1, 3, 3, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	graph.emplace(core::move(node));
	graph.updateTransforms();
	const io::MemoryArchivePtr archive = io::openMemoryArchive();
	KVOFormat format;
	ASSERT_TRUE(format.save(graph, "padded.kvo", archive, testSaveCtx));
	scenegraph::SceneGraph loaded;
	ASSERT_TRUE(format.load("padded.kvo", archive, loaded, testLoadCtx));
	const scenegraph::SceneGraphNode *model = loaded.firstModelNode();
	ASSERT_NE(nullptr, model);
	EXPECT_EQ(voxel::Region(0, 0, 0, 3, 3, 3), model->region());
	EXPECT_EQ(2, voxelutil::countVoxels(*model->volume()));
	EXPECT_TRUE(voxel::isBlocked(model->volume()->voxel(0, 0, 0).getMaterial()));
	EXPECT_TRUE(voxel::isBlocked(model->volume()->voxel(2, 1, 0).getMaterial()));
}

TEST_F(KVOFormatTest, testUnsupportedFlags) {
	for (uint16_t flags : {64, 384, 512, 12}) {
		io::BufferedReadWriteStream stream;
		writeFixture(stream, flags);
		const io::MemoryArchivePtr archive = io::openMemoryArchive();
		ASSERT_TRUE(archive->add("test.kvo", stream.getBuffer(), stream.size()));
		KVOFormat format;
		scenegraph::SceneGraph graph;
		EXPECT_FALSE(format.load("test.kvo", archive, graph, testLoadCtx)) << flags;
	}
}

TEST_F(KVOFormatTest, testTruncated) {
	io::BufferedReadWriteStream stream;
	writeFixture(stream);
	KVOFormat format;
	for (int64_t size = 0; size < stream.size(); ++size) {
		const io::MemoryArchivePtr archive = io::openMemoryArchive();
		ASSERT_TRUE(archive->add("test.kvo", stream.getBuffer(), size));
		scenegraph::SceneGraph graph;
		EXPECT_FALSE(format.load("test.kvo", archive, graph, testLoadCtx)) << size;
		EXPECT_TRUE(graph.empty());
	}
}

} // namespace voxelformat
