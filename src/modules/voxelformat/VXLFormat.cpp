/**
 * @file
 */

#include "VXLFormat.h"
#include "app/App.h"
#include "core/Assert.h"
#include "core/Color.h"
#include "core/Common.h"
#include "core/GameConfig.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "core/collection/Buffer.h"
#include "core/collection/DynamicArray.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "io/Stream.h"
#include "voxel/MaterialColor.h"
#include "voxel/Palette.h"
#include "voxel/PaletteLookup.h"
#include "voxelformat/SceneGraph.h"

#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/matrix.hpp>

namespace voxelformat {

#define wrap(read) \
	if ((read) != 0) { \
		Log::error("Error: " CORE_STRINGIFY(read) " at " SDL_FILE ":%i", SDL_LINE); \
		return false; \
	}

#define wrapBool(read) \
	if (!(read)) { \
		Log::error("Error: " CORE_STRINGIFY(read) " at " SDL_FILE ":%i", SDL_LINE); \
		return false; \
	}

namespace priv {
static const float Scale = 1.0f / 12.0f;
}

// https://stackoverflow.com/a/71168853/774082
// convert from left handed coordinate system (z up) to right handed glm coordinate system (y up)
glm::mat4 VXLFormat::switchYAndZ(const glm::mat4 &in) {
	static const glm::mat4 mat{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f};
	return mat * in * glm::inverse(mat);
}

void VXLFormat::VXLMatrix::fromMat4(const glm::mat4 &in) {
	matrix = switchYAndZ(in);
}

glm::mat4 VXLFormat::VXLMatrix::toMat4() const {
	return switchYAndZ(matrix);
}

void VXLFormat::convertRead(glm::mat4 &glmMatrix, const VXLNodeFooter& footer, bool hva) {
	glm::vec4 &translation = glmMatrix[3];
	if (hva) {
		// swap y and z here
		translation.x *= footer.scale;
		translation.y *= footer.scale;
		translation.z *= footer.scale;
	}

	// TODO: or the pivot?
	translation.x += footer.mins.x;
	translation.y += footer.mins.z;
	translation.z += footer.mins.y;
}

void VXLFormat::convertWrite(VXLMatrix &vxlMatrix, const glm::mat4 &mat, const glm::vec3& mins, bool hva) {
	vxlMatrix.fromMat4(mat);

	// swap y and z here
	// TODO: or the pivot?
	vxlMatrix.matrix[3][0] -= mins.x;
	vxlMatrix.matrix[3][1] -= mins.z;
	vxlMatrix.matrix[3][2] -= mins.y;

	if (hva) {
		// Calculate the ratio between screen units and voxels in all dimensions
		vxlMatrix.matrix[3][0] /= priv::Scale;
		vxlMatrix.matrix[3][1] /= priv::Scale;
		vxlMatrix.matrix[3][2] /= priv::Scale;
	}
}

bool VXLFormat::writeNodeBodyEntry(io::SeekableWriteStream& stream, const voxel::RawVolume* volume, uint8_t x, uint8_t y, uint8_t z, uint8_t& skipCount, uint8_t& voxelCount, uint8_t normalType) const {
	wrapBool(stream.writeUInt8(skipCount))
	wrapBool(stream.writeUInt8(voxelCount))
	for (uint8_t y1 = y - voxelCount; y1 < y; ++y1) {
		const voxel::Voxel& voxel = volume->voxel(x, y1, z);
		wrapBool(stream.writeUInt8(voxel.getColor()))
		uint8_t normalIndex = 0;
		// TODO: normal
		// if (normalType == 2) { // Tiberian Sun
		// } else if (normalType == 4) { // Red Alert
		// }
		wrapBool(stream.writeUInt8(normalIndex))
	}
	wrapBool(stream.writeUInt8(voxelCount)) // duplicated count
	skipCount = voxelCount = 0u;
	return true;
}

bool VXLFormat::writeNode(io::SeekableWriteStream& stream, const SceneGraphNode& node, VXLNodeOffset& offsets, uint64_t nodeSectionOffset) const {
	const voxel::Region& region = node.region();
	const glm::ivec3& size = region.getDimensionsInVoxels();

	const uint32_t baseSize = size.x * size.z;
	const int64_t globalSpanStartPos = stream.pos();
	Log::debug("size.x: %i, size.z: %i, globalSpanStartPos: %u", size.x, size.z, (uint32_t)globalSpanStartPos);

	offsets.start = stream.pos() - (int64_t)nodeSectionOffset;

	for (uint32_t i = 0; i < baseSize; i++) {
		wrapBool(stream.writeUInt32(-1))
	}
	offsets.end = stream.pos() - (int64_t)nodeSectionOffset;
	for (uint32_t i = 0; i < baseSize; i++) {
		wrapBool(stream.writeUInt32(-1))
	}
	offsets.data = stream.pos() - (int64_t)nodeSectionOffset;

	const uint8_t normalType = core::Var::getSafe(cfg::VoxformatVXLNormalType)->intVal();

	const int64_t beforePos = stream.pos();
	for (uint32_t i = 0u; i < baseSize; ++i) {
		const int64_t spanStartPos = stream.pos() - beforePos;

		const uint8_t x = (uint8_t)(i % size.x);
		const uint8_t z = (uint8_t)(i / size.x);

		uint8_t skipCount = 0u;
		uint8_t voxelCount = 0u;
		bool voxelsInColumn = false;
		for (uint8_t y = 0; y <= size.y; ++y) {
			const voxel::Voxel& voxel = node.volume()->voxel(x, y, z);
			if (voxel::isAir(voxel.getMaterial())) {
				if (voxelCount > 0) {
					wrapBool(writeNodeBodyEntry(stream, node.volume(), x, y, z, skipCount, voxelCount, normalType))
					voxelsInColumn = true;
				}
				if (skipCount == 0xFF) {
					wrapBool(writeNodeBodyEntry(stream, node.volume(), x, y, z, skipCount, voxelCount, normalType))
					voxelsInColumn = true;
				}
				++skipCount;
			} else {
				if (skipCount > 0) {
					wrapBool(writeNodeBodyEntry(stream, node.volume(), x, y, z, skipCount, voxelCount, normalType))
					voxelsInColumn = true;
				}
				if (voxelCount == 0xFF) {
					wrapBool(writeNodeBodyEntry(stream, node.volume(), x, y, z, skipCount, voxelCount, normalType))
					voxelsInColumn = true;
				}
				++voxelCount;
			}
		}
		if (voxelCount > 0 || skipCount > 0) {
			wrapBool(writeNodeBodyEntry(stream, node.volume(), x, size.y - 1, z, skipCount, voxelCount, normalType))
			voxelsInColumn = true;
		}
		if (!voxelsInColumn) {
			continue;
		}

		const int64_t spanEndPos = stream.pos();
		wrap(stream.seek(globalSpanStartPos + i * sizeof(uint32_t)))
		wrapBool(stream.writeUInt32((uint32_t)spanStartPos))

		wrap(stream.seek(globalSpanStartPos + baseSize * sizeof(uint32_t) + i * sizeof(uint32_t)))
		wrapBool(stream.writeUInt32(spanEndPos - globalSpanStartPos))
		wrap(stream.seek(spanEndPos))
	}

	return true;
}

bool VXLFormat::writeNodeHeader(io::SeekableWriteStream& stream, const SceneGraphNode& node, uint32_t nodeIdx) const {
	core_assert((uint64_t)stream.pos() == (uint64_t)(HeaderSize + nodeIdx * NodeHeaderSize));
	core::String name = node.name().substr(0, 15);
	if (stream.write(name.c_str(), name.size()) == -1) {
		Log::error("Failed to write node header into stream");
		return false;
	}
	for (size_t i = 0; i < 16 - name.size(); ++i) {
		wrapBool(stream.writeUInt8(0))
	}
	wrapBool(stream.writeUInt32(nodeIdx))
	wrapBool(stream.writeUInt32(1))
	wrapBool(stream.writeUInt32(0))
	return true;
}

bool VXLFormat::writeNodeFooter(io::SeekableWriteStream& stream, const SceneGraphNode& node, const VXLNodeOffset& offsets) const {
	wrapBool(stream.writeUInt32(offsets.start))
	wrapBool(stream.writeUInt32(offsets.end))
	wrapBool(stream.writeUInt32(offsets.data))

	const FrameIndex frameIdx = 0;
	const SceneGraphTransform &transform = node.transform(frameIdx);
	const glm::vec3 &mins = transform.localTranslation();
	VXLMatrix vxlMatrix;
	convertWrite(vxlMatrix, transform.localMatrix(), transform.localTranslation(), false);

	// TODO: always 0.0833333358f?
	wrapBool(stream.writeFloat(priv::Scale /*transform.localScale()*/))

	for (int i = 0; i < 12; ++i) {
		const int col = i % 4;
		const int row = i / 4;
		float val = vxlMatrix.matrix[col][row];
		wrapBool(stream.writeFloat(val))
	}

	const voxel::Region& region = node.region();
	const glm::ivec3& size = region.getDimensionsInVoxels();
	if (size.x > 0xFF || size.y > 0xFF || size.z > 0xFF) {
		Log::error("Failed to write vxl node footer - max volume size exceeded");
		return false;
	}

	wrapBool(stream.writeFloat(mins.x))
	wrapBool(stream.writeFloat(mins.z))
	wrapBool(stream.writeFloat(mins.y))

	const glm::vec3 maxs = mins + glm::vec3(size);
	wrapBool(stream.writeFloat(maxs.x))
	wrapBool(stream.writeFloat(maxs.z))
	wrapBool(stream.writeFloat(maxs.y))

	wrapBool(stream.writeUInt8(size.x))
	wrapBool(stream.writeUInt8(size.z))
	wrapBool(stream.writeUInt8(size.y))

	wrapBool(stream.writeUInt8(core::Var::getSafe(cfg::VoxformatVXLNormalType)->intVal()))

	return true;
}

bool VXLFormat::writeHeader(io::SeekableWriteStream& stream, uint32_t numNodes, const voxel::Palette &palette) {
	VXLHeader header;
	SDL_strlcpy(header.filetype, "Voxel Animation", sizeof(header.filetype));
	header.paletteCount = 1;
	header.nodeCount = numNodes;
	header.tailerCount = numNodes;
	header.bodysize = 0; // bodysize is filled later

	wrapBool(stream.writeString(header.filetype, true))
	wrapBool(stream.writeUInt32(header.paletteCount))
	wrapBool(stream.writeUInt32(header.nodeCount))
	wrapBool(stream.writeUInt32(header.tailerCount))
	wrapBool(stream.writeUInt32(header.bodysize))

	wrapBool(stream.writeUInt8(0x1fU)) // startPaletteRemap
	wrapBool(stream.writeUInt8(0x10U)) // endPaletteRemap
	for (int i = 0; i < palette.colorCount; ++i) {
		const core::RGBA& rgba = palette.colors[i];
		wrapBool(stream.writeUInt8(rgba.r))
		wrapBool(stream.writeUInt8(rgba.g))
		wrapBool(stream.writeUInt8(rgba.b))
	}
	for (int i = palette.colorCount; i < voxel::PaletteMaxColors; ++i) {
		wrapBool(stream.writeUInt8(0))
		wrapBool(stream.writeUInt8(0))
		wrapBool(stream.writeUInt8(0))
	}
	core_assert(stream.pos() == HeaderSize);
	return true;
}

bool VXLFormat::saveVXL(core::DynamicArray<const SceneGraphNode*> &nodes, const core::String &filename, io::SeekableWriteStream& stream) {
	if (nodes.empty()) {
		return false;
	}
	const uint32_t numNodes = nodes.size();
	wrapBool(writeHeader(stream, numNodes, nodes[0]->palette()))
	for (uint32_t i = 0; i < numNodes; ++i) {
		const SceneGraphNode* node = nodes[(int)i];
		wrapBool(writeNodeHeader(stream, *node, i))
	}

	core::Buffer<VXLNodeOffset> nodeOffsets(numNodes);
	const uint64_t afterHeaderPos = stream.pos();
	for (uint32_t i = 0; i < numNodes; ++i) {
		const SceneGraphNode* node = nodes[(int)i];
		wrapBool(writeNode(stream, *node, nodeOffsets[i], afterHeaderPos))
	}

	const uint64_t afterBodyPos = stream.pos();
	const uint64_t bodySize = afterBodyPos - afterHeaderPos;
	wrap(stream.seek(HeaderBodySizeOffset));
	wrapBool(stream.writeUInt32(bodySize))
	wrap(stream.seek(afterBodyPos));

	core_assert((uint64_t)stream.pos() == (uint64_t)(HeaderSize + NodeHeaderSize * numNodes + bodySize));

	for (uint32_t i = 0; i < numNodes; ++i) {
		const SceneGraphNode* node = nodes[(int)i];
		wrapBool(writeNodeFooter(stream, *node, nodeOffsets[i]))
	}
	return true;
}

bool VXLFormat::saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream, ThumbnailCreator thumbnailCreator) {
	core::DynamicArray<const SceneGraphNode*> body;
	core::DynamicArray<const SceneGraphNode*> barrel;
	core::DynamicArray<const SceneGraphNode*> turret;

	const uint32_t numNodes = sceneGraph.size();
	body.reserve(numNodes);
	barrel.reserve(numNodes);
	turret.reserve(numNodes);

	// TODO: split the nodes into the max allowed size
	for (const SceneGraphNode &node : sceneGraph) {
		const core::String &lowerName = node.name().toLower();
		if (lowerName.contains("barrel")) {
			barrel.push_back(&node);
		} else if (lowerName.contains("turret")) {
			turret.push_back(&node);
		} else {
			body.push_back(&node);
		}
	}

	const core::String &basename = core::string::stripExtension(filename);

	if (!saveVXL(body, filename, stream)) {
		return false;
	}
	if (!barrel.empty()) {
		const core::String &extFilename = basename + "barl.vxl";
		io::FileStream extStream(io::filesystem()->open(extFilename, io::FileMode::SysWrite));
		if (extStream.valid() && !saveVXL(barrel, extFilename, extStream)) {
			Log::warn("Failed to write %s", extFilename.c_str());
		}
	}
	if (!turret.empty()) {
		const core::String &extFilename = basename + "tur.vxl";
		io::FileStream extStream(io::filesystem()->open(extFilename, io::FileMode::SysWrite));
		if (extStream.valid() && !saveVXL(turret, extFilename, extStream)) {
			Log::warn("Failed to write %s", extFilename.c_str());
		}
	}
	return saveHVA(basename + ".hva", sceneGraph);
}

