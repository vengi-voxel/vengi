/**
 * @file
 */

#include "VXLFormat.h"
#include "core/Common.h"
#include "core/Color.h"
#include "core/Assert.h"
#include "core/StandardLib.h"
#include "io/FileStream.h"
#include "core/Log.h"
#include "io/Stream.h"
#include "voxel/MaterialColor.h"
#include "core/collection/Buffer.h"
#include "voxelformat/SceneGraphNode.h"
#include <SDL_assert.h>

namespace voxel {

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

bool VXLFormat::writeLimbBodyEntry(io::SeekableWriteStream& stream, const voxel::RawVolume* volume, uint8_t x, uint8_t y, uint8_t z, uint32_t& skipCount, uint32_t& voxelCount) const {
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

bool VXLFormat::writeLimb(io::SeekableWriteStream& stream, const SceneGraph& sceneGraph, uint32_t limbIdx, LimbOffset& offsets, uint64_t limbSectionOffset) const {
	const SceneGraphNode* node = sceneGraph[limbIdx];
	core_assert_always(node != nullptr);
	const voxel::Region& region = node->region();
	const glm::ivec3& size = region.getDimensionsInVoxels();

	const uint32_t baseSize = size.x * size.z;
	const int64_t globalSpanStartPos = stream.pos();
	Log::debug("size.x: %i, size.z: %i, globalSpanStartPos: %u", size.x, size.z, (uint32_t)globalSpanStartPos);

	offsets.start = stream.pos() - limbSectionOffset;
	const size_t limbOffset = HeaderSize + LimbHeaderSize * sceneGraph.size() + offsets.start;
	Log::debug("limbOffset(%u): %u", limbIdx, (uint32_t)limbOffset);

	for (uint32_t i = 0; i < baseSize; i++) {
		wrapBool(stream.writeUInt32(-1))
	}
	offsets.end = stream.pos() - limbSectionOffset;
	for (uint32_t i = 0; i < baseSize; i++) {
		wrapBool(stream.writeUInt32(-1))
	}
	offsets.data = stream.pos() - limbSectionOffset;

	const int64_t beforePos = stream.pos();
	for (uint32_t i = 0u; i < baseSize; ++i) {
		const int32_t spanStartPos = stream.pos() - beforePos;

		const uint8_t x = (uint8_t)(i % size.x);
		const uint8_t z = (uint8_t)(i / size.x);

		uint32_t skipCount = 0u;
		uint32_t voxelCount = 0u;
		bool voxelsInColumn = false;
		for (uint8_t y = 0; y <= size.y; ++y) {
			const voxel::Voxel& voxel = node->volume()->voxel(x, y, z);
			if (voxel::isAir(voxel.getMaterial())) {
				if (voxelCount > 0) {
					wrapBool(writeLimbBodyEntry(stream, node->volume(), x, y, z, skipCount, voxelCount))
					voxelsInColumn = true;
				}
				++skipCount;
			} else {
				if (skipCount > 0) {
					wrapBool(writeLimbBodyEntry(stream, node->volume(), x, y, z, skipCount, voxelCount))
					voxelsInColumn = true;
				}
				++voxelCount;
			}
		}
		if (voxelCount > 0 || skipCount > 0) {
			wrapBool(writeLimbBodyEntry(stream, node->volume(), x, size.y - 1, z, skipCount, voxelCount))
			voxelsInColumn = true;
		}
		if (!voxelsInColumn) {
			continue;
		}

		const int64_t spanEndPos = stream.pos();
		wrap(stream.seek(globalSpanStartPos + i * sizeof(uint32_t)))
		wrapBool(stream.writeUInt32(spanStartPos))

		wrap(stream.seek(globalSpanStartPos + baseSize * sizeof(uint32_t) + i * sizeof(uint32_t)))
		wrapBool(stream.writeUInt32(spanEndPos - globalSpanStartPos))
		wrap(stream.seek(spanEndPos))
	}

	return true;
}

bool VXLFormat::writeLimbHeader(io::SeekableWriteStream& stream, const SceneGraph& sceneGraph, uint32_t limbIdx) const {
	core_assert((uint64_t)stream.pos() == (uint64_t)(HeaderSize + limbIdx * LimbHeaderSize));
	const SceneGraphNode* node = sceneGraph[limbIdx];
	core_assert_always(node != nullptr);
	char name[15];
	core_memcpy(name, node->name().c_str(), sizeof(name));
	if (stream.write(name, sizeof(name)) == -1) {
		Log::error("Failed to write limp header into stream");
		return false;
	}
	wrapBool(stream.writeUInt8('\0'))
	wrapBool(stream.writeUInt32(limbIdx))
	wrapBool(stream.writeUInt32(1))
	wrapBool(stream.writeUInt32(0))
	return true;
}

bool VXLFormat::writeLimbFooter(io::SeekableWriteStream& stream, const SceneGraph& sceneGraph, uint32_t limbIdx, const LimbOffset& offsets) const {
	const SceneGraphNode* node = sceneGraph[limbIdx];
	core_assert_always(node != nullptr);
	wrapBool(stream.writeUInt32(offsets.start))
	wrapBool(stream.writeUInt32(offsets.end))
	wrapBool(stream.writeUInt32(offsets.data))
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			wrapBool(stream.writeFloat(0.0f)) // TODO: region.getLowerCorner
		}
	}
	for (int i = 0; i < 3; ++i) {
		wrapBool(stream.writeFloat(1.0f))
	}
	const voxel::Region& region = node->region();
	const glm::ivec3& size = region.getDimensionsInVoxels();
	wrapBool(stream.writeUInt8(size.x))
	wrapBool(stream.writeUInt8(size.z))
	wrapBool(stream.writeUInt8(size.y))
	wrapBool(stream.writeUInt8(2))
	return true;
}

