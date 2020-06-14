/**
 * @file
 */

#include "VXLFormat.h"
#include "core/Common.h"
#include "core/Color.h"
#include "core/Assert.h"
#include "core/io/FileStream.h"
#include "core/Log.h"
#include "voxel/MaterialColor.h"
#include "core/collection/Buffer.h"
#include <SDL_assert.h>

namespace voxel {

#define wrap(read) \
	if (read != 0) { \
		Log::debug("Error: " CORE_STRINGIFY(read) " at " SDL_FILE ":%i", SDL_LINE); \
		return false; \
	}

#define wrapBool(read) \
	if (!(read)) { \
		Log::debug("Error: " CORE_STRINGIFY(read) " at " SDL_FILE ":%i", SDL_LINE); \
		return false; \
	}

bool VXLFormat::writeLimb(io::FileStream& stream, const VoxelVolumes& volumes, uint32_t limbIdx) const {
	const VoxelVolume& v = volumes[limbIdx];
	const voxel::Region& region = v.volume->region();
	const glm::ivec3& size = region.getDimensionsInVoxels();

	const uint32_t baseSize = size.x * size.z;
	for (uint32_t i = 0; i < baseSize; i++) {
		wrapBool(stream.addInt(0)) // TODO: spanStart[i] and EmptyColumn handling
	}
	for (uint32_t i = 0; i < baseSize; i++) {
		wrapBool(stream.addInt(0)) // TODO: spanEnd[i] and EmptyColumn handling
	}
	for (uint32_t i = 0u; i < baseSize; ++i) {
		//if (spanStart[i] == EmptyColumn) {
		//	continue;
		//}

		// TODO: there might be multiple of this for regions that exceed the byte boundary
		uint32_t firstNonEmptyY = 0;
		uint32_t voxelAmountY = 0u;

		const uint8_t x = (uint8_t)(i % size.x);
		const uint8_t z = (uint8_t)(i / size.x);
		wrapBool(stream.addByte(firstNonEmptyY))
		wrapBool(stream.addByte(voxelAmountY))
		for (uint8_t y = 0; y < voxelAmountY; ++y) {
			const voxel::Voxel& voxel = v.volume->voxel(x, y, z);
			wrapBool(stream.addByte(voxel.getColor()))
			wrapBool(stream.addByte(0)) // TODO: normal
		}
	}
	// TODO: once finished, activate it
	return false;
}

bool VXLFormat::writeLimbHeader(io::FileStream& stream, const VoxelVolumes& volumes, uint32_t limbIdx) const {
	const VoxelVolume& v = volumes[limbIdx];
	wrapBool(stream.append((const uint8_t*)v.name.c_str(), 15))
	wrapBool(stream.addByte('\0'))
	wrapBool(stream.addInt(limbIdx))
	wrapBool(stream.addInt(1))
	wrapBool(stream.addInt(0))
	return true;
}

bool VXLFormat::writeLimbFooter(io::FileStream& stream, const VoxelVolumes& volumes, uint32_t limbIdx) const {
	const VoxelVolume& v = volumes[limbIdx];
	const uint32_t spanStartOff = stream.pos();
	wrapBool(stream.addInt(spanStartOff))
	wrapBool(stream.addInt(0)) // TODO: spanEndOff
	wrapBool(stream.addInt(0)) // TODO: spanDataOff
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			wrapBool(stream.addFloat(0.0f)) // TODO: region.getLowerCorner
		}
	}
	for (int i = 0; i < 3; ++i) {
		wrapBool(stream.addFloat(1.0f))
	}
	const voxel::Region& region = v.volume->region();
	const glm::ivec3& size = region.getDimensionsInVoxels();
	wrapBool(stream.addByte(size.x))
	wrapBool(stream.addByte(size.z))
	wrapBool(stream.addByte(size.y))
	wrapBool(stream.addByte(2))
	return true;
}

bool VXLFormat::writeHeader(io::FileStream& stream, const VoxelVolumes& volumes) {
	wrapBool(stream.addString("Voxel Animation"))
	wrapBool(stream.addInt(1))
	wrapBool(stream.addInt(volumes.size()))
	wrapBool(stream.addInt(volumes.size()))
	wrapBool(stream.addInt(0)) // bodysize is filled later
	wrapBool(stream.addShort(0x1f10U))
	const MaterialColorArray& materialColors = getMaterialColors();
	const uint32_t paletteSize = materialColors.size();
	for (uint32_t i = 0; i < paletteSize; ++i) {
		const glm::u8vec4& rgba = core::Color::getRGBAVec(materialColors[i]);
		wrapBool(stream.addByte(rgba[0]))
		wrapBool(stream.addByte(rgba[1]))
		wrapBool(stream.addByte(rgba[2]))
	}
	core_assert(stream.pos() == HeaderSize);
	return true;
}