bool VXLFormat::readNode(io::SeekableReadStream& stream, VXLModel& mdl, uint32_t nodeIdx, SceneGraph& sceneGraph, const voxel::Palette &palette) const {
	const uint64_t nodeStart = stream.pos();
	const VXLNodeFooter &footer = mdl.nodeFooters[nodeIdx];
	const VXLNodeHeader &header = mdl.nodeHeaders[nodeIdx];

	const uint32_t baseSize = footer.xsize * footer.ysize;
	core::Buffer<int32_t> colStart(baseSize);
	core::Buffer<int32_t> colEnd(baseSize);

	if (stream.skip(footer.spanStartOffset) == -1) {
		Log::error("Failed to skip %u node start offset bytes", footer.spanStartOffset);
		return false;
	}
	for (uint32_t i = 0; i < baseSize; ++i) {
		wrap(stream.readInt32(colStart[i]))
	}
	for (uint32_t i = 0; i < baseSize; ++i) {
		wrap(stream.readInt32(colEnd[i]))
	}

	const uint64_t dataStart = stream.pos();
	if (dataStart - nodeStart != footer.spanDataOffset) {
		Log::error("Invalid offset found for node %u: %u", nodeIdx, footer.spanStartOffset);
		return false;
	}

	// Count the voxels in this node
	for (uint32_t i = 0u; i < baseSize; ++i) {
		if (colStart[i] == EmptyColumn || colEnd[i] == EmptyColumn) {
			continue;
		}

		wrap(stream.seek(dataStart + colStart[i]))
		uint32_t z = 0;
		do {
			uint8_t v;
			wrap(stream.readUInt8(v))
			z += v;
			wrap(stream.readUInt8(v))
			z += v;
			stream.skip(2 * v + 1);
		} while (z < footer.zsize);
	}

	// switch axis
	const voxel::Region region{0, 0, 0, (int)footer.xsize - 1, (int)footer.zsize - 1, (int)footer.ysize - 1};
	if (!region.isValid()) {
		Log::error("Failed to load section with invalid size: %i:%i:%i", (int)footer.xsize, (int)footer.zsize, (int)footer.ysize);
		return false;
	}
	voxel::RawVolume *volume = new voxel::RawVolume(region);
	SceneGraphNode node;
	node.setVolume(volume, true);
	node.setName(header.name);
	if (palette.colorCount > 0) {
		node.setPalette(palette);
	}

	glm::mat4 glmMatrix = footer.transform.toMat4();
	convertRead(glmMatrix, footer, false);

	SceneGraphTransform transform;
	transform.setLocalMatrix(glmMatrix);
	const KeyFrameIndex keyFrameIdx = 0;
	node.setTransform(keyFrameIdx, transform);

	for (uint32_t i = 0u; i < baseSize; ++i) {
		if (colStart[i] == EmptyColumn || colEnd[i] == EmptyColumn) {
			continue;
		}

		wrap(stream.seek(dataStart + colStart[i]))

		const uint8_t x = (uint8_t)(i % footer.xsize);
		const uint8_t y = (uint8_t)(i / footer.xsize);
		uint8_t z = 0;
		while (z < footer.zsize) {
			uint8_t skipCount;
			wrap(stream.readUInt8(skipCount))
			z += skipCount;
			uint8_t voxelCount;
			wrap(stream.readUInt8(voxelCount))
			for (uint8_t j = 0u; j < voxelCount; ++j) {
				uint8_t color;
				wrap(stream.readUInt8(color))
				uint8_t normal;
				wrap(stream.readUInt8(normal))
				const voxel::Voxel v = voxel::createVoxel(color);
				volume->setVoxel(x, z, y, v);
				++z;
			}

			// Skip duplicate count
			stream.skip(1);
		}
	}
	sceneGraph.emplace(core::move(node));
	return true;
}

