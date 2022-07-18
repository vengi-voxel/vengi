/**
 * @file
 */

#include "SMFormat.h"
#include "core/Bits.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/collection/Map.h"
#include "io/BufferedReadWriteStream.h"
#include "io/ZipArchive.h"
#include "io/ZipReadStream.h"
#include "voxel/Voxel.h"
#include <fcntl.h>

namespace voxelformat {

namespace priv {
constexpr int segments = 16;
constexpr int volumeVoxelCount = segments * segments * segments;
constexpr int segmentHeaderSize = 26;
constexpr int blocks = 32;
constexpr int maxSegmentDataCompressedSize = ((blocks * blocks * blocks) * 3 / 2) - segmentHeaderSize;
constexpr int planeBlocks = blocks * blocks;

constexpr core::RGBA paletteColors[] = {
	core::RGBA(100, 103, 105), core::RGBA(10, 10, 12),	core::RGBA(220, 220, 220), core::RGBA(148, 10, 196),
	core::RGBA(10, 84, 196),   core::RGBA(69, 177, 42), core::RGBA(196, 172, 10),  core::RGBA(196, 68, 10),
	core::RGBA(196, 10, 10),   core::RGBA(142, 75, 49), core::RGBA(80, 82, 84),	   core::RGBA(10, 196, 140),
	core::RGBA(196, 10, 150),
};

} // namespace priv

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::debug("Error: " CORE_STRINGIFY(read) " at " SDL_FILE ":%i", SDL_LINE);                                    \
		return false;                                                                                                  \
	}

#define wrapBool(read)                                                                                                 \
	if (!(read)) {                                                                                                     \
		Log::debug("Error: " CORE_STRINGIFY(read) " at " SDL_FILE ":%i", SDL_LINE);                                    \
		return false;                                                                                                  \
	}

static bool readIvec3(io::SeekableReadStream &stream, glm::ivec3 &v) {
	if (stream.readInt32BE(v.x) == -1) {
		return false;
	}
	if (stream.readInt32BE(v.y) == -1) {
		return false;
	}
	if (stream.readInt32BE(v.z) == -1) {
		return false;
	}
	return true;
}

