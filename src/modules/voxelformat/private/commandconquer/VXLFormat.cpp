/**
 * @file
 */

#include "VXLFormat.h"
#include "core/Assert.h"
#include "core/Common.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "core/collection/Buffer.h"
#include "core/collection/StringSet.h"
#include "io/Archive.h"
#include "io/Stream.h"
#include "io/StreamUtil.h"
#include "palette/NormalPalette.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelformat/private/commandconquer/HVAFormat.h"
#include "voxelformat/private/commandconquer/VXLShared.h"
#include "voxelutil/VoxelUtil.h"
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/matrix.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/transform.hpp>

namespace voxelformat {

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

bool VXLFormat::writeLayerBodyEntry(io::SeekableWriteStream &stream, const voxel::RawVolume *volume, int x,
									int y, int z, uint8_t skipCount, uint8_t voxelCount) const {
	Log::trace("skipCount: %i voxelCount: %i for (x/y/z: %i:%i:%i)", skipCount, voxelCount, x, y, z);

	wrapBool(stream.writeUInt8(skipCount))
	wrapBool(stream.writeUInt8(voxelCount))

	voxel::RawVolume::Sampler sampler(*volume);
	sampler.setPosition(x, y, z);
	for (uint8_t i = 0; i < voxelCount; ++i) {
		const voxel::Voxel &voxel = sampler.voxel();
		wrapBool(stream.writeUInt8(voxel.getColor()))
		wrapBool(stream.writeUInt8(voxel.getNormal() == NO_NORMAL ? 0 : voxel.getNormal() - NORMAL_PALETTE_OFFSET))
		sampler.movePositiveY();
	}
	wrapBool(stream.writeUInt8(voxelCount)) // duplicated count
	return true;
}

static int calculateSpanLength(const voxel::RawVolume *v, int x, int y, int z) {
	const voxel::Region &region = v->region();
	int length = 0;
	voxel::RawVolume::Sampler sampler(*v);
	sampler.setPosition(x, y, z);
	for (; y <= region.getUpperY(); ++y) {
		if (voxel::isAir(sampler.voxel().getMaterial())) {
			break;
		}
		sampler.movePositiveY();
		++length;
	}
	return length;
}

static bool spanIsEmpty(const voxel::RawVolume *v, int x, int z) {
	const voxel::Region &region = v->region();
	voxel::RawVolume::Sampler sampler(*v);
	sampler.setPosition(x, region.getLowerY(), z);
	for (int y = region.getLowerY(); y <= region.getUpperY(); ++y) {
		if (!voxel::isAir(sampler.voxel().getMaterial())) {
			return false;
		}
		sampler.movePositiveY();
	}
	return true;
}

bool VXLFormat::writeLayer(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph,
						   const scenegraph::SceneGraphNode &node, vxl::VXLLayerOffset &offsets,
						   uint64_t nodeSectionOffset) const {
	const voxel::Region &region = sceneGraph.resolveRegion(node);
	const glm::ivec3 &size = region.getDimensionsInVoxels();
	if (glm::any(glm::greaterThan(size, maxSize()))) {
		Log::error("Node %i exceeds max supported dimensions", node.id());
		return false;
	}

	// swap y and z here
	const uint32_t baseSize = size.x * size.z;
	const int64_t globalSpanStartPos = stream.pos();
	Log::debug("size.x: %i, size.y: %i, size.z: %i, globalSpanStartPos: %u", size.x, size.y, size.z,
			   (uint32_t)globalSpanStartPos);
	Log::debug("Write layer body at %u", (int)globalSpanStartPos);

	offsets.start = stream.pos() - (int64_t)nodeSectionOffset;

	for (uint32_t i = 0; i < baseSize; i++) {
		wrapBool(stream.writeInt32(-1))
	}
	offsets.end = stream.pos() - (int64_t)nodeSectionOffset;
	for (uint32_t i = 0; i < baseSize; i++) {
		wrapBool(stream.writeInt32(-1))
	}
	offsets.data = stream.pos() - (int64_t)nodeSectionOffset;

	const voxel::RawVolume *v = sceneGraph.resolveVolume(node);
	const int64_t spanDataOffset = stream.pos();
	for (uint32_t i = 0u; i < baseSize; ++i) {
		const int64_t spanStartPos = stream.pos();

		const int x = (i % size.x) + region.getLowerX();
		const int z = size.z - 1 - (i / size.x) + region.getLowerZ();

		int32_t spanStartOffset = vxl::EmptyColumn;
		int32_t spanEndOffset = vxl::EmptyColumn;
		int64_t spanEndPos = stream.pos();
		if (!spanIsEmpty(v, x, z)) {
			uint8_t skipCount = 0u;
			for (int y = region.getLowerY(); y <= region.getUpperY();) {
				int voxelCount = calculateSpanLength(v, x, y, z);
				if (voxelCount > 0) {
					wrapBool(writeLayerBodyEntry(stream, v, x, y, z, skipCount, voxelCount))
					y += voxelCount;
					skipCount = 0;
				} else {
					++skipCount;
					++y;
				}
			}
			if (skipCount > 0) {
				wrapBool(writeLayerBodyEntry(stream, v, 0, 0, 0, skipCount, 0))
			}
			spanEndPos = stream.pos();
			const int64_t spanDelta = spanEndPos - spanStartPos;
			spanStartOffset = (int32_t)(spanStartPos - spanDataOffset);
			spanEndOffset = (int32_t)(spanStartOffset + spanDelta - 1);
		}

		if (stream.seek(globalSpanStartPos + i * sizeof(uint32_t)) == -1) {
			Log::error("Failed to seek");
			return false;
		}
		wrapBool(stream.writeInt32(spanStartOffset))
		Log::trace("Write SpanStartPos: %i", spanStartOffset);

		if (stream.seek(globalSpanStartPos + (i + baseSize) * sizeof(uint32_t)) == -1) {
			Log::error("Failed to seek");
			return false;
		}
		wrapBool(stream.writeInt32(spanEndOffset))
		Log::trace("Write SpanEndPos: %i", spanEndOffset);

		if (stream.seek(spanEndPos) == -1) {
			Log::error("Failed to seek");
			return false;
		}
	}

	return true;
}

bool VXLFormat::writeLayerHeader(io::SeekableWriteStream &stream, const scenegraph::SceneGraphNode &node,
								 uint32_t nodeIdx) const {
	core_assert((uint64_t)stream.pos() == (uint64_t)(vxl::HeaderSize + nodeIdx * vxl::LayerHeaderSize));
	Log::debug("Write layer header at %u", (int)stream.pos());
	core::String name = node.name().substr(0, 15);
	if (stream.write(name.c_str(), name.size()) == -1) {
		Log::error("Failed to write layer header into stream");
		return false;
	}
	for (size_t i = 0; i < 16 - name.size(); ++i) {
		wrapBool(stream.writeUInt8(0))
	}
	wrapBool(stream.writeUInt32(nodeIdx))
	wrapBool(stream.writeUInt32(1))
	wrapBool(stream.writeUInt32(2))
	return true;
}

bool VXLFormat::writeLayerInfo(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph,
							   const scenegraph::SceneGraphNode &node, const vxl::VXLLayerOffset &offsets) const {
	Log::debug("SpanStartOffset: %i", (int32_t)offsets.start);
	Log::debug("SpanEndOffset: %i", (int32_t)offsets.end);
	Log::debug("SpanDataOffset: %i", (int32_t)offsets.data);
	Log::debug("Write layer footer at %u", (int)stream.pos());
	wrapBool(stream.writeUInt32((uint32_t)offsets.start))
	wrapBool(stream.writeUInt32((uint32_t)offsets.end))
	wrapBool(stream.writeUInt32((uint32_t)offsets.data))

	const scenegraph::FrameIndex frameIdx = 0;
	const scenegraph::SceneGraphTransform &transform = node.transform(frameIdx);
	const voxel::Region &region = sceneGraph.resolveRegion(node);
	const glm::ivec3 &size = region.getDimensionsInVoxels();
	core_assert(!glm::any(glm::greaterThan(size, maxSize())));
	// TODO: VOXELFORMAT: check pivot handling (https://github.com/vengi-voxel/vengi/issues/537)
	const glm::mat4 &localMatrix = transform.localMatrix();
	const glm::vec3 scale(glm::length(localMatrix[0]), glm::length(localMatrix[1]), glm::length(localMatrix[2]));
	const glm::vec3 mins = node.pivot() * glm::vec3(-size) * scale;
	const vxl::VXLMatrix &vxlMatrix = vxl::convertVXLWrite(localMatrix);

	// this is always the same and NOT the transform scale
	// the transform scale is calculated by the bounding box when loading the model
	wrapBool(stream.writeFloat(vxl::Scale))

	for (int j = 0; j < 12; ++j) {
		const int col = j % 4;
		const int row = j / 4;
		float val = vxlMatrix.matrix[col][row];
		wrapBool(stream.writeFloat(val))
	}

	// swap y and z here
	wrapBool(stream.writeFloat(mins.x))
	wrapBool(stream.writeFloat(mins.z))
	wrapBool(stream.writeFloat(mins.y))

	const glm::vec3 maxs = mins + glm::vec3(size) * scale;
	wrapBool(stream.writeFloat(maxs.x))
	wrapBool(stream.writeFloat(maxs.z))
	wrapBool(stream.writeFloat(maxs.y))

	wrapBool(stream.writeUInt8(size.x))
	wrapBool(stream.writeUInt8(size.z))
	wrapBool(stream.writeUInt8(size.y))

	if (node.hasNormalPalette() && (node.normalPalette().size() == 36 || node.normalPalette().isTiberianSun())) {
		wrapBool(stream.writeUInt8(2))
	} else {
		wrapBool(stream.writeUInt8(4))
	}

	return true;
}

bool VXLFormat::writeHeader(io::SeekableWriteStream &stream, uint32_t numNodes, const palette::Palette &palette) {
	vxl::VXLHeader header;
	SDL_strlcpy(header.filetype, "Voxel Animation", sizeof(header.filetype));
	header.paletteCount = 1;
	header.layerCount = numNodes;
	header.layerInfoCount = numNodes;
	header.dataSize = 0; // bodysize is filled later

	wrapBool(stream.writeString(header.filetype, true))
	wrapBool(stream.writeUInt32(header.paletteCount))
	wrapBool(stream.writeUInt32(header.layerCount))
	wrapBool(stream.writeUInt32(header.layerInfoCount))
	wrapBool(stream.writeUInt32(header.dataSize))

	wrapBool(stream.writeUInt8(16)) // startPaletteRemap
	wrapBool(stream.writeUInt8(31)) // endPaletteRemap
	for (int i = 0; i < palette.colorCount(); ++i) {
		const color::RGBA &rgba = palette.color(i);
		wrapBool(stream.writeUInt8(rgba.r))
		wrapBool(stream.writeUInt8(rgba.g))
		wrapBool(stream.writeUInt8(rgba.b))
	}
	for (int i = palette.colorCount(); i < palette::PaletteMaxColors; ++i) {
		wrapBool(stream.writeUInt8(0))
		wrapBool(stream.writeUInt8(0))
		wrapBool(stream.writeUInt8(0))
	}
	core_assert(stream.pos() == vxl::HeaderSize);
	return true;
}

bool VXLFormat::saveVXL(const scenegraph::SceneGraph &sceneGraph,
						core::Buffer<const scenegraph::SceneGraphNode *> &nodes, const core::String &filename,
						const io::ArchivePtr &archive) {
	if (nodes.empty()) {
		return false;
	}
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Failed to open stream for file: %s", filename.c_str());
		return false;
	}