bool VXLFormat::readNodes(io::SeekableReadStream& stream, VXLModel& mdl, SceneGraph& sceneGraph, const voxel::Palette &palette) const {
	const VXLHeader& hdr = mdl.header;
	sceneGraph.reserve(hdr.nodeCount);
	const int64_t bodyPos = stream.pos();
	for (uint32_t i = 0; i < hdr.nodeCount; ++i) {
		if (stream.seek(bodyPos) == -1) {
			Log::error("Failed to seek for node %u", i);
			return false;
		}
		wrapBool(readNode(stream, mdl, i, sceneGraph, palette))
	}
	return true;
}

bool VXLFormat::readNodeHeader(io::SeekableReadStream& stream, VXLModel& mdl, uint32_t nodeIdx) const {
	VXLNodeHeader &header = mdl.nodeHeaders[nodeIdx];
	wrapBool(stream.readString(lengthof(header.name), header.name, false))
	wrap(stream.readUInt32(header.id))
	wrap(stream.readUInt32(header.unknown))
	wrap(stream.readUInt32(header.unknown2))
	Log::debug("Node %u name: %s, id %u, unknown: %u, unknown2: %u",
			nodeIdx, header.name, header.id, header.unknown, header.unknown2);
	return true;
}

bool VXLFormat::readNodeHeaders(io::SeekableReadStream& stream, VXLModel& mdl) const {
	for (uint32_t i = 0; i < mdl.header.nodeCount; ++i) {
		wrapBool(readNodeHeader(stream, mdl, i))
	}
	return true;
}

