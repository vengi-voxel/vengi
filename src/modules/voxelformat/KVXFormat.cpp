/**
 * @file
 */

#include "KVXFormat.h"
#include "voxel/MaterialColor.h"
#include "core/StringUtil.h"
#include "core/Log.h"
#include "core/Color.h"
#include <glm/common.hpp>

namespace voxel {

#define wrap(read) \
	if (read != 0) { \
		Log::error("Could not load kvx file: Not enough data in stream " CORE_STRINGIFY(read) " - still %i bytes left", (int)stream.remaining()); \
		delete[] xyoffset; \
		delete[] xoffset; \
		return false; \
	}

bool KVXFormat::loadGroups(const io::FilePtr& file, VoxelVolumes& volumes) {
	if (!(bool)file || !file->exists()) {
		Log::error("Could not load kvx file: File doesn't exist");
		return false;
	}
	io::FileStream stream(file.get());

	/**
	 * For compression purposes, I store the column pointers
	 * in a way that offers quick access to the data, but with slightly more
	 * overhead in calculating the positions.  See example of usage in voxdata.
	 * NOTE: xoffset[0] = (xsiz+1)*4 + xsiz*(ysiz+1)*2 (ALWAYS)
	 */
	uint32_t* xoffset = nullptr;
	uint16_t* xyoffset = nullptr;

	// Total # of bytes (not including numbytes) in each mip-map level
	// but there is only 1 mip-map level
	uint32_t numbytes;
	wrap(stream.readInt(numbytes))

	// Dimensions of voxel. (our depth is kvx height)
	uint32_t width, height, depth;
	wrap(stream.readInt(width))
	wrap(stream.readInt(height))
	wrap(stream.readInt(depth))

	if (width > 256 || height > 255 || depth > 256) {
		Log::error("Dimensions exceeded: w: %i, h: %i, d: %i", width, height, depth);
		return false;
	}

	/**
	 * Centroid of voxel. For extra precision, this location has been shifted up by 8 bits.
	 */
	glm::vec3 pivot;
	wrap(stream.readFloat(pivot.x))
	wrap(stream.readFloat(pivot.y))
	wrap(stream.readFloat(pivot.z))
	pivot /= 256.0f;

	if (width > MaxRegionSize || height > MaxRegionSize || depth > MaxRegionSize) {
		Log::error("Volume exceeds the max allowed size: %i:%i:%i", width, height, depth);
		return false;
	}

	const voxel::Region region(0, 0, 0, width - 1, depth - 1, height - 1);
	if (!region.isValid()) {
		Log::error("Invalid region: %i:%i:%i", width, depth, height);
		return false;
	}

	xoffset = new uint32_t[width + 1];
	xyoffset = new uint16_t[width * (height + 1)];

	for (uint32_t i = 0u; i < width + 1; ++i) {
		wrap(stream.readInt(xoffset[i]))
	}
	for (uint32_t x = 0u; x < width; ++x) {
		for (uint32_t y = 0u; y <= height; ++y) {
			wrap(stream.readShort(xyoffset[x * width + y]))
		}
	}

	if (xoffset[0] != (width + 1) * 4 + width * (height + 1) * 2) {
		delete[] xyoffset;
		delete[] xoffset;
		return false;
	}
	// Read the color palette from the end of the file and convert to our palette
	const size_t currentPos = stream.pos();
	_paletteSize = 256;
	stream.seek(stream.size() - 3 * _paletteSize);
	_palette.resize(_paletteSize);
	const MaterialColorArray& materialColors = getMaterialColors();

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

		const int sr = glm::round((r * 255) / 63.0f);
		const int sg = glm::round((g * 255) / 63.0f);
		const int sb = glm::round((b * 255) / 63.0f);
		const uint8_t nr = glm::clamp(sr, 0, 255);
		const uint8_t ng = glm::clamp(sg, 0, 255);
		const uint8_t nb = glm::clamp(sb, 0, 255);

		const glm::vec4& color = core::Color::fromRGBA(nr, ng, nb, 255);
		const int index = core::Color::getClosestMatch(color, materialColors);
		_palette[i] = index;
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
	uint32_t lastZ = 0;
	voxel::Voxel lastCol;
	for (uint32_t x = 0; x < width; x++) {
		for (uint32_t y = 0; y < height; y++) {
			uint16_t start = xyoffset[x * width + y];
			const uint16_t end = xyoffset[x * width + y + 1];

			while (start < end) {
				uint8_t zpos;
				uint8_t zlen;
				uint8_t visfaces;
				wrap(stream.readByte(zpos))
				wrap(stream.readByte(zlen))
				wrap(stream.readByte(visfaces))
				for (uint8_t i = 0u; i < zlen; ++i) {
					uint8_t index;
					wrap(stream.readByte(index))
					lastCol = voxel::createVoxel(voxel::VoxelType::Generic, convertPaletteIndex(index));
					volume->setVoxel(x - pivot.x, region.getHeightInCells() - (zpos + i - pivot.z), y - pivot.y, lastCol);
				}

				/**
				 * the format only saves the visible voxels - we have to take the face info to
				 * fill the inner voxels
				 */
				if (!(visfaces & (1 << 4))) {
					for (int i = lastZ + 1; i < zpos; i++) {
						volume->setVoxel(x - pivot.x, region.getHeightInCells() - (i - pivot.z), y - pivot.y, lastCol);
					}
				}
				if (!(visfaces & (1 << 5))) {
					lastZ = zpos + zlen - 1;
				}
				start += zlen + 3;
			}
		}
	}

	delete[] xyoffset;
	delete[] xoffset;

	return true;
}

#undef wrap

bool KVXFormat::saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) {
	return false;
}

}