bool VXLFormat::writeHeader(io::SeekableWriteStream& stream, const SceneGraph& sceneGraph) {
	wrapBool(stream.writeString("Voxel Animation"))
	wrapBool(stream.writeUInt32(1))
	wrapBool(stream.writeUInt32(sceneGraph.size()))
	wrapBool(stream.writeUInt32(sceneGraph.size()))
	wrapBool(stream.writeUInt32(0)) // bodysize is filled later
	wrapBool(stream.writeUInt16(0x1f10U))
	const MaterialColorArray& materialColors = getMaterialColors();
	const uint32_t paletteSize = materialColors.size();
	for (uint32_t i = 0; i < paletteSize; ++i) {
		const glm::u8vec4& rgba = core::Color::getRGBAVec(materialColors[i]);
		wrapBool(stream.writeUInt8(rgba[0]))
		wrapBool(stream.writeUInt8(rgba[1]))
		wrapBool(stream.writeUInt8(rgba[2]))
	}
	core_assert(stream.pos() == HeaderSize);
	return true;
}

bool VXLFormat::saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) {
	wrapBool(writeHeader(stream, sceneGraph))
	for (uint32_t i = 0; i < sceneGraph.size(); ++i) {
		wrapBool(writeLimbHeader(stream, sceneGraph, i))
	}

	core::Buffer<LimbOffset> limbOffsets(sceneGraph.size());
	const uint64_t afterHeaderPos = stream.pos();
	for (uint32_t i = 0; i < sceneGraph.size(); ++i) {
		wrapBool(writeLimb(stream, sceneGraph, i, limbOffsets[i], afterHeaderPos))
	}

	const uint64_t afterBodyPos = stream.pos();
	const uint64_t bodySize = afterBodyPos - afterHeaderPos;
	wrap(stream.seek(HeaderBodySizeOffset));
	wrapBool(stream.writeUInt32(bodySize))
	wrap(stream.seek(afterBodyPos));

	core_assert((uint64_t)stream.pos() == (uint64_t)(HeaderSize + LimbHeaderSize * sceneGraph.size() + bodySize));

	for (uint32_t i = 0; i < sceneGraph.size(); ++i) {
		wrapBool(writeLimbFooter(stream, sceneGraph, i, limbOffsets[i]))
	}
	return true;
}