bool VXLFormat::readNodeFooter(io::SeekableReadStream& stream, VXLModel& mdl, uint32_t nodeIdx) const {
	VXLNodeFooter &footer = mdl.nodeFooters[nodeIdx];
	wrap(stream.readUInt32(footer.spanStartOffset))
	wrap(stream.readUInt32(footer.spanEndOffset))
	wrap(stream.readUInt32(footer.spanDataOffset))
	wrap(stream.readFloat(footer.scale))

	for (int i = 0; i < 12; ++i) {
		const int col = i % 4;
		const int row = i / 4;
		float &val = footer.transform.matrix[col][row];
		wrap(stream.readFloat(val))
	}
	for (int i = 0; i < 3; ++i) {
		wrap(stream.readFloat(footer.mins[i]))
	}
	for (int i = 0; i < 3; ++i) {
		wrap(stream.readFloat(footer.maxs[i]))
	}

	wrap(stream.readUInt8(footer.xsize))
	wrap(stream.readUInt8(footer.ysize))
	wrap(stream.readUInt8(footer.zsize))
	wrap(stream.readUInt8(footer.normalType))

	if (footer.xsize == 0 || footer.ysize == 0 || footer.zsize == 0) {
		Log::error("Invalid node size found");
		return false;
	}

	Log::debug("Scale: %f", footer.scale);
	Log::debug("Mins: %f:%f:%f", footer.mins[0], footer.mins[1], footer.mins[2]);
	Log::debug("Maxs: %f:%f:%f", footer.maxs[0], footer.maxs[1], footer.maxs[2]);
	Log::debug("SpanStartOffset: %u", footer.spanStartOffset);
	Log::debug("SpanEndOffset: %u", footer.spanEndOffset);
	Log::debug("SpanDataOffset: %u", footer.spanDataOffset);
	Log::debug("FooterSize: %u:%u:%u", footer.xsize, footer.ysize, footer.zsize);
	Log::debug("Normaltype: %u", footer.normalType);
	return true;
}

