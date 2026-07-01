/**
 * @file
 */

#include "SMFormat.h"
#include "color/Color.h"
#include "color/ColorUtil.h"
#include "core/ArrayLength.h"
#include "core/Bits.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/Map.h"
#include "io/Archive.h"
#include "io/Stream.h"
#include "io/ZipArchive.h"
#include "io/ZipReadStream.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include <SDL3/SDL_stdinc.h>
#include <glm/gtc/quaternion.hpp>

// https://starmadepedia.net/wiki/ID_list
#include "SMPalette.h"

namespace voxelformat {

namespace priv {
constexpr int segments = 16;
constexpr int maxSegments = segments * segments * segments;

// SMD2: 16x16x16 blocks per chunk, 5120 bytes per chunk slot
constexpr int smd2Blocks = 16;
constexpr int smd2ChunkDataSize = 5120;
constexpr int smd2SegmentHeaderSize = 25; // 8 timestamp + 12 position + 4 dataLength + 1 type

// SMD3: 32x32x32 blocks per segment
constexpr int smd3Blocks = 32;
constexpr int smd3SegmentHeaderSize =
	26; // 1 segmentVersion + 8 timestamp + 12 position + 1 hasValidData + 4 compressedSize
constexpr int smd3SegmentDataSize = ((smd3Blocks * smd3Blocks * smd3Blocks) * 3 / 2) - smd3SegmentHeaderSize;

} // namespace priv

struct DockEntry {
	core::String subfolder;
	glm::ivec3 position{0};
	glm::mat3 rotation{1.0f};
};

// Parse dock entries from meta.smbpm - extracts attachment positions and rotations
static void parseDockEntries(io::SeekableReadStream &stream, core::DynamicArray<DockEntry> &docks) {
	const int64_t size = stream.remaining();
	if (size <= 0) {
		return;
	}
	core::DynamicArray<uint8_t> buf((size_t)size);
	if (stream.read(buf.data(), (size_t)size) != (int)size) {
		return;
	}
	const uint8_t *data = buf.data();
	const size_t len = (size_t)size;

	// Search for ATTACHED_ paths and extract dock position + rotation
	size_t pos = 0;
	while (pos < len) {
		// Find next "ATTACHED_" occurrence
		const uint8_t *found = nullptr;
		for (size_t i = pos; i + 9 < len; ++i) {
			if (SDL_memcmp(&data[i], "ATTACHED_", 9) == 0) {
				found = &data[i];
				break;
			}
		}
		if (!found) {
			break;
		}
		const size_t attachIdx = (size_t)(found - data);

		// Find the 2-byte length prefix for the full path (Java writeUTF)
		core::String subfolder;
		size_t afterPath = 0;
		for (size_t back = 2; back < 60 && attachIdx >= back; ++back) {
			const size_t prefixOff = attachIdx - back;
			const uint16_t strLen = (uint16_t)((data[prefixOff] << 8) | data[prefixOff + 1]);
			if (strLen > 0 && strLen < 200 && prefixOff + 2 + strLen <= len) {
				const char *str = (const char *)&data[prefixOff + 2];
				// Verify it contains ATTACHED_
				if (SDL_memcmp(str + (back - 2), "ATTACHED_", 9) == 0) {
					subfolder = core::String(str, strLen);
					afterPath = prefixOff + 2 + strLen;
					break;
				}
			}
		}
		if (subfolder.empty()) {
			pos = attachIdx + 1;
			continue;
		}

		// Find the two 0xf6 markers: first is parent dock pos, second is attached dock pos
		DockEntry entry;
		entry.subfolder = subfolder;
		glm::ivec3 parentDock{0};
		glm::ivec3 attachedDock{0};
		int markersFound = 0;
		for (size_t i = afterPath; i + 13 < len && i < afterPath + 500; ++i) {
			if (data[i] == 0xf6) {
				const size_t p = i + 1;
				if (p + 12 > len) {
					break;
				}
				glm::ivec3 v;
				v.x = (int32_t)((data[p] << 24) | (data[p + 1] << 16) | (data[p + 2] << 8) | data[p + 3]);
				v.y = (int32_t)((data[p + 4] << 24) | (data[p + 5] << 16) | (data[p + 6] << 8) | data[p + 7]);
				v.z = (int32_t)((data[p + 8] << 24) | (data[p + 9] << 16) | (data[p + 10] << 8) | data[p + 11]);
				if (markersFound == 0) {
					parentDock = v;
				} else {
					attachedDock = v;
				}
				++markersFound;
				i = p + 11; // skip past this marker's data
				if (markersFound == 2) {
					break;
				}
			}
		}
		if (markersFound >= 2) {
			entry.position = parentDock - attachedDock;
			Log::debug("Dock entry: %s parent=(%i,%i,%i) attached=(%i,%i,%i) offset=(%i,%i,%i)",
					   entry.subfolder.c_str(), parentDock.x, parentDock.y, parentDock.z, attachedDock.x,
					   attachedDock.y, attachedDock.z, entry.position.x, entry.position.y, entry.position.z);
			docks.push_back(entry);
		} else if (markersFound == 1) {
			entry.position = parentDock;
			Log::debug("Dock entry: %s pos=(%i,%i,%i) (single marker)", entry.subfolder.c_str(), entry.position.x,
					   entry.position.y, entry.position.z);
			docks.push_back(entry);
		}
		pos = attachIdx + 1;
	}
}

// Find dock entry for a given file path (e.g. "Ship/ATTACHED_0/DATA/file.smd3")
static const DockEntry *findDockEntry(const core::DynamicArray<DockEntry> &docks, const core::String &filePath) {
	for (const DockEntry &dock : docks) {
		if (filePath.contains(dock.subfolder)) {
			return &dock;
		}
	}
	return nullptr;
}

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Error: " CORE_STRINGIFY(read) " at " CORE_FILE ":%i", CORE_LINE);                                  \
		return false;                                                                                                  \
	}

#define wrapBool(read)                                                                                                 \
	if (!(read)) {                                                                                                     \
		Log::error("Error: " CORE_STRINGIFY(read) " at " CORE_FILE ":%i", CORE_LINE);                                  \
		return false;                                                                                                  \
	}

static bool readIvec3(io::SeekableReadStream &stream, glm::ivec3 &v) {
	if (stream.readInt32BE(v.x) == -1) {
		Log::error("failed to read int vector x component");
		return false;
	}
	if (stream.readInt32BE(v.y) == -1) {
		Log::error("failed to read int vector y component");
		return false;
	}
	if (stream.readInt32BE(v.z) == -1) {
		Log::error("failed to read int vector z component");
		return false;
	}
	return true;
}

bool SMFormat::loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive,
							  scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
							  const LoadContext &ctx) {
	core::Map<int, int> blockPal;
	for (int i = 0; i < lengthof(BLOCKCOLOR); ++i) {
		blockPal.put(BLOCKCOLOR[i].blockId, palette.getClosestMatch(BLOCKCOLOR[i].color));
	}
	const core::String &extension = core::string::extractExtension(filename);
	if (extension == "smd3") {
		core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
		if (!stream) {
			Log::error("Could not load file %s", filename.c_str());
			return false;
		}
		return readSmd3(*stream, sceneGraph, blockPal, {0, 0, 0}, palette);
	} else if (extension == "smd2") {
		core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
		if (!stream) {
			Log::error("Could not load file %s", filename.c_str());
			return false;
		}
		return readSmd2(*stream, sceneGraph, blockPal, {0, 0, 0}, palette);
	} else if (extension == "sment") {
		core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
		if (!stream) {
			Log::error("Could not load file %s", filename.c_str());
			return false;
		}
		io::ArchivePtr zipArchive = io::openZipArchive(stream);
		io::ArchiveFiles files;
		zipArchive->list("*.smd3,*.smd2", files);
		if (files.empty()) {
			Log::error("No smd3 or smd2 files found in %s", filename.c_str());
			return false;
		}
		// Parse meta.smbpm for dock positions/rotations of attached entities
		core::DynamicArray<DockEntry> docks;
		{
			io::ArchiveFiles metaFiles;
			zipArchive->list("*.smbpm", metaFiles);
			for (const io::FilesystemEntry &m : metaFiles) {
				// Only parse the root meta file, not those inside ATTACHED_N dirs
				if (m.fullPath.contains("ATTACHED_")) {
					continue;
				}
				core::ScopedPtr<io::SeekableReadStream> metaStream(zipArchive->readStream(m.fullPath));
				if (metaStream) {
					parseDockEntries(*metaStream, docks);
				}
			}
		}
		for (const io::FilesystemEntry &e : files) {
			const core::String &fileExt = core::string::extractExtension(e.name);
			const bool isSmd3 = fileExt == "smd3";
			const bool isSmd2 = fileExt == "smd2";
			if (isSmd2 || isSmd3) {
				// position is encoded in the filename
				// ENTITY_SHIP_Rexio_1686826017103.0.0.0.smd3
				// split on '.' and take parts from the end: [l-4].[l-3].[l-2].ext
				glm::ivec3 position(0);
				core::DynamicArray<core::String> parts;
				core::string::splitString(e.name, parts, ".");
				const int l = (int)parts.size();
				if (l >= 4) {
					position.x = core::string::toInt(parts[l - 4]) * priv::segments;
					position.y = core::string::toInt(parts[l - 3]) * priv::segments;
					position.z = core::string::toInt(parts[l - 2]) * priv::segments;
				}
				core::ScopedPtr<io::SeekableReadStream> modelStream(zipArchive->readStream(e.fullPath));
				if (!modelStream) {
					Log::warn("Failed to load zip archive entry %s", e.fullPath.c_str());
					continue;
				}
				const DockEntry *dock = findDockEntry(docks, e.fullPath);
				const int nodesBefore = (int)sceneGraph.nodeSize();
				if (isSmd3) {
					if (!readSmd3(*modelStream, sceneGraph, blockPal, position, palette)) {
						Log::warn("Failed to load %s from %s", e.fullPath.c_str(), filename.c_str());
					}
				} else if (isSmd2) {
					if (!readSmd2(*modelStream, sceneGraph, blockPal, position, palette)) {
						Log::warn("Failed to load %s from %s", e.fullPath.c_str(), filename.c_str());
					}
				}
				if (dock) {
					const scenegraph::SceneGraphKeyFrame keyFrame;
					for (auto iter = sceneGraph.beginModel(); iter != sceneGraph.end(); ++iter) {
						if ((*iter).id() < nodesBefore) {
							continue;
						}
						scenegraph::SceneGraphNode &node = *iter;
						scenegraph::SceneGraphKeyFrame &kf = node.keyFrame(0);
						scenegraph::SceneGraphTransform &transform = kf.transform();
						transform.setLocalTranslation(glm::vec3(dock->position));
						const glm::quat orientation = glm::quat_cast(dock->rotation);
						transform.setLocalOrientation(orientation);
					}
				}
			}
		}
	}
	return !sceneGraph.empty();
}