// https://starmadepedia.net/wiki/ID_list
static const struct BlockPalIdx {
	int blockId;
	int palIdx;
} BLOCKPAL[]{
	{5, 0},	   {69, 3},	  {70, 9},	 {75, 1},	{76, 8},   {77, 4},	  {78, 5},	 {79, 6},	{81, 2},   {232, 0},
	{254, 10}, {263, 0},  {264, 1},	 {265, 8},	{266, 3},  {267, 4},  {268, 5},	 {269, 9},	{270, 6},  {271, 2},
	{293, 0},  {294, 3},  {295, 9},	 {296, 1},	{297, 8},  {298, 4},  {299, 5},	 {300, 6},	{301, 2},  {302, 0},
	{303, 3},  {304, 9},  {305, 1},	 {306, 8},	{307, 4},  {308, 5},  {309, 6},	 {310, 2},	{311, 0},  {312, 1},
	{313, 8},  {314, 3},  {315, 4},	 {316, 5},	{317, 9},  {318, 6},  {319, 2},	 {320, 0},	{321, 1},  {322, 8},
	{323, 3},  {324, 4},  {325, 5},	 {326, 9},	{327, 6},  {328, 2},  {348, 0},	 {357, 0},	{369, 1},  {370, 8},
	{371, 3},  {372, 4},  {373, 5},	 {374, 9},	{375, 6},  {376, 2},  {377, 1},	 {378, 8},	{379, 3},  {380, 4},
	{381, 5},  {382, 9},  {383, 6},	 {384, 2},	{385, 1},  {386, 8},  {387, 3},	 {388, 4},	{389, 5},  {391, 6},
	{392, 2},  {393, 1},  {394, 8},	 {395, 3},	{396, 4},  {397, 5},  {398, 6},	 {400, 2},	{401, 0},  {402, 0},
	{403, 9},  {404, 9},  {426, 7},	 {427, 7},	{428, 7},  {429, 7},  {430, 7},	 {431, 7},	{432, 7},  {433, 7},
	{434, 7},  {435, 7},  {436, 6},	 {437, 6},	{438, 5},  {439, 5},  {507, 2},	 {508, 2},	{509, 2},  {510, 2},
	{511, 2},  {512, 8},  {513, 8},	 {514, 8},	{515, 8},  {516, 8},  {517, 7},	 {518, 7},	{519, 7},  {520, 7},
	{521, 7},  {522, 6},  {523, 6},	 {524, 6},	{525, 6},  {526, 6},  {527, 5},	 {528, 5},	{529, 5},  {530, 5},
	{531, 5},  {532, 4},  {533, 4},	 {534, 4},	{535, 4},  {536, 4},  {537, 3},	 {538, 3},	{539, 3},  {540, 3},
	{541, 3},  {593, 1},  {594, 1},	 {595, 1},	{596, 1},  {597, 1},  {598, 0},	 {599, 0},	{600, 0},  {601, 0},
	{602, 0},  {603, 1},  {604, 1},	 {605, 1},	{606, 1},  {607, 1},  {608, 2},	 {609, 2},	{610, 2},  {611, 2},
	{612, 2},  {613, 3},  {614, 3},	 {615, 3},	{616, 3},  {617, 3},  {618, 4},	 {619, 4},	{620, 4},  {621, 4},
	{622, 4},  {623, 5},  {624, 5},	 {625, 5},	{626, 5},  {627, 5},  {628, 6},	 {629, 6},	{630, 6},  {631, 6},
	{632, 6},  {633, 7},  {634, 7},	 {635, 7},	{636, 7},  {637, 7},  {638, 8},	 {639, 8},	{640, 8},  {641, 8},
	{642, 8},  {643, 9},  {644, 9},	 {645, 9},	{646, 9},  {647, 9},  {648, 6},	 {649, 6},	{650, 6},  {651, 5},
	{652, 5},  {653, 5},  {690, 9},	 {691, 9},	{692, 9},  {693, 9},  {694, 9},	 {698, 0},	{699, 0},  {700, 0},
	{701, 0},  {702, 0},  {703, 0},	 {704, 0},	{705, 0},  {706, 0},  {707, 0},	 {708, 0},	{709, 0},  {710, 0},
	{711, 0},  {712, 0},  {713, 0},	 {714, 0},	{715, 0},  {716, 0},  {717, 0},	 {718, 0},	{719, 0},  {720, 0},
	{721, 0},  {722, 0},  {723, 0},	 {724, 0},	{725, 0},  {726, 0},  {727, 0},	 {728, 0},	{729, 0},  {730, 0},
	{731, 0},  {732, 0},  {733, 0},	 {734, 0},	{735, 0},  {736, 0},  {737, 0},	 {738, 0},	{739, 0},  {740, 0},
	{741, 0},  {742, 0},  {743, 0},	 {744, 0},	{745, 0},  {746, 0},  {747, 0},	 {748, 0},	{749, 0},  {750, 0},
	{751, 0},  {752, 0},  {753, 0},	 {754, 0},	{755, 0},  {756, 0},  {757, 0},	 {758, 0},	{759, 0},  {760, 0},
	{761, 0},  {762, 0},  {763, 0},	 {764, 0},	{765, 0},  {766, 0},  {767, 0},	 {768, 0},	{769, 0},  {770, 0},
	{771, 0},  {772, 0},  {773, 0},	 {774, 0},	{775, 0},  {776, 0},  {777, 0},	 {778, 0},	{779, 0},  {780, 0},
	{781, 0},  {782, 0},  {783, 0},	 {784, 0},	{785, 0},  {786, 0},  {787, 0},	 {788, 0},	{789, 0},  {790, 0},
	{791, 0},  {792, 0},  {793, 0},	 {794, 0},	{795, 0},  {796, 0},  {797, 0},	 {798, 0},	{799, 0},  {800, 0},
	{801, 0},  {802, 0},  {803, 0},	 {804, 0},	{805, 0},  {806, 0},  {807, 0},	 {808, 0},	{809, 0},  {810, 0},
	{811, 0},  {812, 0},  {813, 0},	 {814, 0},	{815, 0},  {816, 0},  {817, 0},	 {818, 10}, {819, 10}, {820, 10},
	{821, 10}, {822, 10}, {823, 10}, {824, 10}, {825, 10}, {826, 10}, {827, 10}, {828, 10}, {829, 10}, {830, 10},
	{831, 10}, {832, 10}, {833, 10}, {834, 10}, {835, 10}, {836, 10}, {837, 10}, {838, 10}, {839, 10}, {840, 10},
	{841, 10}, {851, 10}, {852, 10}, {853, 10}, {854, 10}, {855, 10}, {856, 10}, {857, 10}, {858, 10}, {859, 10},
	{860, 10}, {861, 10}, {862, 10}, {863, 10}, {864, 10}, {868, 11}, {869, 11}, {870, 11}, {871, 11}, {872, 11},
	{873, 11}, {874, 11}, {875, 11}, {876, 11}, {877, 11}, {878, 11}, {879, 11}, {880, 11}, {881, 11}, {882, 11},
	{883, 11}, {884, 11}, {885, 11}, {886, 11}, {887, 11}, {902, 12}, {903, 12}, {904, 12}, {905, 12}, {906, 12},
	{907, 12}, {908, 12}, {909, 12}, {910, 12}, {911, 12}, {912, 12}, {913, 12}, {914, 12}, {915, 12}, {916, 12},
	{917, 12}, {918, 12}, {919, 12}, {920, 12}, {921, 12}};

