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
#include "scenegraph/CoordinateSystem.h"
#include "scenegraph/CoordinateSystemUtil.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/MaterialColor.h"
#include "voxel/Palette.h"
#include "voxel/PaletteLookup.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/matrix.hpp>

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

namespace priv {
static const float Scale = 1.0f / 12.0f;
}

void VXLFormat::VXLMatrix::fromVengi(const glm::mat4 &vengiMatrix) {
	matrix = scenegraph::convertCoordinateSystem(scenegraph::CoordinateSystem::Vengi, scenegraph::CoordinateSystem::VXL,
												 vengiMatrix);
}

glm::mat4 VXLFormat::VXLMatrix::toVengi() const {
	return scenegraph::convertCoordinateSystem(scenegraph::CoordinateSystem::VXL, scenegraph::CoordinateSystem::Vengi,
											   matrix);
}

void VXLFormat::convertRead(glm::mat4 &vengiMatrix, const VXLLayerInfo &footer, bool hva) {
	glm::vec4 &translation = vengiMatrix[3];
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

void VXLFormat::convertWrite(VXLMatrix &vxlMatrix, const glm::mat4 &vengiMatrix, const glm::vec3 &localTranslate, bool hva) {
	vxlMatrix.fromVengi(vengiMatrix);

	// swap y and z here
	// TODO: or the pivot?
	vxlMatrix.matrix[3][0] -= localTranslate.x;
	vxlMatrix.matrix[3][1] -= localTranslate.z;
	vxlMatrix.matrix[3][2] -= localTranslate.y;

	if (hva) {
		// Calculate the ratio between screen units and voxels in all dimensions
		vxlMatrix.matrix[3][0] /= priv::Scale;
		vxlMatrix.matrix[3][1] /= priv::Scale;
		vxlMatrix.matrix[3][2] /= priv::Scale;
	}
}

bool VXLFormat::writeLayerBodyEntry(io::SeekableWriteStream &stream, const voxel::RawVolume *volume, uint8_t x,
									uint8_t y, uint8_t z, uint8_t skipCount, uint8_t voxelCount,
									uint8_t normalType) const {
	Log::trace("skipCount: %i voxelCount: %i", skipCount, voxelCount);
	core_assert(skipCount <= 255);
	core_assert(voxelCount <= 255);

	wrapBool(stream.writeUInt8(skipCount))
	wrapBool(stream.writeUInt8(voxelCount))

	for (uint8_t i = 0; i < voxelCount; ++i) {
		const voxel::Voxel &voxel = volume->voxel(x, y + i, z);
		wrapBool(stream.writeUInt8(voxel.getColor()))
		uint8_t normalIndex = 0;
		// TODO: normal
		// if (normalType == 2) { // Tiberian Sun
		// } else if (normalType == 4) { // Red Alert
		// }
		wrapBool(stream.writeUInt8(normalIndex))
	}
	wrapBool(stream.writeUInt8(voxelCount)) // duplicated count
	return true;
}

static int calculateSpanLength(const voxel::RawVolume *v, int x, int y, int z) {
	const voxel::Region &region = v->region();
	int length = 0;
	for (; y <= region.getUpperY(); ++y) {
		if (voxel::isAir(v->voxel(x, y, z).getMaterial())) {
			break;
		}
		++length;
	}
	return length;
}

static bool spanIsEmpty(const voxel::RawVolume *v, int x, int z) {
	const voxel::Region &region = v->region();
	for (int y = region.getLowerY(); y <= region.getUpperY(); ++y) {
		if (!voxel::isAir(v->voxel(x, y, z).getMaterial())) {
			return false;
		}
	}
	return true;
}

bool VXLFormat::writeLayer(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph,
						   const scenegraph::SceneGraphNode &node, VXLLayerOffset &offsets,
						   uint64_t nodeSectionOffset) const {
	const voxel::Region &region = node.region();
	const glm::ivec3 &size = region.getDimensionsInVoxels();
	if (size.x > 255 || size.y > 255 || size.z > 255) {
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

	const uint8_t normalType = core::Var::getSafe(cfg::VoxformatVXLNormalType)->intVal();

	const int64_t spanDataOffset = stream.pos();
	for (uint32_t i = 0u; i < baseSize; ++i) {
		const int64_t spanStartPos = stream.pos();

		const uint8_t x = (uint8_t)(i % size.x);
		const uint8_t z = (uint8_t)(i / size.x);

		int32_t spanStartOffset = EmptyColumn;
		int32_t spanEndOffset = EmptyColumn;
		int64_t spanEndPos = stream.pos();
		if (!spanIsEmpty(node.volume(), x, z)) {
			uint8_t skipCount = 0u;
			for (int y = region.getLowerY(); y <= region.getUpperY();) {
				int voxelCount = calculateSpanLength(node.volume(), x, y, z);
				if (voxelCount > 0) {
					wrapBool(writeLayerBodyEntry(stream, node.volume(), x, y, z, skipCount, voxelCount, normalType))
					y += voxelCount;
					skipCount = 0;
				} else {
					++skipCount;
					++y;
				}
			}
			if (skipCount > 0) {
				wrapBool(writeLayerBodyEntry(stream, node.volume(), 0, 0, 0, skipCount, 0, normalType))
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
	core_assert((uint64_t)stream.pos() == (uint64_t)(HeaderSize + nodeIdx * LayerHeaderSize));
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
	wrapBool(stream.writeUInt32(0))
	return true;
}

bool VXLFormat::writeLayerInfo(io::SeekableWriteStream &stream, const scenegraph::SceneGraphNode &node,
							   const VXLLayerOffset &offsets) const {
	Log::debug("SpanStartOffset: %i", (int32_t)offsets.start);
	Log::debug("SpanEndOffset: %i", (int32_t)offsets.end);
	Log::debug("SpanDataOffset: %i", (int32_t)offsets.data);
	Log::debug("Write layer footer at %u", (int)stream.pos());
	wrapBool(stream.writeUInt32(offsets.start))
	wrapBool(stream.writeUInt32(offsets.end))
	wrapBool(stream.writeUInt32(offsets.data))

	const scenegraph::FrameIndex frameIdx = 0;
	const scenegraph::SceneGraphTransform &transform = node.transform(frameIdx);
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

	const voxel::Region &region = node.region();
	const glm::ivec3 &size = region.getDimensionsInVoxels();
	if (size.x > 0xFF || size.y > 0xFF || size.z > 0xFF) {
		Log::error("Failed to write vxl layer footer - max volume size exceeded");
		return false;
	}

	// swap y and z here
	// TODO: should we take the region mins into account here, too? Otherwise the
	// result might be different from what you've seen in the editor
	wrapBool(stream.writeFloat(mins.x /*+ region.getLowerX()*/))
	wrapBool(stream.writeFloat(mins.z /*+ region.getLowerZ()*/))
	wrapBool(stream.writeFloat(mins.y /*+ region.getLowerY()*/))

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

bool VXLFormat::writeHeader(io::SeekableWriteStream &stream, uint32_t numNodes, const voxel::Palette &palette) {
	VXLHeader header;
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

	wrapBool(stream.writeUInt8(0x10U)) // startPaletteRemap
	wrapBool(stream.writeUInt8(0x1fU)) // endPaletteRemap
	for (int i = 0; i < palette.colorCount(); ++i) {
		const core::RGBA &rgba = palette.color(i);
		wrapBool(stream.writeUInt8(rgba.r))
		wrapBool(stream.writeUInt8(rgba.g))
		wrapBool(stream.writeUInt8(rgba.b))
	}
	for (int i = palette.colorCount(); i < voxel::PaletteMaxColors; ++i) {
		wrapBool(stream.writeUInt8(0))
		wrapBool(stream.writeUInt8(0))
		wrapBool(stream.writeUInt8(0))
	}
	core_assert(stream.pos() == HeaderSize);
	return true;
}

bool VXLFormat::saveVXL(const scenegraph::SceneGraph &sceneGraph,
						core::DynamicArray<const scenegraph::SceneGraphNode *> &nodes, const core::String &filename,
						io::SeekableWriteStream &stream) {
	if (nodes.empty()) {
		return false;
	}
	const uint32_t numLayers = nodes.size();
	wrapBool(writeHeader(stream, numLayers, nodes[0]->palette()))
	for (uint32_t i = 0; i < numLayers; ++i) {
		const scenegraph::SceneGraphNode *node = nodes[(int)i];
		wrapBool(writeLayerHeader(stream, *node, i))
	}

	core::Buffer<VXLLayerOffset> layerOffsets(numLayers);
	const uint64_t bodyStart = stream.pos();
	for (uint32_t i = 0; i < numLayers; ++i) {
		const scenegraph::SceneGraphNode *node = nodes[(int)i];
		wrapBool(writeLayer(stream, sceneGraph, *node, layerOffsets[i], bodyStart))
	}

	const uint64_t afterBodyPos = stream.pos();
	const uint64_t bodySize = afterBodyPos - bodyStart;
	Log::debug("write %u bytes as body size", (uint32_t)bodySize);
	wrap(stream.seek(HeaderBodySizeOffset));
	wrapBool(stream.writeUInt32(bodySize))
	wrap(stream.seek(afterBodyPos));

	core_assert((uint64_t)stream.pos() == (uint64_t)(HeaderSize + LayerHeaderSize * numLayers + bodySize));

	for (uint32_t i = 0; i < numLayers; ++i) {
		const scenegraph::SceneGraphNode *node = nodes[(int)i];
		wrapBool(writeLayerInfo(stream, *node, layerOffsets[i]))
	}
	return true;
}

bool VXLFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
						   io::SeekableWriteStream &stream, const SaveContext &ctx) {
	core::DynamicArray<const scenegraph::SceneGraphNode *> body;
	core::DynamicArray<const scenegraph::SceneGraphNode *> barrel;
	core::DynamicArray<const scenegraph::SceneGraphNode *> turret;

	const uint32_t numNodes = sceneGraph.size();
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

	if (!saveVXL(sceneGraph, body, filename, stream)) {
		return false;
	}
	if (!barrel.empty()) {
		const core::String &extFilename = basename + "barl.vxl";
		io::FileStream extStream(io::filesystem()->open(extFilename, io::FileMode::SysWrite));
		if (extStream.valid() && !saveVXL(sceneGraph, barrel, extFilename, extStream)) {
			Log::warn("Failed to write %s", extFilename.c_str());
		}
	}
	if (!turret.empty()) {
		const core::String &extFilename = basename + "tur.vxl";
		io::FileStream extStream(io::filesystem()->open(extFilename, io::FileMode::SysWrite));
		if (extStream.valid() && !saveVXL(sceneGraph, turret, extFilename, extStream)) {
			Log::warn("Failed to write %s", extFilename.c_str());
		}
	}
	return saveHVA(basename + ".hva", sceneGraph);
}

bool VXLFormat::readLayer(io::SeekableReadStream &stream, VXLModel &mdl, uint32_t nodeIdx,
						  scenegraph::SceneGraph &sceneGraph, const voxel::Palette &palette) const {
	const uint64_t nodeStart = stream.pos();
	const VXLLayerInfo &footer = mdl.layerInfos[nodeIdx];
	const VXLLayerHeader &header = mdl.layerHeaders[nodeIdx];

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
	scenegraph::SceneGraphNode node;
	node.setVolume(volume, true);
	node.setName(header.name);
	if (palette.colorCount() > 0) {
		node.setPalette(palette);
	}

	glm::mat4 glmMatrix = footer.transform.toVengi();
	convertRead(glmMatrix, footer, false);

	scenegraph::SceneGraphTransform transform;
	transform.setLocalMatrix(glmMatrix);
	const scenegraph::KeyFrameIndex keyFrameIdx = 0;
	node.setTransform(keyFrameIdx, transform);

	for (uint32_t i = 0u; i < baseSize; ++i) {
		Log::trace("Read SpanStartPos: %i", (int)colStart[i]);
		Log::trace("Read SpanEndPos: %i", (int)colEnd[i]);
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

			Log::trace("skipCount: %i voxelCount: %i", (int)skipCount, (int)voxelCount);

			for (uint8_t j = 0u; j < voxelCount; ++j) {
				uint8_t color;
				wrap(stream.readUInt8(color))
				uint8_t normal;
				wrap(stream.readUInt8(normal))
				const voxel::Voxel v = voxel::createVoxel(palette, color);
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

bool VXLFormat::readLayers(io::SeekableReadStream &stream, VXLModel &mdl, scenegraph::SceneGraph &sceneGraph,
						   const voxel::Palette &palette) const {
	const VXLHeader &hdr = mdl.header;
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

bool VXLFormat::readLayerHeader(io::SeekableReadStream &stream, VXLModel &mdl, uint32_t layerIdx) const {
	VXLLayerHeader &header = mdl.layerHeaders[layerIdx];
	Log::debug("Read layer header at %u", (int)stream.pos());
	wrapBool(stream.readString(lengthof(header.name), header.name, false))
	wrap(stream.readUInt32(header.infoIndex))
	wrap(stream.readUInt32(header.unknown))
	wrap(stream.readUInt32(header.unknown2))
	Log::debug("Node %u name: %s, id %u, unknown: %u, unknown2: %u", layerIdx, header.name, header.infoIndex,
			   header.unknown, header.unknown2);
	return true;
}

bool VXLFormat::readLayerHeaders(io::SeekableReadStream &stream, VXLModel &mdl) const {
	for (uint32_t i = 0; i < mdl.header.layerCount; ++i) {
		wrapBool(readLayerHeader(stream, mdl, i))
	}
	return true;
}

bool VXLFormat::readLayerInfo(io::SeekableReadStream &stream, VXLModel &mdl, uint32_t nodeIdx) const {
	VXLLayerInfo &footer = mdl.layerInfos[nodeIdx];
	Log::debug("Read layer footer at %u", (int)stream.pos());
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

	core::Var::getSafe(cfg::VoxformatVXLNormalType)->setVal(footer.normalType);

	return true;
}

bool VXLFormat::readLayerInfos(io::SeekableReadStream &stream, VXLModel &mdl) const {
	const VXLHeader &hdr = mdl.header;
	wrap(stream.seek(HeaderSize + LayerHeaderSize * hdr.layerCount + hdr.dataSize))
	for (uint32_t i = 0; i < hdr.layerInfoCount; ++i) {
		wrapBool(readLayerInfo(stream, mdl, i))
	}
	return true;
}

bool VXLFormat::readHeader(io::SeekableReadStream &stream, VXLModel &mdl, voxel::Palette &palette) {
	VXLHeader &hdr = mdl.header;
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

	palette.setSize(voxel::PaletteMaxColors);
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
			palette.color(i) = core::RGBA(p[0], p[1], p[2]);
		}
	} else {
		palette.commandAndConquer();
		Log::warn("No palette found in vxl");
	}

	return true;
}

bool VXLFormat::prepareModel(VXLModel &mdl) const {
	const VXLHeader &hdr = mdl.header;
	if (hdr.layerCount > MaxLayers) {
		Log::error("Node size exceeded the max allowed value: %u", hdr.layerCount);
		return false;
	}
	return true;
}

bool VXLFormat::readHVAHeader(io::SeekableReadStream &stream, HVAHeader &header) const {
	char name[16];
	wrapBool(stream.readString(lengthof(name), name, false))
	header.filename = name;
	Log::debug("hva name: %s", header.filename.c_str());
	wrap(stream.readUInt32(header.numFrames))
	Log::debug("numframes: %i", header.numFrames);
	wrap(stream.readUInt32(header.numLayers))
	Log::debug("sections: %i", header.numLayers);
	for (uint32_t i = 0; i < header.numLayers; ++i) {
		wrapBool(stream.readString(lengthof(name), name, false))
		header.nodeNames[i] = name;
		Log::debug("hva section %u: %s", i, header.nodeNames[i].c_str());
	}
	return true;
}

int VXLFormat::VXLModel::findLayerByName(const core::String &name) const {
	for (uint32_t i = 0; i < header.layerCount; ++i) {
		if (name == layerHeaders[i].name) {
			return (int)i;
		}
	}
	return -1;
}

bool VXLFormat::readHVAFrames(io::SeekableReadStream &stream, const VXLModel &mdl, HVAModel &file) const {
	if (file.header.numLayers >= lengthof(file.frames)) {
		Log::error("Max allowed frame count exceeded");
		return false;
	}
	for (uint32_t i = 0; i < file.header.numLayers; ++i) {
		file.header.layerIds[i] = mdl.findLayerByName(file.header.nodeNames[i]);
		if (file.header.layerIds[i] == -1) {
			Log::debug("Failed to resolve layer id for '%s' (node idx: %i/%i)", file.header.nodeNames[i].c_str(), i,
					   file.header.numLayers);
			for (uint32_t i = 0; i < mdl.header.layerCount; ++i) {
				Log::debug(" - found: %s", mdl.layerHeaders[i].name);
			}
		}
	}

	for (uint32_t frameIdx = 0; frameIdx < file.header.numFrames; ++frameIdx) {
		HVAFrames &frame = file.frames[frameIdx];
		frame.resize(file.header.numLayers);
		for (uint32_t nodeIdx = 0; nodeIdx < file.header.numLayers; ++nodeIdx) {
			VXLMatrix &vxlMatrix = frame[nodeIdx];
			for (int i = 0; i < 12; ++i) {
				const int col = i % 4;
				const int row = i / 4;
				float &val = vxlMatrix.matrix[col][row];
				wrap(stream.readFloat(val))
			}
			Log::debug("load frame %u for layer %i with translation: %f:%f:%f", frameIdx, nodeIdx,
					   vxlMatrix.matrix[3][0], vxlMatrix.matrix[3][1], vxlMatrix.matrix[3][2]);
		}
	}

	return true;
}

bool VXLFormat::loadHVA(const core::String &filename, const VXLModel &mdl, scenegraph::SceneGraph &sceneGraph) {
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
		for (uint32_t vxlNodeId = 0; vxlNodeId < file.header.numLayers; ++vxlNodeId) {
			const core::String &name = file.header.nodeNames[vxlNodeId];
			scenegraph::SceneGraphNode *node = sceneGraph.findNodeByName(name);
			if (node == nullptr) {
				Log::warn("Can't find node with name '%s' for vxl node %u", name.c_str(), vxlNodeId);
				continue;
			}
			// hva transforms are overriding the vxl transform
			scenegraph::SceneGraphKeyFrame &kf = node->keyFrame(keyFrameIdx);
			kf.frameIdx = keyFrameIdx * 6; // running at 6 fps

			const int nodeId = file.header.layerIds[vxlNodeId];
			if (nodeId != InvalidNodeId) {
				glm::mat4 glmMatrix = sectionMatrices[vxlNodeId].toVengi();
				convertRead(glmMatrix, mdl.layerInfos[nodeId], true);

				scenegraph::SceneGraphTransform transform;
				transform.setLocalMatrix(glmMatrix);
				kf.setTransform(transform);
			}
		}
	}
	return true;
}

bool VXLFormat::writeHVAHeader(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph) const {
	char name[16];
	core_memset(name, 0, sizeof(name));
	// TODO: name
	if (stream.write(name, sizeof(name)) == -1) {
		Log::error("Failed to write hva header name");
		return false;
	}
	uint32_t numFrames = 0;

	for (auto iter = sceneGraph.beginAllModels(); iter != sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		numFrames = core_max(numFrames, node.keyFrames().size());
	}

	stream.writeUInt32(numFrames);
	uint32_t numNodes = sceneGraph.size();
	stream.writeUInt32(numNodes);
	for (uint32_t frame = 0; frame < numFrames; ++frame) {
		for (auto iter = sceneGraph.beginAllModels(); iter != sceneGraph.end(); ++iter) {
			const scenegraph::SceneGraphNode &node = *iter;
			const core::String &name = node.name().substr(0, 15);
			if (stream.write(name.c_str(), name.size()) == -1) {
				Log::error("Failed to write layer name");
				return false;
			}
			for (size_t i = 0; i < 16 - name.size(); ++i) {
				wrapBool(stream.writeUInt8(0))
			}
		}
	}
	return true;
}

bool VXLFormat::writeHVAFrames(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph) const {
	uint32_t numFrames = 0;
	for (auto iter = sceneGraph.beginAllModels(); iter != sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		numFrames = core_max(numFrames, node.keyFrames().size());
	}

	for (uint32_t i = 0; i < numFrames; ++i) {
		for (auto iter = sceneGraph.beginAllModels(); iter != sceneGraph.end(); ++iter) {
			scenegraph::SceneGraphNode &node = *iter;
			const scenegraph::SceneGraphTransform &transform = node.transform(i);
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

bool VXLFormat::saveHVA(const core::String &filename, const scenegraph::SceneGraph &sceneGraph) {
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

bool VXLFormat::loadFromFile(const core::String &filename, scenegraph::SceneGraph &sceneGraph, voxel::Palette &palette,
							 const LoadContext &ctx) {
	const io::FilePtr &file = io::filesystem()->open(filename);
	if (file && file->validHandle()) {
		io::FileStream stream(file);
		return loadGroupsPalette(filename, stream, sceneGraph, palette, ctx);
	}
	return true;
}

size_t VXLFormat::loadPalette(const core::String &filename, io::SeekableReadStream &stream, voxel::Palette &palette,
							  const LoadContext &ctx) {
	VXLModel mdl;
	if (!readHeader(stream, mdl, palette)) {
		return false;
	}
	return palette.colorCount();
}

bool VXLFormat::loadGroupsPalette(const core::String &filename, io::SeekableReadStream &stream,
								  scenegraph::SceneGraph &sceneGraph, voxel::Palette &palette, const LoadContext &ctx) {
	VXLModel mdl;

	wrapBool(readHeader(stream, mdl, palette))
	wrapBool(prepareModel(mdl))

	wrapBool(readLayerHeaders(stream, mdl))
	const int64_t bodyPos = stream.pos();
	if (stream.skip(mdl.header.dataSize) == -1) {
		Log::error("Failed to skip %u bytes", mdl.header.dataSize);
		return false;
	}
	wrapBool(readLayerInfos(stream, mdl))

	if (stream.seek(bodyPos) == -1) {
		Log::error("Failed to seek");
		return false;
	}
	wrapBool(readLayers(stream, mdl, sceneGraph, palette))

	const core::String &basename = core::string::stripExtension(filename);
	wrapBool(loadHVA(basename + ".hva", mdl, sceneGraph))

	if (!core::string::endsWith(filename, "barl.vxl")) {
		wrapBool(loadFromFile(basename + "barl.vxl", sceneGraph, palette, ctx));
	}
	if (!core::string::endsWith(filename, "tur.vxl")) {
		wrapBool(loadFromFile(basename + "tur.vxl", sceneGraph, palette, ctx));
	}

	return true;
}

#undef wrap
#undef wrapBool

} // namespace voxelformat
