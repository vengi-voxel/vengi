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
#include "scenegraph/SceneGraph.h"
#include <glm/common.hpp>

namespace voxelformat {

namespace priv {

#if 0
enum KVXVisibility {
	Left = 1,
	Right = 2,
	Front = 4,
	Back = 8,
	Up = 16,
	Down = 32
};

static uint8_t calculateVisibility(const voxel::RawVolume *v, int x, int y, int z) {
	uint8_t vis = 0;
	voxel::FaceBits visBits = voxel::visibleFaces(*v, x, y, z);
	if (visBits == voxel::FaceBits::None) {
		return vis;
	}
	// x
	if ((visBits & voxel::FaceBits::NegativeX) != voxel::FaceBits::None) {
		vis |= KVXVisibility::Left;
	}
	if ((visBits & voxel::FaceBits::PositiveX) != voxel::FaceBits::None) {
		vis |= KVXVisibility::Right;
	}
	// y (our z)
	if ((visBits & voxel::FaceBits::NegativeZ) != voxel::FaceBits::None) {
		vis |= KVXVisibility::Front;
	}
	if ((visBits & voxel::FaceBits::PositiveZ) != voxel::FaceBits::None) {
		vis |= KVXVisibility::Back;
	}
	// z (our y) is running from top to bottom
	if ((visBits & voxel::FaceBits::NegativeY) != voxel::FaceBits::None) {
		vis |= KVXVisibility::Up;
	}
	if ((visBits & voxel::FaceBits::PositiveY) != voxel::FaceBits::None) {
		vis |= KVXVisibility::Down;
	}
	return vis;
}
#endif

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
	// int x_low_w;
	// int y_low_d;
	// int z_low_h;
	// uint8_t col;
	uint8_t slabztop;
	uint8_t slabzleng;
	uint8_t slabbackfacecullinfo;
	// followed by array of colors
};

} // namespace priv

#define wrap(read) \
	if ((read) != 0) { \
		Log::error("Could not load kvx file: Not enough data in stream " CORE_STRINGIFY(read)); \
		return false; \
	}