bool VXLFormat::readNodeFooters(io::SeekableReadStream& stream, VXLModel& mdl) const {
	const VXLHeader& hdr = mdl.header;
	wrap(stream.seek(HeaderSize + NodeHeaderSize * hdr.nodeCount + hdr.bodysize))
	for (uint32_t i = 0; i < hdr.tailerCount; ++i) {
		wrapBool(readNodeFooter(stream, mdl, i))
	}
	return true;
}

bool VXLFormat::readHeader(io::SeekableReadStream& stream, VXLModel& mdl, voxel::Palette &palette) {
	VXLHeader& hdr = mdl.header;
	wrapBool(stream.readString(lengthof(hdr.filetype), hdr.filetype, false))
	if (SDL_strcmp(hdr.filetype, "Voxel Animation") != 0) {
		Log::error("Invalid vxl header");
		return false;
	}
	wrap(stream.readUInt32(hdr.paletteCount))
	wrap(stream.readUInt32(hdr.nodeCount))
	wrap(stream.readUInt32(hdr.tailerCount))
	wrap(stream.readUInt32(hdr.bodysize))

	Log::debug("Palettes: %u", hdr.paletteCount);
	Log::debug("Nodes: %u", hdr.nodeCount);
	Log::debug("Tailers: %u", hdr.tailerCount);
	Log::debug("BodySize: %u", hdr.bodysize);

	palette.colorCount = voxel::PaletteMaxColors;
	bool valid = false;
	for (uint32_t n = 0; n < hdr.paletteCount; ++n) {
		wrap(stream.readUInt8(hdr.palette.startPaletteRemap)) // 0x1f
		wrap(stream.readUInt8(hdr.palette.endPaletteRemap)) // 0x10
		Log::debug("palette %u: %u start, %u end palette offset", n, hdr.palette.startPaletteRemap, hdr.palette.endPaletteRemap);
		for (int i = 0; i < palette.colorCount; ++i) {
			wrap(stream.readUInt8(hdr.palette.palette[i][0]))
			wrap(stream.readUInt8(hdr.palette.palette[i][1]))
			wrap(stream.readUInt8(hdr.palette.palette[i][2]))
			if (hdr.palette.palette[i][0] != 0 || hdr.palette.palette[i][1] != 0 || hdr.palette.palette[i][2] != 0) {
				valid = true;
			}
		}
		if (valid) {
			break;
		}
	}

	if (valid) {
		for (int i = 0; i < palette.colorCount; ++i) {
			const uint8_t *p = hdr.palette.palette[i];
			palette.colors[i] = core::RGBA(p[0], p[1], p[2]);
		}
	} else {
		palette.commandAndConquer();
		Log::warn("No palette found in vxl");
	}

	return true;
}

