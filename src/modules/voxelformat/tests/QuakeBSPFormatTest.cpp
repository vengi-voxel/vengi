/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "core/ConfigVar.h"
#include "io/BufferedReadWriteStream.h"
#include "io/MemoryArchive.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "util/VarUtil.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelformat/private/mesh/quake/QuakeBSPFormat.h"

namespace voxelformat {

namespace {

void writeSolidCubeBsp(io::BufferedReadWriteStream &stream, const char *wallName = "wall", uint8_t wallIndex = 4) {
	struct Lump {
		uint32_t offset = 0;
		uint32_t len = 0;
	};
	Lump lumps[15];
	for (int i = 0; i < 124; ++i) {
		stream.writeUInt8(0);
	}

	auto finish = [&](int lump, int64_t start) {
		lumps[lump].offset = (uint32_t)start;
		lumps[lump].len = (uint32_t)(stream.pos() - start);
	};

	{
		const int64_t start = stream.pos();
		const float plane[6][4] = {{1, 0, 0, 32}, {0, 1, 0, 32}, {0, 0, 1, 32}, {-1, 0, 0, 0}, {0, -1, 0, 0}, {0, 0, -1, 0}};
		const int32_t type[6] = {0, 1, 2, 0, 1, 2};
		for (int i = 0; i < 6; ++i) {
			for (int k = 0; k < 4; ++k) {
				stream.writeFloat(plane[i][k]);
			}
			stream.writeInt32(type[i]);
		}
		finish(1, start);
	}

	{
		const int64_t start = stream.pos();
		const int mipHeader = 40;
		const int pixels = 64 + 16 + 4 + 1;
		const int mipSize = mipHeader + pixels;
		stream.writeInt32(2);
		stream.writeInt32(12);
		stream.writeInt32(12 + mipSize);
		auto writeMip = [&](const char *name, uint8_t index) {
			char texName[16] = {};
			for (int i = 0; name[i] != '\0' && i < 15; ++i) {
				texName[i] = name[i];
			}
			stream.write(texName, 16);
			stream.writeUInt32(8);
			stream.writeUInt32(8);
			stream.writeUInt32(40);
			stream.writeUInt32(40 + 64);
			stream.writeUInt32(40 + 64 + 16);
			stream.writeUInt32(40 + 64 + 16 + 4);
			for (int i = 0; i < pixels; ++i) {
				stream.writeUInt8(index);
			}
		};
		writeMip(wallName, wallIndex);
		writeMip("clip", 4);
		finish(2, start);
	}

	{
		const int64_t start = stream.pos();
		const float verts[8][3] = {{0, 0, 0}, {32, 0, 0}, {32, 32, 0}, {0, 32, 0},
									{0, 0, 32}, {32, 0, 32}, {32, 32, 32}, {0, 32, 32}};
		for (int i = 0; i < 8; ++i) {
			for (int k = 0; k < 3; ++k) {
				stream.writeFloat(verts[i][k]);
			}
		}
		finish(3, start);
	}

	{
		const int64_t start = stream.pos();
		for (int i = 0; i < 6; ++i) {
			stream.writeInt32(i);
			stream.writeInt16(-1);
			stream.writeInt16(i < 5 ? (int16_t)(i + 1) : (int16_t)-2);
			for (int b = 0; b < 16; ++b) {
				stream.writeUInt8(0);
			}
		}
		finish(5, start);
	}

	{
		const int64_t start = stream.pos();
		for (int i = 0; i < 2; ++i) {
			stream.writeFloat(1.0f);
			stream.writeFloat(0.0f);
			stream.writeFloat(0.0f);
			stream.writeFloat(0.0f);
			stream.writeFloat(0.0f);
			stream.writeFloat(1.0f);
			stream.writeFloat(0.0f);
			stream.writeFloat(0.0f);
			stream.writeUInt32((uint32_t)i);
			stream.writeUInt32(0);
		}
		finish(6, start);
	}

	{
		const int64_t start = stream.pos();
		for (int i = 0; i < 7; ++i) {
			stream.writeUInt16(i < 6 ? (uint16_t)i : 0);
			stream.writeUInt16(0);
			stream.writeInt32(i * 4);
			stream.writeUInt16(4);
			stream.writeUInt16(i < 6 ? 0 : 1);
			for (int s = 0; s < 4; ++s) {
				stream.writeUInt8(0);
			}
			stream.writeInt32(-1);
		}
		finish(7, start);
	}

	{
		const int64_t start = stream.pos();
		stream.writeInt32(-1);
		for (int b = 0; b < 24; ++b) {
			stream.writeUInt8(0);
		}
		stream.writeInt32(-2);
		for (int b = 0; b < 24; ++b) {
			stream.writeUInt8(0);
		}
		finish(10, start);
	}

	{
		const int quads[7][4] = {{1, 2, 6, 5}, {3, 7, 6, 2}, {4, 5, 6, 7}, {0, 4, 7, 3},
								  {0, 1, 5, 4}, {0, 3, 2, 1}, {1, 2, 6, 5}};
		const int64_t edgeStart = stream.pos();
		// Edge 0 cannot be referenced forwards: a surfedge of 0 is read as the reverse of edge 0.
		stream.writeInt16(0);
		stream.writeInt16(0);
		for (int i = 0; i < 7; ++i) {
			for (int k = 0; k < 4; ++k) {
				stream.writeInt16((int16_t)quads[i][k]);
				stream.writeInt16((int16_t)quads[i][(k + 1) % 4]);
			}
		}
		finish(12, edgeStart);
		const int64_t surfStart = stream.pos();
		for (int i = 1; i <= 28; ++i) {
			stream.writeInt32(i);
		}
		finish(13, surfStart);
	}

	{
		const int64_t start = stream.pos();
		auto writeModel = [&](float minv, float maxv, int first, int count) {
			for (int i = 0; i < 3; ++i) {
				stream.writeFloat(minv);
			}
			for (int i = 0; i < 3; ++i) {
				stream.writeFloat(maxv);
			}
			for (int i = 0; i < 3; ++i) {
				stream.writeFloat(0.0f);
			}
			stream.writeInt32(0);
			stream.writeInt32(-1);
			stream.writeInt32(-1);
			stream.writeInt32(-1);
			stream.writeInt32(1);
			stream.writeInt32(first);
			stream.writeInt32(count);
		};
		writeModel(0.0f, 32.0f, 0, 6);
		writeModel(4.0f, 8.0f, 6, 1);
		finish(14, start);
	}

	stream.seek(0);
	stream.writeUInt32(29);
	for (int i = 0; i < 15; ++i) {
		stream.writeUInt32(lumps[i].offset);
		stream.writeUInt32(lumps[i].len);
	}
}

void writePadded(io::BufferedReadWriteStream &stream, const char *text, int len) {
	int n = 0;
	while (text[n] != '\0') {
		++n;
	}
	for (int i = 0; i < len; ++i) {
		stream.writeUInt8(i < n ? (uint8_t)text[i] : 0);
	}
}

void writeCubeVertices(io::BufferedReadWriteStream &stream) {
	const float v[8][3] = {{0, 0, 0}, {32, 0, 0}, {32, 32, 0}, {0, 32, 0},
						   {0, 0, 32}, {32, 0, 32}, {32, 32, 32}, {0, 32, 32}};
	for (int i = 0; i < 8; ++i) {
		for (int k = 0; k < 3; ++k) {
			stream.writeFloat(v[i][k]);
		}
	}
}

void writeQuake2CubeBsp(io::BufferedReadWriteStream &stream) {
	struct Lump {
		uint32_t offset = 0;
		uint32_t len = 0;
	};
	Lump lumps[19];
	for (int i = 0; i < 8 + 19 * 8; ++i) {
		stream.writeUInt8(0);
	}
	auto finish = [&](int lump, int64_t start) {
		lumps[lump].offset = (uint32_t)start;
		lumps[lump].len = (uint32_t)(stream.pos() - start);
	};
	const int quads[6][4] = {{1, 2, 6, 5}, {3, 7, 6, 2}, {4, 5, 6, 7}, {0, 4, 7, 3}, {0, 1, 5, 4}, {0, 3, 2, 1}};

	{
		const int64_t start = stream.pos();
		writeCubeVertices(stream);
		finish(2, start);
	}
	{
		const int64_t start = stream.pos();
		auto writeTexinfo = [&](const char *name, int flags) {
			for (int i = 0; i < 8; ++i) {
				stream.writeFloat(0.0f);
			}
			stream.writeInt32(flags);
			stream.writeInt32(0);
			writePadded(stream, name, 32);
			stream.writeInt32(-1);
		};
		writeTexinfo("wall", 0);
		writeTexinfo("clip", 0x80);
		finish(5, start);
	}
	{
		const int64_t start = stream.pos();
		for (int i = 0; i < 7; ++i) {
			stream.writeUInt16(0);
			stream.writeInt16(0);
			stream.writeInt32(i < 6 ? i * 4 : 0);
			stream.writeInt16(4);
			stream.writeInt16(i < 6 ? 0 : 1);
			for (int s = 0; s < 4; ++s) {
				stream.writeUInt8(0);
			}
			stream.writeInt32(-1);
		}
		finish(6, start);
	}
	{
		const int64_t edgeStart = stream.pos();
		stream.writeInt16(0);
		stream.writeInt16(0);
		for (int i = 0; i < 6; ++i) {
			for (int k = 0; k < 4; ++k) {
				stream.writeInt16((int16_t)quads[i][k]);
				stream.writeInt16((int16_t)quads[i][(k + 1) % 4]);
			}
		}
		finish(11, edgeStart);
		const int64_t surfStart = stream.pos();
		for (int i = 1; i <= 24; ++i) {
			stream.writeInt32(i);
		}
		finish(12, surfStart);
	}
	{
		const int64_t start = stream.pos();
		auto writeModel = [&](int first, int count) {
			for (int i = 0; i < 9; ++i) {
				stream.writeFloat(0.0f);
			}
			stream.writeInt32(0);
			stream.writeInt32(first);
			stream.writeInt32(count);
		};
		writeModel(0, 6);
		writeModel(6, 1);
		finish(13, start);
	}

	stream.seek(0);
	stream.writeUInt8('I');
	stream.writeUInt8('B');
	stream.writeUInt8('S');
	stream.writeUInt8('P');
	stream.writeUInt32(38);
	for (int i = 0; i < 19; ++i) {
		stream.writeUInt32(lumps[i].offset);
		stream.writeUInt32(lumps[i].len);
	}
}

void expectHollowCube(const scenegraph::SceneGraph &sceneGraph) {
	ASSERT_EQ(1u, sceneGraph.size());
	const scenegraph::SceneGraphNode &node = *sceneGraph.beginModel();
	const voxel::RawVolume *volume = node.volume();
	ASSERT_NE(nullptr, volume);
	const glm::ivec3 origin(glm::round(node.transform(0).localTranslation()));
	auto at = [&](int x, int y, int z) {
		return volume->voxel(x - origin.x, y - origin.y, z - origin.z);
	};
	// Axis-aligned faces land on the positive side of each plane.
	EXPECT_FALSE(voxel::isAir(at(32, 16, 16).getMaterial()));
	EXPECT_TRUE(voxel::isAir(at(16, 16, 16).getMaterial()));
}

} // namespace

class QuakeBSPFormatTest : public AbstractFormatTest {};

TEST_F(QuakeBSPFormatTest, testLoad) {
	util::ScopedVarChange scoped(cfg::VoxformatScale, "0.001");
	testLoad("ufoai.bsp", 3); // hospital bsp from https://ufoai.org/maps/2.6/base/maps/b/
}

TEST_F(QuakeBSPFormatTest, testQuake1SolidShell) {
	util::ScopedVarChange scale(cfg::VoxformatScale, "1");
	io::BufferedReadWriteStream bytes;
	writeSolidCubeBsp(bytes);
	const io::MemoryArchivePtr archive = io::openMemoryArchive();
	ASSERT_TRUE(archive->add("cube.bsp", bytes.getBuffer(), (size_t)bytes.size()));

	QuakeBSPFormat format;
	scenegraph::SceneGraph sceneGraph;
	ASSERT_TRUE(format.load("cube.bsp", archive, sceneGraph, testLoadCtx));
	ASSERT_EQ(1u, sceneGraph.size());

	const scenegraph::SceneGraphNode &node = *sceneGraph.beginModel();
	const voxel::RawVolume *volume = node.volume();
	ASSERT_NE(nullptr, volume);
	const glm::ivec3 origin(glm::round(node.transform(0).localTranslation()));
	auto at = [&](int x, int y, int z) {
		return volume->voxel(x - origin.x, y - origin.y, z - origin.z);
	};
	EXPECT_FALSE(voxel::isAir(at(31, 16, 16).getMaterial()));
	EXPECT_FALSE(voxel::isAir(at(30, 16, 16).getMaterial()));
	EXPECT_FALSE(voxel::isAir(at(0, 16, 16).getMaterial()));
	EXPECT_TRUE(voxel::isAir(at(29, 16, 16).getMaterial()));
	EXPECT_TRUE(voxel::isAir(at(16, 16, 16).getMaterial()));
}

TEST_F(QuakeBSPFormatTest, testQuake2Cube) {
	util::ScopedVarChange scale(cfg::VoxformatScale, "1");
	util::ScopedVarChange fill(cfg::VoxformatFillHollow, "false");
	io::BufferedReadWriteStream bytes;
	writeQuake2CubeBsp(bytes);
	const io::MemoryArchivePtr archive = io::openMemoryArchive();
	ASSERT_TRUE(archive->add("cube.bsp", bytes.getBuffer(), (size_t)bytes.size()));

	QuakeBSPFormat format;
	scenegraph::SceneGraph sceneGraph;
	ASSERT_TRUE(format.load("cube.bsp", archive, sceneGraph, testLoadCtx));
	expectHollowCube(sceneGraph);
}

TEST_F(QuakeBSPFormatTest, testGoldSrcStyleMaskedMiptex) {
	util::ScopedVarChange scale(cfg::VoxformatScale, "1");
	for (const bool masked : {false, true}) {
		io::BufferedReadWriteStream bytes;
		writeSolidCubeBsp(bytes, masked ? "{wall" : "wall", 255);
		const io::MemoryArchivePtr archive = io::openMemoryArchive();
		ASSERT_TRUE(archive->add("cube.bsp", bytes.getBuffer(), (size_t)bytes.size()));

		QuakeBSPFormat format;
		scenegraph::SceneGraph sceneGraph;
		EXPECT_EQ(!masked, format.load("cube.bsp", archive, sceneGraph, testLoadCtx));
	}
}

} // namespace voxelformat