	const uint32_t numLayers = nodes.size();
	wrapBool(writeHeader(*stream, numLayers, nodes[0]->palette()))
	for (uint32_t i = 0; i < numLayers; ++i) {
		const scenegraph::SceneGraphNode *node = nodes[(int)i];
		wrapBool(writeLayerHeader(*stream, *node, i))
	}

	{
		core::StringSet names;
		for (uint32_t i = 0; i < numLayers; ++i) {
			const scenegraph::SceneGraphNode *node = nodes[(int)i];
			if (!names.insert(node->name())) {
				Log::warn("Duplicated layer name found: %s - this will lead to errors for hva loading",
						node->name().c_str());
			}
		}
	}

	core::Buffer<vxl::VXLLayerOffset> layerOffsets(numLayers);
	const uint64_t bodyStart = stream->pos();
	for (uint32_t i = 0; i < numLayers; ++i) {
		const scenegraph::SceneGraphNode *node = nodes[(int)i];
		wrapBool(writeLayer(*stream, sceneGraph, *node, layerOffsets[i], bodyStart))
	}

	const uint64_t afterBodyPos = stream->pos();
	const uint64_t bodySize = afterBodyPos - bodyStart;
	Log::debug("write %u bytes as body size", (uint32_t)bodySize);
	if (stream->seek(vxl::HeaderBodySizeOffset) == -1) {
		Log::error("Failed to seek to body size");
		return false;
	}
	wrapBool(stream->writeUInt32((uint32_t)bodySize))
	if (stream->seek(afterBodyPos) == -1) {
		Log::error("Failed to seek to after body");
		return false;
	}