bool VXLFormat::prepareModel(VXLModel& mdl) const {
	const VXLHeader& hdr = mdl.header;
	if (hdr.nodeCount > MaxNodes) {
		Log::error("Node size exceeded the max allowed value: %u", hdr.nodeCount);
		return false;
	}
	return true;
}

bool VXLFormat::readHVAHeader(io::SeekableReadStream& stream, HVAHeader& header) const {
	char name[16];
	wrapBool(stream.readString(lengthof(name), name, false))
	header.filename = name;
	Log::debug("hva name: %s", header.filename.c_str());
	wrap(stream.readUInt32(header.numFrames))
	Log::debug("numframes: %i", header.numFrames);
	wrap(stream.readUInt32(header.numNodes))
	Log::debug("sections: %i", header.numNodes);
	for (uint32_t i = 0; i < header.numNodes; ++i) {
		wrapBool(stream.readString(lengthof(name), name, false))
		header.nodeNames[i] = name;
		Log::debug("hva section %u: %s", i, header.nodeNames[i].c_str());
	}
	return true;
}

int VXLFormat::VXLModel::findNodeByName(const core::String& name) const {
	for (uint32_t i = 0; i < header.nodeCount; ++i) {
		if (name == nodeHeaders[i].name) {
			return (int)i;
		}
	}
	return -1;
}

