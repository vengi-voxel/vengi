/**
 * @file
 */

#include "VXLFormat.h"
#include "app/App.h"
#include "core/Common.h"
#include "core/Color.h"
#include "core/Assert.h"
#include "core/StandardLib.h"
#include "core/StringUtil.h"
#include "core/Log.h"
#include "core/collection/DynamicArray.h"
#include <glm/gtx/transform.hpp>
#include "io/File.h"
#include "io/Filesystem.h"
#include "io/FileStream.h"
#include "io/Stream.h"
#include "voxel/MaterialColor.h"
#include "core/collection/Buffer.h"
#include "voxel/Palette.h"
#include "voxelformat/SceneGraphNode.h"
#include "voxel/PaletteLookup.h"

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

bool VXLFormat::writeNodeBodyEntry(io::SeekableWriteStream& stream, const voxel::RawVolume* volume, uint8_t x, uint8_t y, uint8_t z, uint32_t& skipCount, uint32_t& voxelCount) const {
	wrapBool(stream.writeUInt8(skipCount))
	wrapBool(stream.writeUInt8(voxelCount))
	for (uint8_t y1 = y - voxelCount; y1 < y; ++y1) {
		const voxel::Voxel& voxel = volume->voxel(x, y1, z);
		wrapBool(stream.writeUInt8(voxel.getColor()))
		wrapBool(stream.writeUInt8(0)) // TODO: normal
	}
	wrapBool(stream.writeUInt8(voxelCount)) // duplicated count
	skipCount = voxelCount = 0u;
	return true;
}