	core_assert((uint64_t)stream->pos() == (uint64_t)(vxl::HeaderSize + vxl::LayerHeaderSize * numLayers + bodySize));

	for (uint32_t i = 0; i < numLayers; ++i) {
		const scenegraph::SceneGraphNode *node = nodes[(int)i];
		wrapBool(writeLayerInfo(*stream, sceneGraph, *node, layerOffsets[i]))
	}
	return true;
}

bool VXLFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
						   const io::ArchivePtr &archive, const SaveContext &ctx) {
	core::Buffer<const scenegraph::SceneGraphNode *> body;
	core::Buffer<const scenegraph::SceneGraphNode *> barrel;
	core::Buffer<const scenegraph::SceneGraphNode *> turret;

	const size_t numNodes = sceneGraph.size(scenegraph::SceneGraphNodeType::AllModels);
	body.reserve(numNodes);
	barrel.reserve(numNodes);
	turret.reserve(numNodes);

	for (auto iter = sceneGraph.beginAllModels(); iter != sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
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

	if (!saveVXL(sceneGraph, body, filename, archive)) {
		return false;
	}
	if (!barrel.empty()) {
		const core::String &extFilename = basename + "barl.vxl";
		if (!saveVXL(sceneGraph, barrel, extFilename, archive)) {
			Log::warn("Failed to write %s", extFilename.c_str());
		}
	}
	if (!turret.empty()) {
		const core::String &extFilename = basename + "tur.vxl";
		if (!saveVXL(sceneGraph, turret, extFilename, archive)) {
			Log::warn("Failed to write %s", extFilename.c_str());
		}
	}
	HVAFormat hva;
	return hva.saveHVA(basename + ".hva", archive, sceneGraph);
}

bool VXLFormat::readLayer(io::SeekableReadStream &stream, vxl::VXLModel &mdl, uint32_t nodeIdx,
						  scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette) const {
	const uint64_t nodeStart = stream.pos();
	const vxl::VXLLayerInfo &footer = mdl.layerInfos[nodeIdx];
	const vxl::VXLLayerHeader &header = mdl.layerHeaders[nodeIdx];

	const uint32_t baseSize = footer.xsize * footer.ysize;
	core::Buffer<int32_t> colStart(baseSize);
	core::Buffer<int32_t> colEnd(baseSize);

	Log::debug("Read layer body at %u", (int)nodeStart);

	if (stream.skip(footer.spanStartOffset) == -1) {
		Log::error("Failed to skip %u layer start offset bytes", footer.spanStartOffset);
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
		Log::error("Invalid offset found for layer %u: %u", nodeIdx, footer.spanStartOffset);
		return false;
	}

	// switch axis
	const voxel::Region region{0, 0, 0, (int)footer.xsize - 1, (int)footer.zsize - 1, (int)footer.ysize - 1};
	if (!region.isValid()) {
		Log::error("Failed to load section with invalid size: %i:%i:%i", (int)footer.xsize, (int)footer.zsize,
				   (int)footer.ysize);
		return false;
	}
	// y and z are switched here
	Log::debug("size.x: %i, size.y: %i, size.z: %i", footer.xsize, (int)footer.zsize, (int)footer.ysize);
	voxel::RawVolume *volume = new voxel::RawVolume(region);
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(volume, true);
	node.setName(header.name);
	node.setPivot(footer.pivot());
	if (palette.colorCount() > 0) {
		node.setPalette(palette);
	}
	scenegraph::SceneGraphTransform transform;
	transform.setLocalMatrix(vxl::convertVXLRead(footer.transform, footer));
	const scenegraph::KeyFrameIndex keyFrameIdx = 0;
	node.setTransform(keyFrameIdx, transform);
	uint8_t maxNormalIndex = 0;

	for (uint32_t i = 0u; i < baseSize; ++i) {
		Log::trace("Read SpanStartPos: %i", (int)colStart[i]);
		Log::trace("Read SpanEndPos: %i", (int)colEnd[i]);
		if (colStart[i] == vxl::EmptyColumn || colEnd[i] == vxl::EmptyColumn) {
			continue;
		}

		if (stream.seek(dataStart + colStart[i]) == -1) {
			Log::error("Failed to seek to column start");
			return false;
		}

		const uint8_t x = (uint8_t)(i % footer.xsize);
		const uint8_t y = (uint8_t)(i / footer.xsize);
		uint8_t z = 0;
		glm::ivec3 pos(x, z, footer.ysize - y - 1);
		while (z < footer.zsize) {
			uint8_t skipCount;
			wrap(stream.readUInt8(skipCount))
			z += skipCount;
			uint8_t voxelCount;
			wrap(stream.readUInt8(voxelCount))

			Log::trace("skipCount: %i voxelCount: %i", (int)skipCount, (int)voxelCount);

			voxel::RawVolume::Sampler sampler(volume);
			sampler.setPosition(pos.x, z, pos.z);
			for (uint8_t j = 0u; j < voxelCount; ++j) {
				uint8_t color;
				wrap(stream.readUInt8(color))
				uint8_t normal;
				wrap(stream.readUInt8(normal))
				maxNormalIndex = core_max(maxNormalIndex, normal);
				const voxel::Voxel v = voxel::createVoxel(palette, color, normal);
				sampler.setVoxel(v);
				sampler.movePositiveY();
				++z;
			}

			// Skip duplicate count
			stream.skip(1);
		}
	}

	palette::NormalPalette normalPalette;
	// TODO: VOXELFORMAT: index 1 and 3 https://modenc.renegadeprojects.com/Normals#Index_1_Normals
	if (footer.normalType == 2) {
		normalPalette.tiberianSun();
	} else {
		normalPalette.redAlert2();
	}
	node.setNormalPalette(normalPalette);
	sceneGraph.emplace(core::move(node));
	return true;
}

bool VXLFormat::readLayers(io::SeekableReadStream &stream, vxl::VXLModel &mdl, scenegraph::SceneGraph &sceneGraph,
						   const palette::Palette &palette) const {
	const vxl::VXLHeader &hdr = mdl.header;
	sceneGraph.reserve(hdr.layerCount);
	const int64_t bodyPos = stream.pos();
	for (uint32_t i = 0; i < hdr.layerCount; ++i) {
		if (stream.seek(bodyPos) == -1) {
			Log::error("Failed to seek for layer %u", i);
			return false;
		}
		wrapBool(readLayer(stream, mdl, i, sceneGraph, palette))
	}
	return true;
}

bool VXLFormat::readLayerHeader(io::SeekableReadStream &stream, vxl::VXLModel &mdl, uint32_t layerIdx) const {
	vxl::VXLLayerHeader &header = mdl.layerHeaders[layerIdx];
	Log::debug("Read layer header at %u", (int)stream.pos());
	wrapBool(stream.readString(lengthof(header.name), header.name, false))
	wrap(stream.readUInt32(header.infoIndex))
	wrap(stream.readUInt32(header.unknown))
	wrap(stream.readUInt32(header.unknown2))
	Log::debug("Node %u name: %s, id %u, unknown: %u, unknown2: %u", layerIdx, header.name, header.infoIndex,
			   header.unknown, header.unknown2);
	return true;
}

bool VXLFormat::readLayerHeaders(io::SeekableReadStream &stream, vxl::VXLModel &mdl) const {
	for (uint32_t i = 0; i < mdl.header.layerCount; ++i) {
		wrapBool(readLayerHeader(stream, mdl, i))
	}
	core::StringSet names;
	for (uint32_t i = 0; i < mdl.header.layerCount; ++i) {
		if (!names.insert(mdl.layerHeaders[i].name)) {
			Log::warn("Duplicated layer name found: %s - this will lead to errors for hva loading",
					  mdl.layerHeaders[i].name);
		}
	}
	return true;
}

bool VXLFormat::readLayerInfo(io::SeekableReadStream &stream, vxl::VXLModel &mdl, uint32_t nodeIdx) const {
	vxl::VXLLayerInfo &footer = mdl.layerInfos[nodeIdx];
	Log::debug("Read layer footer at %u", (int)stream.pos());
	wrap(stream.readUInt32(footer.spanStartOffset))
	wrap(stream.readUInt32(footer.spanEndOffset))
	wrap(stream.readUInt32(footer.spanDataOffset))
	wrap(stream.readFloat(footer.scale))

	for (int j = 0; j < 12; ++j) {
		const int col = j % 4;
		const int row = j / 4;
		float &val = footer.transform.matrix[col][row];
		wrap(stream.readFloat(val))
	}
	wrapBool(io::readVec3(stream, footer.mins))
	wrapBool(io::readVec3(stream, footer.maxs))

	wrap(stream.readUInt8(footer.xsize))
	wrap(stream.readUInt8(footer.ysize))
	wrap(stream.readUInt8(footer.zsize))
	wrap(stream.readUInt8(footer.normalType))

	if (footer.xsize == 0 || footer.ysize == 0 || footer.zsize == 0) {
		Log::error("Invalid layer size found");
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

bool VXLFormat::readLayerInfos(io::SeekableReadStream &stream, vxl::VXLModel &mdl) const {
	const vxl::VXLHeader &hdr = mdl.header;
	if (stream.seek(vxl::HeaderSize + vxl::LayerHeaderSize * hdr.layerCount + hdr.dataSize) == -1) {
		Log::error("Failed to seek to layer info");
		return false;
	}
	for (uint32_t i = 0; i < hdr.layerInfoCount; ++i) {
		wrapBool(readLayerInfo(stream, mdl, i))
	}
	return true;
}

bool VXLFormat::readHeader(io::SeekableReadStream &stream, vxl::VXLModel &mdl, palette::Palette &palette) {
	vxl::VXLHeader &hdr = mdl.header;
	wrapBool(stream.readString(lengthof(hdr.filetype), hdr.filetype, false))
	if (SDL_strcmp(hdr.filetype, "Voxel Animation") != 0) {
		Log::error("Invalid vxl header");
		return false;
	}
	wrap(stream.readUInt32(hdr.paletteCount))
	wrap(stream.readUInt32(hdr.layerCount))
	wrap(stream.readUInt32(hdr.layerInfoCount))
	wrap(stream.readUInt32(hdr.dataSize))

	Log::debug("Palettes: %u", hdr.paletteCount);
	Log::debug("Nodes: %u", hdr.layerCount);
	Log::debug("Tailers: %u", hdr.layerInfoCount);
	Log::debug("BodySize: %u", hdr.dataSize);

	palette.setSize(palette::PaletteMaxColors);
	bool valid = false;
	for (uint32_t n = 0; n < hdr.paletteCount; ++n) {
		wrap(stream.readUInt8(hdr.palette.startPaletteRemap)) // 0x1f
		wrap(stream.readUInt8(hdr.palette.endPaletteRemap))	  // 0x10
		Log::debug("palette %u: %u start, %u end palette offset", n, hdr.palette.startPaletteRemap,
				   hdr.palette.endPaletteRemap);
		for (int i = 0; i < palette.colorCount(); ++i) {
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
		for (int i = 0; i < palette.colorCount(); ++i) {
			const uint8_t *p = hdr.palette.palette[i];
			palette.setColor(i, color::RGBA(p[0], p[1], p[2]));
		}
	} else {
		palette.commandAndConquer();
		Log::debug("No palette found in vxl");
	}

	return true;
}

bool VXLFormat::prepareModel(vxl::VXLModel &mdl) const {
	const vxl::VXLHeader &hdr = mdl.header;
	if (hdr.layerCount > vxl::MaxLayers) {
		Log::error("Node size exceeded the max allowed value: %u", hdr.layerCount);
		return false;
	}
	return true;
}

size_t VXLFormat::loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
							  const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Failed to open stream for file: %s", filename.c_str());
		return 0;
	}
	vxl::VXLModel mdl;
	if (!readHeader(*stream, mdl, palette)) {
		return 0;
	}
	return palette.colorCount();
}

bool VXLFormat::loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
								  scenegraph::SceneGraph &sceneGraph, palette::Palette &palette, const LoadContext &ctx) {
	vxl::VXLModel mdl;
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Failed to open stream for file: %s", filename.c_str());
		return false;
	}

	wrapBool(readHeader(*stream, mdl, palette))
	wrapBool(prepareModel(mdl))

	wrapBool(readLayerHeaders(*stream, mdl))
	const int64_t bodyPos = stream->pos();
	if (stream->skip(mdl.header.dataSize) == -1) {
		Log::error("Failed to skip %u bytes", mdl.header.dataSize);
		return false;
	}
	wrapBool(readLayerInfos(*stream, mdl))

	if (stream->seek(bodyPos) == -1) {
		Log::error("Failed to seek");
		return false;
	}
	wrapBool(readLayers(*stream, mdl, sceneGraph, palette))

	const core::String &basename = core::string::stripExtension(filename);

	const bool loadHVA = core::getVar(cfg::VoxformatVXLLoadHVA)->boolVal();
	if (loadHVA && archive->exists(basename + ".hva")) {
		HVAFormat hva;
		wrapBool(hva.loadHVA(basename + ".hva", archive, mdl, sceneGraph))
	}

	if (!core::string::endsWith(filename, "barl.vxl")) {
		if (archive->exists(basename + "barl.vxl")) {
			wrapBool(loadGroupsPalette(basename + "barl.vxl", archive, sceneGraph, palette, ctx))
		}
	}
	if (!core::string::endsWith(filename, "tur.vxl")) {
		if (archive->exists(basename + "tur.vxl")) {
			wrapBool(loadGroupsPalette(basename + "tur.vxl", archive, sceneGraph, palette, ctx))
		}
	}

	return true;
}

#undef wrap
#undef wrapBool

} // namespace voxelformat
