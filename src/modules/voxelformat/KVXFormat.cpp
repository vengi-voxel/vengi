/**
 * @file
 */

#include "KVXFormat.h"
#include "io/Stream.h"
#include "voxel/MaterialColor.h"
#include "io/FileStream.h"
#include "core/StringUtil.h"
#include "core/Log.h"
#include "core/Color.h"
#include "voxel/PaletteLookup.h"
#include "voxel/Palette.h"
#include <glm/common.hpp>

namespace voxelformat {

#define wrap(read) \
	if ((read) != 0) { \
		Log::error("Could not load kvx file: Not enough data in stream " CORE_STRINGIFY(read)); \
		return false; \
	}

bool KVXFormat::loadGroupsPalette(const core::String &filename, io::SeekableReadStream& stream, SceneGraph &sceneGraph, voxel::Palette &palette) {
	// Total # of bytes (not including numbytes) in each mip-map level
	// but there is only 1 mip-map level
	uint32_t numbytes;
	wrap(stream.readUInt32(numbytes))

	// Dimensions of voxel. (our depth is kvx height)
	uint32_t xsiz, ysiz, zsiz;
	wrap(stream.readUInt32(xsiz))
	wrap(stream.readUInt32(ysiz))
	wrap(stream.readUInt32(zsiz))

	if (xsiz > 256 || ysiz > 256 || zsiz > 255) {
		Log::error("Dimensions exceeded: w: %i, h: %i, d: %i", xsiz, zsiz, ysiz);
		return false;
	}

	if (xsiz > MaxRegionSize || ysiz > MaxRegionSize || zsiz > MaxRegionSize) {
		Log::error("Volume exceeds the max allowed size: %i:%i:%i", xsiz, zsiz, ysiz);
		return false;
	}

	const voxel::Region region(0, 0, 0, (int)xsiz - 1, (int)zsiz - 1, (int)ysiz - 1);
	if (!region.isValid()) {
		Log::error("Invalid region: %i:%i:%i", xsiz, zsiz, ysiz);
		return false;
	}

	/**
	 * Centroid of voxel. For extra precision, this location has been shifted up by 8 bits.
	 */
	SceneGraphTransform transform;
	uint32_t pivx, pivy, pivz;
	wrap(stream.readUInt32(pivx))
	wrap(stream.readUInt32(pivy))
	wrap(stream.readUInt32(pivz))

#if 0
	glm::vec3 normalizedPivot;
	normalizedPivot.x = (float)pivx / 256.0f;
	normalizedPivot.y = (float)pivy / 256.0f;
	normalizedPivot.z = (float)pivz / 256.0f;
	transform.setPivot(normalizedPivot);
#endif

	/**
	 * For compression purposes, I store the column pointers
	 * in a way that offers quick access to the data, but with slightly more
	 * overhead in calculating the positions.  See example of usage in voxdata.
	 * NOTE: xoffset[0] = (xsiz+1)*4 + xsiz*(ysiz+1)*2 (ALWAYS)
	 */
	uint16_t xyoffset[256][257];
	uint32_t xoffset;
	wrap(stream.readUInt32(xoffset))

	core_assert(((xsiz + 1) << 2) == sizeof(uint32_t) * (xsiz + 1));
	stream.skip((int64_t)sizeof(uint32_t) * xsiz);
	for (uint32_t x = 0u; x < xsiz; ++x) {
		for (uint32_t y = 0u; y <= ysiz; ++y) {
			wrap(stream.readUInt16(xyoffset[x][y]))
		}
	}

	if (xoffset != (xsiz + 1) * 4 + xsiz * (ysiz + 1) * 2) {
		Log::error("Invalid offset values found");
		return false;
	}
	// Read the color palette from the end of the file and convert to our palette
	const size_t currentPos = stream.pos();
	palette.colorCount = voxel::PaletteMaxColors;
	stream.seek(stream.size() - 3 * palette.colorCount);

	/**
	 * The last 768 bytes of the KVX file is a standard 256-color VGA palette.
	 * The palette is in (Red:0, Green:1, Blue:2) order and intensities range
	 * from 0-63.
	 */
	for (int i = 0; i < palette.colorCount; ++i) {
		uint8_t r, g, b;
		wrap(stream.readUInt8(r))
		wrap(stream.readUInt8(g))
		wrap(stream.readUInt8(b))

		const uint8_t nr = glm::clamp((uint32_t)glm::round(((float)r * 255.0f) / 63.0f), 0u, 255u);
		const uint8_t ng = glm::clamp((uint32_t)glm::round(((float)g * 255.0f) / 63.0f), 0u, 255u);
		const uint8_t nb = glm::clamp((uint32_t)glm::round(((float)b * 255.0f) / 63.0f), 0u, 255u);

		const glm::vec4& color = core::Color::fromRGBA(nr, ng, nb, 255);
		palette.colors[i] = core::Color::getRGBA(color);
	}
	stream.seek((int64_t)currentPos);

	voxel::RawVolume *volume = new voxel::RawVolume(region);
	SceneGraphNode node;
	node.setVolume(volume, true);
	node.setName(filename);
	node.setTransform(0, transform, true);
	node.setPalette(palette);
	sceneGraph.emplace(core::move(node));

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
				wrap(stream.readUInt8(header.slabztop))
				wrap(stream.readUInt8(header.slabzleng))
				wrap(stream.readUInt8(header.slabbackfacecullinfo))
				for (uint8_t i = 0u; i < header.slabzleng; ++i) {
					uint8_t col;
					wrap(stream.readUInt8(col))
					lastCol = voxel::createVoxel(voxel::VoxelType::Generic, col);
					volume->setVoxel((int)x, (int)((zsiz - 1) - (header.slabztop + i)), (int)y, lastCol);
				}

				/**
				 * the format only saves the visible voxels - we have to take the face info to
				 * fill the inner voxels
				 */
				if (!(header.slabbackfacecullinfo & (1 << 4))) {
					for (uint32_t i = lastZ + 1; i < header.slabztop; ++i) {
						volume->setVoxel((int)x, (int)((zsiz - 1) - i), (int)y, lastCol);
					}
				}
				if (!(header.slabbackfacecullinfo & (1 << 5))) {
					lastZ = header.slabztop + header.slabzleng;
				}
				n -= (int32_t)(header.slabzleng + sizeof(header));
			}
		}
	}

	return true;
}

#undef wrap

bool KVXFormat::saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) {
	return false;
}

}