bool KVXFormat::loadGroupsPalette(const core::String &filename, io::SeekableReadStream& stream, SceneGraph &sceneGraph, voxel::Palette &palette, const LoadContext &ctx) {
	// Total # of bytes (not including numbytes) in each mip-map level
	// but there is only 1 mip-map level
	uint32_t numbytes;
	wrap(stream.readUInt32(numbytes))

	// Dimensions of voxel. (our depth is kvx height)
	uint32_t xsiz_w, ysiz_d, zsiz_h;
	wrap(stream.readUInt32(xsiz_w))
	wrap(stream.readUInt32(ysiz_d))
	wrap(stream.readUInt32(zsiz_h))

	if (xsiz_w > 256 || ysiz_d > 256 || zsiz_h > 255) {
		Log::error("Dimensions exceeded: w: %i, h: %i, d: %i", xsiz_w, zsiz_h, ysiz_d);
		return false;
	}

	const voxel::Region region(0, 0, 0, (int)xsiz_w - 1, (int)zsiz_h - 1, (int)ysiz_d - 1);
	if (!region.isValid()) {
		Log::error("Invalid region: %i:%i:%i", xsiz_w, zsiz_h, ysiz_d);
		return false;
	}

	/**
	 * Centroid of voxel. For extra precision, this location has been shifted up by 8 bits.
	 */
	SceneGraphTransform transform;
	uint32_t pivx_w, pivy_d, pivz_h;
	wrap(stream.readUInt32(pivx_w))
	wrap(stream.readUInt32(pivy_d))
	wrap(stream.readUInt32(pivz_h))

	pivx_w >>= 8;
	pivy_d >>= 8;
	pivz_h >>= 8;

	pivz_h = zsiz_h - 1 - pivz_h;

	glm::vec3 normalizedPivot;
	normalizedPivot.x = (float)pivx_w / 256.0f;
	normalizedPivot.y = (float)pivy_d / 256.0f;
	normalizedPivot.z = (float)pivz_h / 256.0f;
	core::exchange(normalizedPivot.y, normalizedPivot.z);
	transform.setPivot(normalizedPivot);

	/**
	 * For compression purposes, I store the column pointers
	 * in a way that offers quick access to the data, but with slightly more
	 * overhead in calculating the positions.  See example of usage in voxdata.
	 * NOTE: xoffset[0] = (xsiz+1)*4 + xsiz*(ysiz+1)*2 (ALWAYS)
	 */
	uint16_t xyoffset[256][257];
	uint32_t xoffset[257];
	for (uint32_t x = 0u; x <= xsiz_w; ++x) {
		wrap(stream.readUInt32(xoffset[x]))
	}

	for (uint32_t x = 0u; x < xsiz_w; ++x) {
		for (uint32_t y = 0u; y <= ysiz_d; ++y) {
			wrap(stream.readUInt16(xyoffset[x][y]))
		}
	}

	const uint32_t offset = (xsiz_w + 1) * 4 + xsiz_w * (ysiz_d + 1) * 2;
	if (xoffset[0] != offset) {
		Log::error("Invalid offset values found");
		return false;
	}
	// Read the color palette from the end of the file and convert to our palette
	const int64_t currentPos = stream.pos();
	if (stream.seek(-3l * voxel::PaletteMaxColors, SEEK_END) == -1) {
		Log::error("Can't seek to palette data");
		return false;
	}
	if (stream.pos() < currentPos) {
		Log::error("Seek to palette data yields invalid stream position");
		return false;
	}

	palette.setSize(voxel::PaletteMaxColors);
	/**
	 * The last 768 bytes of the KVX file is a standard 256-color VGA palette.
	 * The palette is in (Red:0, Green:1, Blue:2) order and intensities range
	 * from 0-63.
	 */
	for (int i = 0; i < palette.colorCount(); ++i) {
		uint8_t r, g, b;
		wrap(stream.readUInt8(r))
		wrap(stream.readUInt8(g))
		wrap(stream.readUInt8(b))
		palette.color(i) = core::RGBA(r, g, b);
	}
	stream.seek(currentPos);

	voxel::RawVolume *volume = new voxel::RawVolume(region);
	SceneGraphNode node;
	node.setVolume(volume, true);
	node.setName(filename);
	const KeyFrameIndex keyFrameIdx = 0;
	node.setTransform(keyFrameIdx, transform);
	node.setPalette(palette);
	sceneGraph.emplace(core::move(node));

	uint32_t lastZ = 0;
	voxel::Voxel lastCol;

	for (uint32_t x = 0; x < xsiz_w; ++x) {
		for (uint32_t y = 0; y < ysiz_d; ++y) {
			const uint16_t end = xyoffset[x][y + 1];
			const uint16_t start = xyoffset[x][y];
			int32_t n = end - start;

			while (n > 0) {
				priv::slab header;
				wrap(stream.readUInt8(header.slabztop))
				wrap(stream.readUInt8(header.slabzleng))
				wrap(stream.readUInt8(header.slabbackfacecullinfo))
				for (uint8_t i = 0u; i < header.slabzleng; ++i) {
					uint8_t col;
					wrap(stream.readUInt8(col))
					lastCol = voxel::createVoxel(palette, col);
					volume->setVoxel((int)x, (int)((zsiz_h - 1) - (header.slabztop + i)), (int)y, lastCol);
				}

				/**
				 * the format only saves the visible voxels - we have to take the face info to
				 * fill the inner voxels
				 */
				if (!(header.slabbackfacecullinfo & (1 << 4))) {
					for (uint32_t i = lastZ + 1; i < header.slabztop; ++i) {
						volume->setVoxel((int)x, (int)((zsiz_h - 1) - i), (int)y, lastCol);
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

#define wrapBool(read) \
	if ((read) == false) { \
		Log::error("Could not write kv6 file: Not enough space in stream " CORE_STRINGIFY(read)); \
		return false; \
	}

bool KVXFormat::saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream, const SaveContext &ctx) {
#if 0
	const SceneGraph::MergedVolumePalette &merged = sceneGraph.merge();
	if (merged.first == nullptr) {
		Log::error("Failed to merge volumes");
		return false;
	}

	core::ScopedPtr<voxel::RawVolume> scopedPtr(merged.first);
	const voxel::Region &region = merged.first->region();
	const glm::ivec3 &dim = region.getDimensionsInVoxels();

	if (dim.x > 256 || dim.z > 256 || dim.y > 255) {
		Log::error("Dimensions exceeded: w: %i, h: %i, d: %i", dim.x, dim.y, dim.z);
		return false;
	}

	int32_t xlen[256] {};
	uint16_t xyoffset[256][256] {}; // our z

	core::DynamicArray<priv::slab> voxdata;
	const uint32_t numvoxs = voxelutil::visitSurfaceVolume(*merged.first, [&](int x, int y, int z, const voxel::Voxel &voxel) {
		priv::slab vd;
		vd.x_low_w = x - region.getLowerX();
		// flip y and z here
		vd.y_low_d = z - region.getLowerZ();
		vd.col = voxel.getColor();
		vd.slabzleng = 0; // TODO
		vd.slabztop = region.getHeightInCells() - (y - region.getLowerY()); // TODO
		vd.slabbackfacecullinfo = priv::calculateVisibility(merged.first, x, y, z);
		voxdata.push_back(vd);
		++xlen[x];
		++xyoffset[vd.x_low_w][vd.y_low_d];
	}, voxelutil::VisitorOrder::XZY);

	constexpr uint32_t MAXVOXS = 1048576;
	if (numvoxs > MAXVOXS) {
		Log::error("Max allowed voxels exceeded: %u (max is %u)", numvoxs, MAXVOXS);
		return false;
	}

	const int xsiz_w = dim.x;
	// flip y and z here
	const int ysiz_d = dim.z;
	const int zsiz_h = dim.y;
	wrapBool(stream.writeUInt32(0)) // numbytes

	wrapBool(stream.writeUInt32(xsiz_w))
	wrapBool(stream.writeUInt32(ysiz_d))
	wrapBool(stream.writeUInt32(zsiz_h))

	glm::vec3 pivot(0.0f); // normalized pivot
	wrapBool(stream.writeFloat(-pivot.x))
	wrapBool(stream.writeFloat(pivot.z))
	wrapBool(stream.writeFloat(-pivot.y))

	for (int x = 0u; x <= xsiz_w; ++x) {
		wrapBool(stream.writeInt32(xlen[x]))
		Log::debug("xlen[%u]: %i", x, xlen[x]);
	}

	for (int x = 0u; x < xsiz_w; ++x) {
		for (int y = ysiz_d - 1; y >= 0; --y) {
			wrapBool(stream.writeUInt16(xyoffset[x][y]))
			Log::debug("xyoffset[%u][%u]: %u", x, y, xyoffset[x][y]);
		}
	}

	// TODO: this is not correct
	for (const priv::slab &data : voxdata) {
		const core::RGBA color = merged.second.color(data.col);
		wrapBool(stream.writeUInt8(color.b))
		wrapBool(stream.writeUInt8(color.g))
		wrapBool(stream.writeUInt8(color.r))
		wrapBool(stream.writeUInt8(data.z_low_h))
		wrapBool(stream.writeUInt8(data.slabbackfacecullinfo))
	}

	// palette is last
	for (int i = 0; i < merged.second.colorCount(); ++i) {
		const core::RGBA color = merged.second.color(i);
		wrapBool(stream.writeUInt8(color.b))
		wrapBool(stream.writeUInt8(color.g))
		wrapBool(stream.writeUInt8(color.r))
	}
	for (int i = merged.second.colorCount(); i < voxel::PaletteMaxColors; ++i) {
		wrapBool(stream.writeUInt8(0))
		wrapBool(stream.writeUInt8(0))
		wrapBool(stream.writeUInt8(0))
	}

	const uint32_t numBytes = stream.pos() - sizeof(uint32_t);
	stream.seek(0);
	wrapBool(stream.writeUInt32(numBytes))

	return true;
#else
	return false;
#endif
}

#undef wrapBool

}