bool VXLFormat::readLimb(io::SeekableReadStream& stream, vxl_mdl& mdl, uint32_t limbIdx, SceneGraph& sceneGraph) const {
	const vxl_limb_tailer &footer = mdl.limb_tailers[limbIdx];
	const vxl_limb_header &header = mdl.limb_headers[limbIdx];

	const uint32_t baseSize = footer.xsize * footer.ysize;
	core::Buffer<int32_t> colStart(baseSize);

	// switch axis
	RawVolume *volume = new RawVolume(Region{0, 0, 0, footer.xsize - 1, footer.zsize - 1, footer.ysize - 1});
	voxel::SceneGraphNode node;
	node.setVolume(volume, true);
	node.setName(header.limb_name);
	sceneGraph.emplace(core::move(node));
	++mdl.volumeIdx;

	const size_t limbOffset = HeaderSize + LimbHeaderSize * mdl.header.n_limbs + footer.span_start_off;
	wrap(stream.seek(limbOffset))
	Log::debug("limbOffset: %u", (uint32_t)limbOffset);
	for (uint32_t i = 0; i < baseSize; ++i) {
		uint32_t v;
		wrap(stream.readUInt32(v))
		colStart[i] = v;
	}
	// skip spanPosEnd values
	stream.skip(sizeof(uint32_t) * baseSize);

	const uint64_t dataStart = stream.pos();

	// Count the voxels in this limb
	for (uint32_t i = 0u; i < baseSize; ++i) {
		if (colStart[i] == EmptyColumn) {
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

	for (uint32_t i = 0u; i < baseSize; ++i) {
		if (colStart[i] == EmptyColumn) {
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
				const uint8_t palIdx = convertPaletteIndex(color);
				const voxel::Voxel v = voxel::createVoxel(voxel::VoxelType::Generic, palIdx);
				volume->setVoxel(x, z, y, v);
				++z;
			}

			// Skip duplicate count
			stream.skip(1);
		} while (z < footer.zsize);
	}
	return true;
}

bool VXLFormat::readLimbs(io::SeekableReadStream& stream, vxl_mdl& mdl, SceneGraph& sceneGraph) const {
	const vxl_header& hdr = mdl.header;
	sceneGraph.reserve(hdr.n_limbs);
	for (uint32_t i = 0; i < hdr.n_limbs; ++i) {
		wrapBool(readLimb(stream, mdl, i, sceneGraph))
	}
	return true;
}

bool VXLFormat::readLimbHeader(io::SeekableReadStream& stream, vxl_mdl& mdl, uint32_t limbIdx) const {
	vxl_limb_header &header = mdl.limb_headers[limbIdx];
	wrapBool(stream.readString(sizeof(header.limb_name), header.limb_name))
	Log::debug("Limb %u name: %s", limbIdx, header.limb_name);
	wrap(stream.readUInt32(header.unknown))
	wrap(stream.readUInt32(header.limb_number))
	wrap(stream.readUInt32(header.unknown))
	wrap(stream.readUInt32(header.unknown2))
	return true;
}

bool VXLFormat::readLimbHeaders(io::SeekableReadStream& stream, vxl_mdl& mdl) const {
	wrap(stream.seek(HeaderSize))
	for (uint32_t i = 0; i < mdl.header.n_limbs; ++i) {
		wrapBool(readLimbHeader(stream, mdl, i))
	}
	return true;
}

bool VXLFormat::readLimbFooter(io::SeekableReadStream& stream, vxl_mdl& mdl, uint32_t limbIdx) const {
	vxl_limb_tailer &footer = mdl.limb_tailers[limbIdx];
	wrap(stream.readUInt32(footer.span_start_off))
	wrap(stream.readUInt32(footer.span_end_off))
	wrap(stream.readUInt32(footer.span_data_off))
	Log::debug("offsets: %u:%u:%u", footer.span_start_off, footer.span_end_off, footer.span_data_off);
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			wrap(stream.readFloat(footer.transform[i][j]))
		}
	}
	for (int i = 0; i < 3; ++i) {
		wrap(stream.readFloat(footer.scale[i]))
	}
	Log::debug("scale: %f:%f:%f", footer.scale[0], footer.scale[1], footer.scale[2]);
	wrap(stream.readUInt8(footer.xsize))
	wrap(stream.readUInt8(footer.ysize))
	wrap(stream.readUInt8(footer.zsize))
	wrap(stream.readUInt8(footer.type))

	if (footer.xsize == 0 || footer.ysize == 0 || footer.zsize == 0) {
		Log::error("Invalid limb size found");
		return false;
	}

	Log::debug("size: %u:%u:%u, type: %u", footer.xsize, footer.ysize, footer.zsize, footer.type);
	return true;
}