bool VXLFormat::writeNode(io::SeekableWriteStream& stream, const SceneGraph& sceneGraph, uint32_t nodeIdx, VXLNodeOffset& offsets, uint64_t nodeSectionOffset) const {
	const SceneGraphNode* node = sceneGraph[(int)nodeIdx];
	core_assert_always(node != nullptr);
	const voxel::Region& region = node->region();
	const glm::ivec3& size = region.getDimensionsInVoxels();

	const uint32_t baseSize = size.x * size.z;
	const int64_t globalSpanStartPos = stream.pos();
	Log::debug("size.x: %i, size.z: %i, globalSpanStartPos: %u", size.x, size.z, (uint32_t)globalSpanStartPos);

	offsets.start = stream.pos() - (int64_t)nodeSectionOffset;
	const size_t nodeOffset = HeaderSize + NodeHeaderSize * sceneGraph.size() + offsets.start;
	Log::debug("nodeOffset(%u): %u", nodeIdx, (uint32_t)nodeOffset);

	for (uint32_t i = 0; i < baseSize; i++) {
		wrapBool(stream.writeUInt32(-1))
	}
	offsets.end = stream.pos() - (int64_t)nodeSectionOffset;
	for (uint32_t i = 0; i < baseSize; i++) {
		wrapBool(stream.writeUInt32(-1))
	}
	offsets.data = stream.pos() - (int64_t)nodeSectionOffset;

	const int64_t beforePos = stream.pos();
	for (uint32_t i = 0u; i < baseSize; ++i) {
		const int64_t spanStartPos = stream.pos() - beforePos;

		const uint8_t x = (uint8_t)(i % size.x);
		const uint8_t z = (uint8_t)(i / size.x);

		uint32_t skipCount = 0u;
		uint32_t voxelCount = 0u;
		bool voxelsInColumn = false;
		for (uint8_t y = 0; y <= size.y; ++y) {
			const voxel::Voxel& voxel = node->volume()->voxel(x, y, z);
			if (voxel::isAir(voxel.getMaterial())) {
				if (voxelCount > 0) {
					wrapBool(writeNodeBodyEntry(stream, node->volume(), x, y, z, skipCount, voxelCount))
					voxelsInColumn = true;
				}
				++skipCount;
			} else {
				if (skipCount > 0) {
					wrapBool(writeNodeBodyEntry(stream, node->volume(), x, y, z, skipCount, voxelCount))
					voxelsInColumn = true;
				}
				++voxelCount;
			}
		}
		if (voxelCount > 0 || skipCount > 0) {
			wrapBool(writeNodeBodyEntry(stream, node->volume(), x, size.y - 1, z, skipCount, voxelCount))
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

bool VXLFormat::writeNodeHeader(io::SeekableWriteStream& stream, const SceneGraph& sceneGraph, uint32_t nodeIdx) const {
	core_assert((uint64_t)stream.pos() == (uint64_t)(HeaderSize + nodeIdx * NodeHeaderSize));
	const SceneGraphNode* node = sceneGraph[(int)nodeIdx];
	core_assert_always(node != nullptr);
	core::String name = node->name().substr(0, 15);
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

bool VXLFormat::writeNodeFooter(io::SeekableWriteStream& stream, const SceneGraph& sceneGraph, uint32_t nodeIdx, const VXLNodeOffset& offsets) const {
	const SceneGraphNode* node = sceneGraph[(int)nodeIdx];
	core_assert_always(node != nullptr);
	wrapBool(stream.writeUInt32(offsets.start))
	wrapBool(stream.writeUInt32(offsets.end))
	wrapBool(stream.writeUInt32(offsets.data))

	const FrameIndex frameIdx = 0;
	const SceneGraphTransform &transform = node->transform(frameIdx);
	const glm::mat4 &matrix = transform.localMatrix();

	wrapBool(stream.writeFloat(transform.localScale()))

	for (int row = 0; row < 3; ++row) {
		for (int col = 0; col < 4; ++col) {
			wrapBool(stream.writeFloat(matrix[col][row]))
		}
	}
	const glm::vec3 &mins = transform.localTranslation();
	const glm::vec3 maxs = mins + glm::vec3(node->region().getDimensionsInVoxels());
	for (int i = 0; i < 3; ++i) {
		wrapBool(stream.writeFloat(mins[i]))
	}
	for (int i = 0; i < 3; ++i) {
		wrapBool(stream.writeFloat(maxs[i]))
	}

	const voxel::Region& region = node->region();
	const glm::ivec3& size = region.getDimensionsInVoxels();
	if (size.x > 0xFF || size.y > 0xFF || size.z > 0xFF) {
		Log::error("Failed to write vxl node footer - max volume size exceeded");
		return false;
	}
	wrapBool(stream.writeUInt8(size.x))
	wrapBool(stream.writeUInt8(size.z))
	wrapBool(stream.writeUInt8(size.y))
	wrapBool(stream.writeUInt8(2)) // normalType TS
	return true;
}

bool VXLFormat::writeHeader(io::SeekableWriteStream& stream, const SceneGraph& sceneGraph) {
	wrapBool(stream.writeString("Voxel Animation", true))
	wrapBool(stream.writeUInt32(1))
	wrapBool(stream.writeUInt32(sceneGraph.size()))
	wrapBool(stream.writeUInt32(sceneGraph.size()))
	wrapBool(stream.writeUInt32(0)) // bodysize is filled later
	wrapBool(stream.writeUInt8(0x1fU))
	wrapBool(stream.writeUInt8(0x10U))

	const voxel::Palette &palette = sceneGraph.firstPalette();
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

bool VXLFormat::saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) {
	wrapBool(writeHeader(stream, sceneGraph))
	for (uint32_t i = 0; i < sceneGraph.size(); ++i) {
		wrapBool(writeNodeHeader(stream, sceneGraph, i))
	}

	core::Buffer<VXLNodeOffset> nodeOffsets(sceneGraph.size());
	const uint64_t afterHeaderPos = stream.pos();
	for (uint32_t i = 0; i < sceneGraph.size(); ++i) {
		wrapBool(writeNode(stream, sceneGraph, i, nodeOffsets[i], afterHeaderPos))
	}

	const uint64_t afterBodyPos = stream.pos();
	const uint64_t bodySize = afterBodyPos - afterHeaderPos;
	wrap(stream.seek(HeaderBodySizeOffset));
	wrapBool(stream.writeUInt32(bodySize))
	wrap(stream.seek(afterBodyPos));

	core_assert((uint64_t)stream.pos() == (uint64_t)(HeaderSize + NodeHeaderSize * sceneGraph.size() + bodySize));

	for (uint32_t i = 0; i < sceneGraph.size(); ++i) {
		wrapBool(writeNodeFooter(stream, sceneGraph, i, nodeOffsets[i]))
	}
	const core::String &basename = core::string::stripExtension(filename);
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
	voxel::RawVolume *volume = new voxel::RawVolume(voxel::Region{0, 0, 0, footer.xsize - 1, footer.zsize - 1, footer.ysize - 1});
	SceneGraphNode node;
	node.setVolume(volume, true);
	node.setName(header.name);
	if (palette.colorCount > 0) {
		node.setPalette(palette);
	}
	SceneGraphTransform transform;
	glm::mat4 transformMatrix = footer.transform;
	transform.setLocalMatrix(transformMatrix);
	// transform.setLocalScale(footer.scale); // TODO
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
		do {
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
				const voxel::Voxel v = voxel::createVoxel(voxel::VoxelType::Generic, color);
				volume->setVoxel(x, z, y, v);
				++z;
			}

			// Skip duplicate count
			stream.skip(1);
		} while (z < footer.zsize);
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
	Log::debug("Node %u name: %s", nodeIdx, header.name);
	wrap(stream.readUInt32(header.unknown))
	wrap(stream.readUInt32(header.id))
	wrap(stream.readUInt32(header.unknown2))
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

	Log::debug("offsets: %u:%u:%u", footer.spanStartOffset, footer.spanEndOffset, footer.spanDataOffset);

	footer.transform = glm::mat4(1.0f);
	for (int row = 0; row < 3; ++row) {
		for (int col = 0; col < 4; ++col) {
			wrap(stream.readFloat(footer.transform[col][row]))
		}
	}
	for (int i = 0; i < 3; ++i) {
		wrap(stream.readFloat(footer.mins[i]))
	}
	for (int i = 0; i < 3; ++i) {
		wrap(stream.readFloat(footer.maxs[i]))
	}
	Log::debug("scale: %f", footer.scale);
	Log::debug("mins: %f:%f:%f", footer.mins[0], footer.mins[1], footer.mins[2]);
	Log::debug("maxs: %f:%f:%f", footer.maxs[0], footer.maxs[1], footer.maxs[2]);

	wrap(stream.readUInt8(footer.xsize))
	wrap(stream.readUInt8(footer.ysize))
	wrap(stream.readUInt8(footer.zsize))

	Log::debug("size: %u:%u:%u", footer.xsize, footer.ysize, footer.zsize);
	wrap(stream.readUInt8(footer.normalType))

	if (footer.xsize == 0 || footer.ysize == 0 || footer.zsize == 0) {
		Log::error("Invalid node size found");
		return false;
	}

	Log::debug("size: %u:%u:%u, type: %u", footer.xsize, footer.ysize, footer.zsize, footer.normalType);
	return true;
}

bool VXLFormat::readNodeFooters(io::SeekableReadStream& stream, VXLModel& mdl) const {
	const VXLHeader& hdr = mdl.header;
	wrap(stream.seek(HeaderSize + NodeHeaderSize * hdr.nodeCount + hdr.bodysize))
	for (uint32_t i = 0; i < hdr.nodeCount; ++i) {
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
	wrap(stream.readUInt32(hdr.unknown))
	wrap(stream.readUInt32(hdr.nodeCount))
	wrap(stream.readUInt32(hdr.nodeCount2))
	wrap(stream.readUInt32(hdr.bodysize))
	wrap(stream.readUInt8(hdr.startPaletteRemap))
	wrap(stream.readUInt8(hdr.endPaletteRemap))

	Log::debug("Found %u nodes", hdr.nodeCount);

	palette.colorCount = voxel::PaletteMaxColors;
	bool valid = false;
	for (int i = 0; i < palette.colorCount; ++i) {
		wrap(stream.readUInt8(hdr.palette[i][0]))
		wrap(stream.readUInt8(hdr.palette[i][1]))
		wrap(stream.readUInt8(hdr.palette[i][2]))
		if (hdr.palette[i][0] != 0 || hdr.palette[i][1] != 0 || hdr.palette[i][2] != 0) {
			valid = true;
		}
	}

	// TODO: glow colors 240-255 ?

	if (valid) {
		for (int i = 0; i < palette.colorCount; ++i) {
			const uint8_t *p = hdr.palette[i];
			palette.colors[i] = core::Color::getRGBA(p[0], p[1], p[2]);
		}
	} else {
		palette.colorCount = 0;
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
			glm::mat4 &m = frame[nodeIdx];
			m = glm::mat4(1.0f);
			for (int row = 0; row < 3; ++row) {
				for (int col = 0; col < 4; ++col) {
					wrap(stream.readFloat(m[col][row]))
				}
			}
			const int nodeId = file.header.nodeIds[nodeIdx];
			if (nodeId == -1) {
				m = glm::mat4(1.0f);
				continue;
			}
			const VXLNodeFooter& footer = mdl.nodeFooters[nodeId];
			const glm::vec3 size(footer.xsize, footer.ysize, footer.zsize);
			const glm::vec3 nodeScale = (footer.maxs - footer.mins) / size;
			Log::debug("nodeScale: %f:%f:%f", nodeScale[0], nodeScale[1], nodeScale[2]);
			// The HVA transformation matrices must be scaled - the VXL ones not!
			// Calculate the ratio between screen units and voxels in all dimensions
			glm::vec4 &translationCol = m[3];
			translationCol[0] *= (footer.scale * nodeScale[0]);
			translationCol[1] *= (footer.scale * nodeScale[1]);
			translationCol[2] *= (footer.scale * nodeScale[2]);
			//m *= glm::translate(footer.mins);
		}
	}

	return true;
}

bool VXLFormat::loadHVA(const core::String &filename, const VXLModel &mdl, SceneGraph& sceneGraph) {
	HVAModel file;
	{
		const io::FilesystemPtr &filesystem = io::filesystem();
		io::FilePtr hvaFile = filesystem->open(filename);
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
		for (uint32_t section = 0; section < file.header.numNodes; ++section) {
			const core::String &name = file.header.nodeNames[section];
			SceneGraphNode* node = sceneGraph.findNodeByName(name);
			if (node == nullptr) {
				Log::debug("Can't find node with name %s", name.c_str());
				continue;
			}
			SceneGraphKeyFrame &kf = node->keyFrame(keyFrameIdx);
			kf.frameIdx = keyFrameIdx * 6; // running at 6 fps
			// hva transforms are overriding the vxl transform
			SceneGraphTransform transform;
			// transform.setPivot(mdl.nodeFooters[section].mins / size);
			transform.setLocalMatrix(sectionMatrices[section]);
			kf.setTransform(transform);
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
			glm::mat4 matrix = transform.localMatrix();
#if 0
			//const glm::vec3 size(node.region().getDimensionsInVoxels());
			const glm::vec3 nodeScale(1.0f); // TODO (footer.maxs - footer.mins) / size;
			// The HVA transformation matrices must be scaled - the VXL ones not!
			glm::vec4 &v = matrix[3];
			v[0] /= (transform.scale() * nodeScale[0]);
			v[1] /= (transform.scale() * nodeScale[1]);
			v[2] /= (transform.scale() * nodeScale[2]);
#endif
			for (int row = 0; row < 3; ++row) {
				for (int col = 0; col < 4; ++col) {
					wrapBool(stream.writeFloat(matrix[col][row]))
				}
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

	return true;
}

#undef wrap
#undef wrapBool

}