bool SMFormat::readSmd2(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph,
						const core::Map<int, int> &blockPal, const glm::ivec3 &position,
						const palette::Palette &palette) {
	uint32_t version;
	wrap(stream.readUInt32BE(version))

	// SMD2 segment index: 16*16*16 entries of (int32 offset, int32 size)
	for (int i = 0; i < priv::maxSegments; i++) {
		int32_t segmentOffset;
		wrap(stream.readInt32BE(segmentOffset))
		int32_t segmentSize;
		wrap(stream.readInt32BE(segmentSize))
	}
	// SMD2 timestamp table: 16*16*16 entries of int64
	for (int i = 0; i < priv::maxSegments; i++) {
		uint64_t timestamp;
		wrap(stream.readUInt64BE(timestamp))
	}
	while (!stream.eos()) {
		if (!readSegment(stream, sceneGraph, blockPal, version, 2, palette)) {
			Log::error("Failed to read segment");
			return false;
		}
	}

	return true;
}

bool SMFormat::readSmd3(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph,
						const core::Map<int, int> &blockPal, const glm::ivec3 &position,
						const palette::Palette &palette) {
	uint32_t version;
	wrap(stream.readUInt32BE(version))

	core::Map<uint16_t, uint16_t> segmentsMap;

	for (int i = 0; i < priv::maxSegments; i++) {
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
		if (!readSegment(stream, sceneGraph, blockPal, version, 3, palette)) {
			Log::error("Failed to read segment");
			return false;
		}
	}

	return true;
}