bool VXLFormat::saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) {
	if (!(bool)file) {
		Log::error("Could not save vxl file: No file given");
		return false;
	}

	io::FileStream stream(file.get());

	wrap(writeHeader(stream, volumes))
	for (uint32_t i = 0; i < volumes.size(); ++i) {
		wrapBool(writeLimbHeader(stream, volumes, i))
	}
	const uint64_t afterHeaderPos = stream.pos();
	for (uint32_t i = 0; i < volumes.size(); ++i) {
		wrapBool(writeLimb(stream, volumes, i))
	}

	const uint64_t afterBodyPos = stream.pos();
	const uint64_t bodySize = afterBodyPos - afterHeaderPos;
	wrap(stream.seek(HeaderBodySizeOffset));
	wrapBool(stream.addInt(bodySize))
	wrap(stream.seek(afterBodyPos));

	for (uint32_t i = 0; i < volumes.size(); ++i) {
		wrapBool(writeLimbFooter(stream, volumes, i))
	}
	return true;
}

bool VXLFormat::readLimb(io::FileStream& stream, vxl_mdl& mdl, uint32_t limbIdx, VoxelVolumes& volumes) const {
	const vxl_limb_tailer &footer = mdl.limb_tailers[limbIdx];
	const vxl_limb_header &header = mdl.limb_headers[limbIdx];

	const uint32_t baseSize = footer.xsize * footer.ysize;
	core::Buffer<int32_t> colStart(baseSize);

	// switch axis
	RawVolume *volume = new RawVolume(Region{0, 0, 0, footer.xsize, footer.zsize, footer.ysize});
	volumes[mdl.volumeIdx].volume = volume;
	volumes[mdl.volumeIdx].name = header.limb_name;
	volumes[mdl.volumeIdx].pivot = volume->region().getCenter();
	++mdl.volumeIdx;

	wrap(stream.seek(HeaderSize + LimbHeaderSize * mdl.header.n_limbs + footer.span_start_off))
	for (uint32_t i = 0; i < baseSize; i++) {
		uint32_t v;
		wrap(stream.readInt(v))
		colStart[i] = *(int32_t*)&v;
	}
	stream.skip(4 * baseSize);
	const uint64_t dataStart = stream.pos();

	// Count the voxels in this limb
	uint32_t voxelCount = 0;
	for (uint32_t i = 0u; i < baseSize; ++i) {
		if (colStart[i] == EmptyColumn) {
			continue;
		}

		wrap(stream.seek(dataStart + colStart[i]))
		uint32_t z = 0;
		do {
			uint8_t v;
			wrap(stream.readByte(v))
			z += v;
			wrap(stream.readByte(v))
			z += v;
			voxelCount += v;
			stream.skip(2 * v + 1);
		} while (z < footer.zsize);
	}

	for (uint32_t i = 0u; i < baseSize; ++i) {
		if (colStart[i] == EmptyColumn) {
			continue;
		}

		wrap(stream.seek(dataStart + colStart[i]))

		uint8_t x = (uint8_t)(i % footer.xsize);
		uint8_t y = (uint8_t)(i / footer.xsize);
		uint8_t z = 0;
		do {
			uint8_t v;
			wrap(stream.readByte(v))
			z += v;
			uint8_t count;
			wrap(stream.readByte(count))
			for (uint8_t j = 0u; j < count; ++j) {
				uint8_t color;
				wrap(stream.readByte(color))
				uint8_t normal;
				wrap(stream.readByte(normal))
				const uint8_t palIdx = convertPaletteIndex(color);
				const voxel::Voxel v = voxel::createColorVoxel(voxel::VoxelType::Generic, palIdx);
				volume->setVoxel(x, z, y, v);
				++z;
			}

			// Skip duplicate count
			stream.skip(1);
		} while (z < footer.zsize);
	}
	return true;
}

bool VXLFormat::readLimbs(io::FileStream& stream, vxl_mdl& mdl, VoxelVolumes& volumes) const {
	const vxl_header& hdr = mdl.header;
	volumes.resize(hdr.n_limbs);
	for (uint32_t i = 0; i < hdr.n_limbs; ++i) {
		wrapBool(readLimb(stream, mdl, i, volumes))
	}
	return true;
}

bool VXLFormat::readLimbHeader(io::FileStream& stream, vxl_mdl& mdl, uint32_t limbIdx) const {
	vxl_limb_header &header = mdl.limb_headers[limbIdx];
	wrapBool(stream.readString(sizeof(header.limb_name), header.limb_name))
	Log::debug("Limb %u name: %s", limbIdx, header.limb_name);
	wrap(stream.readInt(header.unknown))
	wrap(stream.readInt(header.limb_number))
	wrap(stream.readInt(header.unknown))
	wrap(stream.readInt(header.unknown2))
	return true;
}