bool VXLFormat::readHVAFrames(io::SeekableReadStream& stream, const VXLModel &mdl, HVAModel& file) const {
	if (file.header.numNodes >= lengthof(file.frames)) {
		Log::error("Max allowed frame count exceeded");
		return false;
	}
	for (uint32_t i = 0; i < file.header.numNodes; ++i) {
		file.header.nodeIds[i] = mdl.findNodeByName(file.header.nodeNames[i]);
		if (file.header.nodeIds[i] == -1) {
			Log::debug("Failed to resolve node id for '%s' (node idx: %i/%i)",
				file.header.nodeNames[i].c_str(), i, file.header.numNodes);
			for (uint32_t i = 0; i < mdl.header.nodeCount; ++i) {
				Log::debug(" - found: %s", mdl.nodeHeaders[i].name);
			}
		}
	}

	for (uint32_t frameIdx = 0; frameIdx < file.header.numFrames; ++frameIdx) {
		HVAFrames &frame = file.frames[frameIdx];
		frame.resize(file.header.numNodes);
		for (uint32_t nodeIdx = 0; nodeIdx < file.header.numNodes; ++nodeIdx) {
			VXLMatrix &vxlMatrix = frame[nodeIdx];
			for (int i = 0; i < 12; ++i) {
				const int col = i % 4;
				const int row = i / 4;
				float &val = vxlMatrix.matrix[col][row];
				wrap(stream.readFloat(val))
			}
			Log::debug("load frame %u for node %i with translation: %f:%f:%f", frameIdx, nodeIdx, vxlMatrix.matrix[3][0], vxlMatrix.matrix[3][1], vxlMatrix.matrix[3][2]);
		}
	}

	return true;
}

bool VXLFormat::loadHVA(const core::String &filename, const VXLModel &mdl, SceneGraph& sceneGraph) {
	HVAModel file;
	{
		const io::FilesystemPtr &filesystem = io::filesystem();
		const io::FilePtr &hvaFile = filesystem->open(filename);
		if (!hvaFile->validHandle()) {
			// if there is no hva file, we still don't show an error
			return true;
		}
		io::FileStream stream(hvaFile);
		wrapBool(readHVAHeader(stream, file.header));
		wrapBool(readHVAFrames(stream, mdl, file));
	}
	Log::debug("load %u frames", file.header.numFrames);
	for (uint32_t keyFrameIdx = 0; keyFrameIdx < file.header.numFrames; ++keyFrameIdx) {
		const HVAFrames &sectionMatrices = file.frames[keyFrameIdx];
		for (uint32_t vxlNodeId = 0; vxlNodeId < file.header.numNodes; ++vxlNodeId) {
			const core::String &name = file.header.nodeNames[vxlNodeId];
			SceneGraphNode* node = sceneGraph.findNodeByName(name);
			if (node == nullptr) {
				Log::debug("Can't find node with name %s", name.c_str());
				continue;
			}
			// hva transforms are overriding the vxl transform
			SceneGraphKeyFrame &kf = node->keyFrame(keyFrameIdx);
			kf.frameIdx = keyFrameIdx * 6; // running at 6 fps

			const int nodeId = file.header.nodeIds[vxlNodeId];
			if (nodeId != -1) {
				glm::mat4 glmMatrix = sectionMatrices[vxlNodeId].toMat4();
				convertRead(glmMatrix, mdl.nodeFooters[nodeId], true);

				SceneGraphTransform transform;
				transform.setLocalMatrix(glmMatrix);
				kf.setTransform(transform);
			}
		}
	}
	return true;
}