static glm::ivec3 posByIndex(uint32_t blockIndex, int blocks) {
	const int planeBlocks = blocks * blocks;
	const int z = (int)blockIndex / planeBlocks;
	const int divR = (int)blockIndex % planeBlocks;
	const int y = divR / blocks;
	const int x = divR % blocks;
	return glm::ivec3(x, y, z);
}

size_t SMFormat::loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
							 const LoadContext &ctx) {
	for (int i = 0; i < lengthof(BLOCKCOLOR); ++i) {
		uint8_t index = 0;
		const color::RGBA rgba = BLOCKCOLOR[i].color;
		if (!palette.tryAdd(rgba, true, &index)) {
			continue;
		}
		for (int j = 0; j < lengthof(BLOCKEMITCOLOR); ++j) {
			if (BLOCKEMITCOLOR[j].blockId != BLOCKCOLOR[i].blockId) {
				continue;
			}
			const color::RGBA emit = BLOCKEMITCOLOR[j].color;
			const float factor = color::getDistance(emit, rgba, color::Distance::HSB);
			palette.setEmit(index, 1.0f - factor);
		}
	}
	return palette.size();
}

bool SMFormat::readSegment(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph,
						   const core::Map<int, int> &blockPal, int headerVersion, int fileVersion,
						   const palette::Palette &palette) {
	const int64_t startHeader = stream.pos();
	const bool isSmd2 = fileVersion == 2;
	const int blocks = isSmd2 ? priv::smd2Blocks : priv::smd3Blocks;
	const int segmentTotalSize =
		isSmd2 ? priv::smd2ChunkDataSize : (priv::smd3SegmentDataSize + priv::smd3SegmentHeaderSize);
	Log::debug("read segment (fileVersion=%i, blocks=%i)", fileVersion, blocks);

	if (headerVersion != 0) {
		uint8_t segmentVersion;
		wrap(stream.readUInt8(segmentVersion))
		Log::debug("segmentVersion: %i", (int)segmentVersion);
	}

	uint64_t timestamp;
	wrap(stream.readUInt64BE(timestamp))

	glm::ivec3 segmentPosition;
	wrapBool(readIvec3(stream, segmentPosition))
	Log::debug("segmentPosition: %i:%i:%i", segmentPosition.x, segmentPosition.y, segmentPosition.z);

	bool hasValidData;
	uint32_t compressedSize;
	if (headerVersion == 0) {
		uint8_t segmentType;
		wrap(stream.readUInt8(segmentType))
		int32_t dataLength;
		wrap(stream.readInt32BE(dataLength))
		hasValidData = dataLength > 0;
		compressedSize = dataLength;
	} else { // Valid as of 0.1867, smd file version 1
		hasValidData = stream.readBool();
		wrap(stream.readUInt32BE(compressedSize))
	}
	Log::debug("hasValidData: %i", (int)hasValidData);

	if (!hasValidData) {
		stream.seek(startHeader + segmentTotalSize);
		return true;
	}

	core_assert(stream.pos() - startHeader == (isSmd2 ? priv::smd2SegmentHeaderSize : priv::smd3SegmentHeaderSize));

	io::ZipReadStream blockDataStream(stream, (int)compressedSize);

	const voxel::Region region(segmentPosition, segmentPosition + (blocks - 1));
	voxel::RawVolume *volume = new voxel::RawVolume(region);

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(volume);
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
		// SMD2 uses big-endian byte assembly, SMD3 uses little-endian
		const uint32_t blockData =
			isSmd2 ? ((buf[0] << 16) | (buf[1] << 8) | buf[2]) : (buf[0] | (buf[1] << 8) | (buf[2] << 16));
		if (blockData == 0u) {
			continue;
		}
		const uint32_t blockId = core::bits(blockData, 0, 11);
		if (blockId == 0u) {
			continue;
		}
		// const uint32_t hitpoints = core::bits(blockData, 11, 9);
		// const uint32_t active = core::bits(blockData, 20, 1);
		// const uint32_t orientation = core::bits(blockData, 21, 3);
		auto palIter = blockPal.find((int)blockId);
		uint8_t palIndex = 0;
		if (palIter == blockPal.end()) {
			Log::trace("Skip block id %i", (int)blockId);
		} else {
			palIndex = palIter->value;
		}

		glm::ivec3 pos = segmentPosition + posByIndex(index, blocks);

		volume->setVoxel(pos, voxel::createVoxel(palette, palIndex));
		empty = false;
	}

	stream.seek(startHeader + segmentTotalSize);
	if (empty) {
		return true;
	}

	sceneGraph.emplace(core::move(node));

	return true;
}

#undef wrap
#undef wrapBool

} // namespace voxelformat