bool VXLFormat::readLimbHeaders(io::FileStream& stream, vxl_mdl& mdl) const {
	wrap(stream.seek(HeaderSize))
	for (uint32_t i = 0; i < mdl.header.n_limbs; ++i) {
		wrapBool(readLimbHeader(stream, mdl, i))
	}
	return true;
}

bool VXLFormat::readLimbFooter(io::FileStream& stream, vxl_mdl& mdl, uint32_t limbIdx) const {
	vxl_limb_tailer &footer = mdl.limb_tailers[limbIdx];
	wrap(stream.readInt(footer.span_start_off))
	wrap(stream.readInt(footer.span_end_off))
	wrap(stream.readInt(footer.span_data_off))
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			wrap(stream.readFloat(footer.transform[i][j]))
		}
	}
	for (int i = 0; i < 3; ++i) {
		wrap(stream.readFloat(footer.scale[i]))
	}
	wrap(stream.readByte(footer.xsize))
	wrap(stream.readByte(footer.ysize))
	wrap(stream.readByte(footer.zsize))
	wrap(stream.readByte(footer.type))
	return true;
}

bool VXLFormat::readLimbFooters(io::FileStream& stream, vxl_mdl& mdl) const {
	const vxl_header& hdr = mdl.header;
	wrap(stream.seek(HeaderSize + LimbHeaderSize * hdr.n_limbs + hdr.bodysize))
	for (uint32_t i = 0; i < hdr.n_limbs; ++i) {
		wrapBool(readLimbFooter(stream, mdl, i))
	}
	return true;
}

bool VXLFormat::readHeader(io::FileStream& stream, vxl_mdl& mdl) {
	vxl_header& hdr = mdl.header;
	wrapBool(stream.readString(sizeof(hdr.filetype), hdr.filetype))
	if (SDL_strcmp(hdr.filetype, "Voxel Animation")) {
		Log::error("Invalid vxl header");
		return false;
	}
	wrap(stream.readInt(hdr.unknown))
	wrap(stream.readInt(hdr.n_limbs))
	wrap(stream.readInt(hdr.n_limbs2))
	wrap(stream.readInt(hdr.bodysize))
	wrap(stream.readShort(hdr.unknown2))
	_paletteSize = 256;
	_palette.resize(_paletteSize);
	bool valid = false;
	for (uint32_t i = 0; i < _paletteSize; ++i) {
		wrap(stream.readByte(hdr.palette[i][0]))
		wrap(stream.readByte(hdr.palette[i][1]))
		wrap(stream.readByte(hdr.palette[i][2]))
		if (hdr.palette[i][0] != 0 || hdr.palette[i][1] != 0 || hdr.palette[i][2] != 0) {
			valid = true;
		}
	}

	if (valid) {
		// convert to our palette
		const MaterialColorArray& materialColors = getMaterialColors();
		for (uint32_t i = 0; i < _paletteSize; ++i) {
			const uint8_t *p = hdr.palette[i];
			const glm::vec4& color = core::Color::fromRGBA(p[0], p[1], p[2], 0xff);
			const int index = core::Color::getClosestMatch(color, materialColors);
			Log::info("Convert color %i:%i:%i to index: %i", p[0], p[1], p[2], index);
			_palette[i] = index;
		}
	} else {
		_paletteSize = 0;
	}

	return true;
}

bool VXLFormat::prepareModel(vxl_mdl& mdl) const {
	const vxl_header& hdr = mdl.header;
	if (hdr.n_limbs > 512) {
		Log::error("Limb size exceeded the max allowed value of 512: %u", hdr.n_limbs);
		return false;
	}
	mdl.limb_headers = new vxl_limb_header[hdr.n_limbs];
	mdl.limb_bodies = new vxl_limb_body[hdr.n_limbs];
	mdl.limb_tailers = new vxl_limb_tailer[hdr.n_limbs];
	return true;
}

bool VXLFormat::loadGroups(const io::FilePtr& file, VoxelVolumes& volumes) {
	if (!(bool)file || !file->exists()) {
		Log::error("Could not load vxl file: File doesn't exist");
		return false;
	}

	io::FileStream stream(file.get());

	vxl_mdl mdl;
	wrapBool(readHeader(stream, mdl))
	wrapBool(prepareModel(mdl))

	wrapBool(readLimbHeaders(stream, mdl))
	wrapBool(readLimbFooters(stream, mdl))

	wrapBool(readLimbs(stream, mdl, volumes))

	return true;
}

#undef wrap
#undef wrapBool

}
