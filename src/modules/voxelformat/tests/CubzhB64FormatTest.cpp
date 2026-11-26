/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "core/ConfigVar.h"
#include "io/Base64.h"
#include "io/BufferedReadWriteStream.h"
#include "io/MemoryArchive.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "util/VarUtil.h"
#include "voxel/RawVolume.h"
#include "voxelformat/private/cubzh/CubzhB64Format.h"

namespace voxelformat {

class CubzhB64FormatTest : public AbstractFormatTest {};

TEST_F(CubzhB64FormatTest, testLoad) {
	testLoad("hubmap.b64");
}

TEST_F(CubzhB64FormatTest, testPalettePerBlockChunk) {
	util::ScopedVarChange createPalette(cfg::VoxelCreatePalette, true);
	io::BufferedReadWriteStream decoded;
	ASSERT_TRUE(decoded.writeUInt8(3)); // world version
	for (int chunk = 0; chunk < 2; ++chunk) {
		ASSERT_TRUE(decoded.writeUInt8(3)); // blocks chunk
		ASSERT_TRUE(decoded.writeUInt32(2 + 4 * 8));
		ASSERT_TRUE(decoded.writeUInt16(4));
		for (int x = 0; x < 4; ++x) {
			ASSERT_TRUE(decoded.writePascalStringUInt16LE("0"));
			ASSERT_TRUE(decoded.writeUInt8(1)); // add block
			ASSERT_TRUE(decoded.writeUInt32(color::RGBA((uint8_t)(40 + x), (uint8_t)(chunk * 128), 16)));
		}
	}
	ASSERT_NE(-1, decoded.seek(0));
	const core::String encoded = io::Base64::encode(decoded);
	const io::MemoryArchivePtr archive = io::openMemoryArchive();
	ASSERT_TRUE(archive->add("block-palettes.b64", reinterpret_cast<const uint8_t *>(encoded.c_str()), encoded.size()));
	CubzhB64Format format;
	scenegraph::SceneGraph graph;
	ASSERT_TRUE(format.load("block-palettes.b64", archive, graph, testLoadCtx));
	ASSERT_EQ(2u, graph.size(scenegraph::SceneGraphNodeType::Model));
	int chunk = 0;
	for (auto iter = graph.beginModel(); iter != graph.end(); ++iter, ++chunk) {
		const scenegraph::SceneGraphNode &node = *iter;
		EXPECT_EQ(4, node.palette().colorCount());
		for (int x = 0; x < 4; ++x) {
			const voxel::Voxel voxel = node.volume()->voxel(x, 0, 0);
			EXPECT_EQ(color::RGBA((uint8_t)(40 + x), (uint8_t)(chunk * 128), 16), node.palette().color(voxel.getColor()));
		}
	}
}

} // namespace voxelformat