bool SMFormat::loadGroups(const core::String &filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph) {
	io::ZipArchive archive;
	if (!archive.open(&stream)) {
		Log::error("Failed to load zip archive from %s", filename.c_str());
		return false;
	}
	core::Map<int, int> blockPal;
	for (int i = 0; i < lengthof(BLOCKPAL); ++i) {
		blockPal.put(BLOCKPAL[i].blockId, BLOCKPAL[i].palIdx);
	}
	const io::ZipArchiveFiles &files = archive.files();
	for (const io::FilesystemEntry &e : files) {
		const core::String &extension = core::string::extractExtension(e.name);
		const bool isSmd3 = extension == "smd3";
		const bool isSmd2 = extension == "smd2";
		if (isSmd2 || isSmd3) {
			io::BufferedReadWriteStream modelStream((int64_t)e.size);
			if (!archive.load(e.name, modelStream)) {
				Log::warn("Failed to load zip archive entry %s", e.name.c_str());
				continue;
			}
			if (modelStream.seek(0) == -1) {
				Log::error("Failed to seek back to the start of the stream for %s", e.name.c_str());
				continue;
			}
			if (isSmd3) {
				if (!readSmd3(modelStream, sceneGraph, blockPal)) {
					Log::warn("Failed to load %s from %s", e.name.c_str(), filename.c_str());
				}
			}
			// TODO: read *.smd2
		}
	}
	return !sceneGraph.empty();
}

bool SMFormat::readSmd3(io::SeekableReadStream &stream, SceneGraph &sceneGraph, const core::Map<int, int>& blockPal) {
	uint32_t version;
	wrap(stream.readUInt32BE(version))

	core::Map<uint16_t, uint16_t> segmentsMap;

	for (int i = 0; i < priv::volumeVoxelCount; i++) {
		uint16_t segmentId;
		wrap(stream.readUInt16BE(segmentId))
		uint16_t segmentSize;
		wrap(stream.readUInt16BE(segmentSize))
		if (segmentId > 0) {
			Log::debug("segment %i with size: %i", (int)segmentId, (int)segmentSize);
			segmentsMap.put(segmentId, segmentSize);
		}
	}
	while (!stream.eos()) {
		if (!readSegment(stream, sceneGraph, blockPal)) {
			Log::error("Failed to read segment");
			return false;
		}
	}

	return true;
}

