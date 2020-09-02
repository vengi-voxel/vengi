/**
 * @file
 */

#include "KVXFormat.h"
#include "voxel/MaterialColor.h"
#include "io/FileStream.h"
#include "core/StringUtil.h"
#include "core/Log.h"
#include "core/Color.h"
#include <glm/common.hpp>

namespace voxel {

#define wrap(read) \
	if (read != 0) { \
		Log::error("Could not load kvx file: Not enough data in stream " CORE_STRINGIFY(read) " - still %i bytes left", (int)stream.remaining()); \
		return false; \
	}

bool KVXFormat::loadGroups(const io::FilePtr& file, VoxelVolumes& volumes) {
	if (!(bool)file || !file->exists()) {
		Log::error("Could not load kvx file: File doesn't exist");
		return false;
	}
	io::FileStream stream(file.get());

	// Total # of bytes (not including numbytes) in each mip-map level
	// but there is only 1 mip-map level
	uint32_t numbytes;
	wrap(stream.readInt(numbytes))

	// Dimensions of voxel. (our depth is kvx height)
	uint32_t xsiz, ysiz, zsiz;
	wrap(stream.readInt(xsiz))
	wrap(stream.readInt(ysiz))
	wrap(stream.readInt(zsiz))

	if (xsiz > 256 || ysiz > 256 || zsiz > 255) {
		Log::error("Dimensions exceeded: w: %i, h: %i, d: %i", xsiz, zsiz, ysiz);
		return false;
	}

	/**
	 * Centroid of voxel. For extra precision, this location has been shifted up by 8 bits.
	 */
	glm::vec3 pivot(0.0f);
	uint32_t pivx, pivy, pivz;
	wrap(stream.readInt(pivx))
	wrap(stream.readInt(pivy))
	wrap(stream.readInt(pivz))
	pivot.x = (float)pivx / 256.0f;
	pivot.y = (float)pivy / 256.0f;
	pivot.z = (float)pivz / 256.0f;

	if (xsiz > MaxRegionSize || ysiz > MaxRegionSize || zsiz > MaxRegionSize) {
		Log::error("Volume exceeds the max allowed size: %i:%i:%i", xsiz, zsiz, ysiz);
		return false;
	}

	const voxel::Region region(0, 0, 0, xsiz - 1, zsiz - 1, ysiz - 1);
	if (!region.isValid()) {
		Log::error("Invalid region: %i:%i:%i", xsiz, zsiz, ysiz);
		return false;
	}

	/**
	 * For compression purposes, I store the column pointers
	 * in a way that offers quick access to the data, but with slightly more
	 * overhead in calculating the positions.  See example of usage in voxdata.
	 * NOTE: xoffset[0] = (xsiz+1)*4 + xsiz*(ysiz+1)*2 (ALWAYS)
	 */
	uint16_t xyoffset[256][257];
	uint32_t xoffset;
	wrap(stream.readInt(xoffset))

	core_assert(((xsiz + 1) << 2) == sizeof(uint32_t) * (xsiz + 1));
	stream.skip(sizeof(uint32_t) * xsiz);
	for (uint32_t x = 0u; x < xsiz; ++x) {
		for (uint32_t y = 0u; y <= ysiz; ++y) {
			wrap(stream.readShort(xyoffset[x][y]))
		}
	}

	if (xoffset != (xsiz + 1) * 4 + xsiz * (ysiz + 1) * 2) {
		Log::error("Invalid offset values found");
		return false;
	}
	// Read the color palette from the end of the file and convert to our palette
	const size_t currentPos = stream.pos();
	_paletteSize = _palette.size();
	stream.seek(stream.size() - 3 * _paletteSize);

	/**
	 * The last 768 bytes of the KVX file is a standard 256-color VGA palette.
	 * The palette is in (Red:0, Green:1, Blue:2) order and intensities range
	 * from 0-63.
	 */
	for (size_t i = 0; i < _paletteSize; ++i) {
		uint8_t r, g, b;
		wrap(stream.readByte(r))
		wrap(stream.readByte(g))
		wrap(stream.readByte(b))

		const uint8_t nr = glm::clamp((uint32_t)glm::round((r * 255) / 63.0f), 0u, 255u);
		const uint8_t ng = glm::clamp((uint32_t)glm::round((g * 255) / 63.0f), 0u, 255u);
		const uint8_t nb = glm::clamp((uint32_t)glm::round((b * 255) / 63.0f), 0u, 255u);

		const glm::vec4& color = core::Color::fromRGBA(nr, ng, nb, 255);
		_palette[i] = findClosestIndex(color);
	}
	stream.seek(currentPos);

	RawVolume *volume = new RawVolume(region);
	volumes.push_back(VoxelVolume{volume, file->fileName(), true});

	/**
	 * voxdata: stored in sequential format.  Here's how you can get pointers to
	 * the start and end of any (x, y) column:
	 *
	 * @code
	 * //pointer to start of slabs on column (x, y):
	 * startptr = &voxdata[xoffset[x] + xyoffset[x][y]];
	 *
	 * //pointer to end of slabs on column (x, y):
	 * endptr = &voxdata[xoffset[x] + xyoffset[x][y+1]];
	 * @endcode
	 *
	 * Note: endptr is actually the first piece of data in the next column
	 *
	 * Once you get these pointers, you can run through all of the "slabs" in
	 * the column. Each slab has 3 bytes of header, then an array of colors.
	 * Here's the format:
	 *
	 * @code
	 * char slabztop;             //Starting z coordinate of top of slab
	 * char slabzleng;            //# of bytes in the color array - slab height
	 * char slabbackfacecullinfo; //Low 6 bits tell which of 6 faces are exposed
	 * char col[slabzleng];       //The array of colors from top to bottom
	 * @endcode
	 */

	struct slab {
		uint8_t slabztop;
		uint8_t slabzleng;
		uint8_t slabbackfacecullinfo;
	};

	uint32_t lastZ = 0;
	voxel::Voxel lastCol;

	for (uint32_t x = 0; x < xsiz; ++x) {
		for (uint32_t y = 0; y < ysiz; ++y) {
			const uint16_t end = xyoffset[x][y + 1];
			const uint16_t start = xyoffset[x][y];
			int32_t n = end - start;

			while (n > 0) {
				slab header;
				wrap(stream.readByte(header.slabztop))
				wrap(stream.readByte(header.slabzleng))
				wrap(stream.readByte(header.slabbackfacecullinfo))
				for (uint8_t i = 0u; i < header.slabzleng; ++i) {
					uint8_t col;
					wrap(stream.readByte(col))
					lastCol = voxel::createVoxel(voxel::VoxelType::Generic, convertPaletteIndex(col));
					volume->setVoxel(x, (zsiz - 1) - (header.slabztop + i), y, lastCol);
				}

				/**
				 * the format only saves the visible voxels - we have to take the face info to
				 * fill the inner voxels
				 */
				if (!(header.slabbackfacecullinfo & (1 << 4))) {
					for (int i = lastZ + 1; i < header.slabztop; ++i) {
						volume->setVoxel(x, (zsiz - 1) - i, y, lastCol);
					}
				}
				if (!(header.slabbackfacecullinfo & (1 << 5))) {
					lastZ = header.slabztop + header.slabzleng;
				}
				n -= (header.slabzleng + sizeof(header));
			}
		}
	}

	return true;
}

#undef wrap

bool KVXFormat::saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) {
	return false;
}

}