bool VXLFormat::writeHVAHeader(io::SeekableWriteStream& stream, const SceneGraph& sceneGraph) const {
	char name[16];
	core_memset(name, 0, sizeof(name));
	// TODO: name
	if (stream.write(name, sizeof(name)) == -1) {
		Log::error("Failed to write hva header name");
		return false;
	}
	uint32_t numFrames = 0;

	for (const SceneGraphNode &node : sceneGraph) {
		numFrames = core_max(numFrames, node.keyFrames().size());
	}

	stream.writeUInt32(numFrames);
	uint32_t numNodes = sceneGraph.size();
	stream.writeUInt32(numNodes);
	for (uint32_t frame = 0; frame < numFrames; ++frame) {
		for (const SceneGraphNode &node : sceneGraph) {
			const core::String &name = node.name().substr(0, 15);
			if (stream.write(name.c_str(), name.size()) == -1) {
				Log::error("Failed to write node name");
				return false;
			}
			for (size_t i = 0; i < 16 - name.size(); ++i) {
				wrapBool(stream.writeUInt8(0))
			}
		}
	}
	return true;
}

bool VXLFormat::writeHVAFrames(io::SeekableWriteStream& stream, const SceneGraph& sceneGraph) const {
	uint32_t numFrames = 0;
	for (const SceneGraphNode &node : sceneGraph) {
		numFrames = core_max(numFrames, node.keyFrames().size());
	}

	for (uint32_t i = 0; i < numFrames; ++i) {
		for (const SceneGraphNode &node : sceneGraph) {
			const SceneGraphTransform &transform = node.transform(i);
			VXLMatrix vxlMatrix;
			convertWrite(vxlMatrix, transform.localMatrix(), transform.localTranslation(), true);

			for (int i = 0; i < 12; ++i) {
				const int col = i % 4;
				const int row = i / 4;
				float val = vxlMatrix.matrix[col][row];
				wrapBool(stream.writeFloat(val))
			}
		}
	}
	return true;
}

bool VXLFormat::saveHVA(const core::String &filename, const SceneGraph& sceneGraph) {
	const io::FilesystemPtr &filesystem = io::filesystem();
	io::FilePtr hvaFile = filesystem->open(filename, io::FileMode::SysWrite);
	if (!hvaFile->validHandle()) {
		return false;
	}
	io::FileStream stream(hvaFile);
	wrapBool(writeHVAHeader(stream, sceneGraph));
	wrapBool(writeHVAFrames(stream, sceneGraph));
	return true;
}

bool VXLFormat::loadFromFile(const core::String &filename, SceneGraph& sceneGraph, voxel::Palette &palette) {
	const io::FilePtr &file = io::filesystem()->open(filename);
	if (file && file->validHandle()) {
		io::FileStream stream(file);
		return loadGroupsPalette(filename, stream, sceneGraph, palette);
	}
	return true;
}

size_t VXLFormat::loadPalette(const core::String &filename, io::SeekableReadStream& stream, voxel::Palette &palette) {
	VXLModel mdl;
	if (!readHeader(stream, mdl, palette)) {
		return false;
	}
	return palette.colorCount;
}

bool VXLFormat::loadGroupsPalette(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, voxel::Palette &palette) {
	VXLModel mdl;

	wrapBool(readHeader(stream, mdl, palette))
	wrapBool(prepareModel(mdl))

	wrapBool(readNodeHeaders(stream, mdl))
	const int64_t bodyPos = stream.pos();
	if (stream.skip(mdl.header.bodysize) == -1) {
		Log::error("Failed to skip %u bytes", mdl.header.bodysize);
		return false;
	}
	wrapBool(readNodeFooters(stream, mdl))

	if (stream.seek(bodyPos) == -1) {
		Log::error("Failed to seek");
		return false;
	}
	wrapBool(readNodes(stream, mdl, sceneGraph, palette))

	const core::String &basename = core::string::stripExtension(filename);
	wrapBool(loadHVA(basename + ".hva", mdl, sceneGraph))

	if (!core::string::endsWith(filename, "barl.vxl")) {
		wrapBool(loadFromFile(basename + "barl.vxl", sceneGraph, palette));
	}
	if (!core::string::endsWith(filename, "tur.vxl")) {
		wrapBool(loadFromFile(basename + "tur.vxl", sceneGraph, palette));
	}

	return true;
}

#undef wrap
#undef wrapBool

}