bool VXLFormat::readLimbFooters(io::SeekableReadStream& stream, vxl_mdl& mdl) const {
	const vxl_header& hdr = mdl.header;
	wrap(stream.seek(HeaderSize + LimbHeaderSize * hdr.n_limbs + hdr.bodysize))
	for (uint32_t i = 0; i < hdr.n_limbs; ++i) {
		wrapBool(readLimbFooter(stream, mdl, i))
	}
	return true;
}

bool VXLFormat::readHeader(io::SeekableReadStream& stream, vxl_mdl& mdl) {
	vxl_header& hdr = mdl.header;
	wrapBool(stream.readString(sizeof(hdr.filetype), hdr.filetype))
	if (SDL_strcmp(hdr.filetype, "Voxel Animation") != 0) {
		Log::error("Invalid vxl header");
		return false;
	}
	wrap(stream.readUInt32(hdr.unknown))
	wrap(stream.readUInt32(hdr.n_limbs))
	wrap(stream.readUInt32(hdr.n_limbs2))
	wrap(stream.readUInt32(hdr.bodysize))
	wrap(stream.readUInt16(hdr.unknown2))

	Log::debug("Found %u limbs", hdr.n_limbs);

	_paletteSize = _palette.size();
	_colorsSize = _paletteSize;
	bool valid = false;
	for (uint32_t i = 0; i < _paletteSize; ++i) {
		wrap(stream.readUInt8(hdr.palette[i][0]))
		wrap(stream.readUInt8(hdr.palette[i][1]))
		wrap(stream.readUInt8(hdr.palette[i][2]))
		if (hdr.palette[i][0] != 0 || hdr.palette[i][1] != 0 || hdr.palette[i][2] != 0) {
			valid = true;
		}
	}

	if (valid) {
		// convert to our palette
		for (uint32_t i = 0; i < _paletteSize; ++i) {
			const uint8_t *p = hdr.palette[i];
			const glm::vec4& color = core::Color::fromRGBA(p[0], p[1], p[2], 0xffu);
			const int index = findClosestIndex(color);
			_colors[i] = core::Color::getRGBA(color);
			_palette[i] = index;
		}
	} else {
		_paletteSize = 0;
		_colorsSize = 0;
	}

	return true;
}

bool VXLFormat::prepareModel(vxl_mdl& mdl) const {
	const vxl_header& hdr = mdl.header;
	if (hdr.n_limbs > MaxLimbs) {
		Log::error("Limb size exceeded the max allowed value: %u", hdr.n_limbs);
		return false;
	}
	return true;
}

bool VXLFormat::loadGroups(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph) {
	vxl_mdl mdl;
	wrapBool(readHeader(stream, mdl))
	wrapBool(prepareModel(mdl))

	wrapBool(readLimbHeaders(stream, mdl))
	wrapBool(readLimbFooters(stream, mdl))

	wrapBool(readLimbs(stream, mdl, sceneGraph))

	return true;
}

#undef wrap
#undef wrapBool

}