static constexpr glm::ivec3 posByIndex(uint32_t blockIndex) {
	const int z = (int)blockIndex / priv::planeBlocks;
	const int divR = (int)blockIndex % priv::planeBlocks;
	const int y = divR / priv::blocks;
	const int x = divR % priv::blocks;
	return glm::ivec3(x, y, z);
}

bool SMFormat::readSegment(io::SeekableReadStream &stream, SceneGraph &sceneGraph, const core::Map<int, int>& blockPal) {
	const int64_t startHeader = stream.pos();
	Log::debug("read segment");
	voxel::Palette palette;
	for (int i = 0; i < lengthof(priv::paletteColors); ++i) {
		palette.colors[i] = priv::paletteColors[i];
	}
	palette.colorCount = lengthof(priv::paletteColors);

	uint8_t version;
	wrap(stream.readUInt8(version))
	Log::debug("version: %i", (int)version);

	uint64_t timestamp;
	wrap(stream.readUInt64BE(timestamp))

	glm::ivec3 position;
	wrapBool(readIvec3(stream, position))
	Log::debug("pos: %i:%i:%i", position.x, position.y, position.z);

	bool hasValidData = stream.readBool();
	Log::debug("hasValiddata: %i", (int)hasValidData);

	uint32_t compressedSize;
	wrap(stream.readUInt32BE(compressedSize))

	if (!hasValidData) {
		stream.seek(startHeader + priv::maxSegmentDataCompressedSize);
		return true;
	}

	core_assert(stream.pos() - startHeader == priv::segmentHeaderSize);

	io::ZipReadStream blockDataStream(stream, (int)compressedSize);

	const voxel::Region region(position, position + (priv::blocks - 1));
	voxel::RawVolume *volume = new voxel::RawVolume(region);

	SceneGraphNode node;
	node.setVolume(volume, true);
	node.setPalette(palette);
	bool empty = true;
	int index = -1;
	while (!blockDataStream.eos()) {
		++index;
		uint8_t buf[3];
		// byte orientation : 3
		// byte isActive: 1
		// byte hitpoints: 9
		// ushort blockId: 11
		wrap(blockDataStream.readUInt8(buf[0]))
		wrap(blockDataStream.readUInt8(buf[1]))
		wrap(blockDataStream.readUInt8(buf[2]))
		const uint32_t blockData = buf[0] | (buf[1] << 8) | (buf[2] << 16);
		if (blockData == 0) {
			continue;
		}
		const uint32_t blockId = core::bits(blockData, 0, 11);
		// const uint32_t hitpoints = core::bits(blockData, 11, 9);
		// const uint32_t active = core::bits(blockData, 20, 1);
		// const uint32_t orientation = core::bits(blockData, 21, 3);
		auto palIter = blockPal.find((int)blockId);
		uint8_t palIndex = 0;
		if (palIter == blockPal.end()) {
			Log::warn("Skip block id %i", (int)blockId);
		} else {
			palIndex = palIter->value;
		}

		glm::ivec3 pos = position + posByIndex(index);

		volume->setVoxel(pos, voxel::createVoxel(voxel::VoxelType::Generic, palIndex));
		empty = false;
	}

	core_assert(stream.pos() - startHeader == (int)compressedSize + priv::segmentHeaderSize);

	stream.seek(startHeader + priv::maxSegmentDataCompressedSize + priv::segmentHeaderSize);
	if (empty) {
		return true;
	}

	sceneGraph.emplace(core::move(node));

	return true;
}

#undef wrap
#undef wrapBool

} // namespace voxelformat
