/**
 * @file
 */

#include "KVXFormat.h"
#include "SLABShared.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/collection/Vector.h"
#include "io/Stream.h"
#include "scenegraph/SceneGraph.h"
#include "voxel/MaterialColor.h"
#include "palette/Palette.h"
#include "voxel/RawVolume.h"
#include <glm/common.hpp>

namespace voxelformat {

namespace priv {

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
struct VoxtypeKVX {
	uint8_t ztop = 0;
	uint8_t zlength = 0;
	SLABVisibility vis = SLABVisibility::None;
	core::Vector<uint8_t, 256> colors; // zlength entries
};

} // namespace priv

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load kvx file: Not enough data in stream " CORE_STRINGIFY(read));                        \
		return false;                                                                                                  \
	}

#define wrapBool(read)                                                                                                 \
	if ((read) == false) {                                                                                             \
		Log::error("Could not load kv6 file: Not enough space in stream " CORE_STRINGIFY(read));                       \
		return false;                                                                                                  \
	}

bool KVXFormat::loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
								  scenegraph::SceneGraph &sceneGraph, palette::Palette &palette, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	// Total # of bytes (not including numbytes) in each mip-map level
	// but there is only 1 mip-map level (or 5 in unstripped kvx files)
	uint32_t numbytes;
	wrap(stream->readUInt32(numbytes))

	// Dimensions of voxel. (our depth is kvx height)
	uint32_t xsiz_w, ysiz_d, zsiz_h;
	wrap(stream->readUInt32(xsiz_w))
	wrap(stream->readUInt32(ysiz_d))
	wrap(stream->readUInt32(zsiz_h))

	Log::debug("Dimensions: %i:%i:%i", xsiz_w, ysiz_d, zsiz_h);

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
	scenegraph::SceneGraphTransform transform;
	int32_t pivx_w, pivy_d, pivz_h;
	wrap(stream->readInt32(pivx_w))
	wrap(stream->readInt32(pivy_d))
	wrap(stream->readInt32(pivz_h))

	// For extra precision, this location has been shifted up by 8 bits.
	pivx_w >>= 8;
	pivy_d >>= 8;
	pivz_h >>= 8;

	pivz_h = zsiz_h - 1 - pivz_h;

	glm::vec3 normalizedPivot;
	normalizedPivot.x = (float)pivx_w / 256.0f;
	normalizedPivot.y = (float)pivy_d / 256.0f;
	normalizedPivot.z = (float)pivz_h / 256.0f;
	core::exchange(normalizedPivot.y, normalizedPivot.z);

	/**
	 * For compression purposes, I store the column pointers
	 * in a way that offers quick access to the data, but with slightly more
	 * overhead in calculating the positions.  See example of usage in voxdata.
	 * NOTE: xoffset[0] = (xsiz+1)*4 + xsiz*(ysiz+1)*2 (ALWAYS)
	 */
	uint16_t xyoffsets[256][257];
	uint32_t xoffsets[257];
	for (uint32_t x = 0u; x <= xsiz_w; ++x) {
		wrap(stream->readUInt32(xoffsets[x]))
	}

	for (uint32_t x = 0u; x < xsiz_w; ++x) {
		for (uint32_t y = 0u; y <= ysiz_d; ++y) {
			wrap(stream->readUInt16(xyoffsets[x][y]))
		}
	}

	const uint32_t offset = (xsiz_w + 1) * 4 + xsiz_w * (ysiz_d + 1) * 2;
	if (xoffsets[0] != offset) {
		Log::error("Invalid offset values found");
		return false;
	}
	// Read the color palette from the end of the file and convert to our palette
	const int64_t currentPos = stream->pos();
	if (stream->seek(-3l * palette::PaletteMaxColors, SEEK_END) == -1) {
		Log::error("Can't seek to palette data");
		return false;
	}
	if (stream->pos() < currentPos) {
		Log::error("Seek to palette data yields invalid stream position");
		return false;
	}

	palette.setSize(palette::PaletteMaxColors);
	/**
	 * The last 768 bytes of the KVX file is a standard 256-color VGA palette.
	 * The palette is in (Red:0, Green:1, Blue:2) order and intensities range
	 * from 0-63.
	 */
	for (int i = 0; i < palette.colorCount(); ++i) {
		color::RGBA color;
		wrapBool(priv::readRGBScaledColor(*stream, color))
		palette.setColor(i, color);
	}
	stream->seek(currentPos);

	voxel::RawVolume *volume = new voxel::RawVolume(region);
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(volume, true);
	node.setName(core::string::extractFilename(filename));
	const scenegraph::KeyFrameIndex keyFrameIdx = 0;
	node.setTransform(keyFrameIdx, transform);
	node.setPalette(palette);
	node.setPivot(normalizedPivot);
	sceneGraph.emplace(core::move(node));

	// TODO: PERF: use volume sampler
	for (uint32_t x = 0; x < xsiz_w; ++x) {
		for (uint32_t y = 0; y < ysiz_d; ++y) {
			const uint16_t end = xyoffsets[x][y + 1];
			const uint16_t start = xyoffsets[x][y];
			int32_t n = end - start;

			while (n > 0) {
				priv::VoxtypeKVX header;
				wrap(stream->readUInt8(header.ztop))
				wrap(stream->readUInt8(header.zlength))
				wrap(stream->readUInt8((uint8_t &)header.vis))
				for (uint8_t i = 0u; i < header.zlength; ++i) {
					uint8_t col;
					wrap(stream->readUInt8(col))
					voxel::Voxel voxel = voxel::createVoxel(palette, col);
					const int nx = (int)x;
					const int ny = region.getUpperY() - (int)header.ztop - (int)i;
					const int nz = (int)y;
					volume->setVoxel(nx, ny, nz, voxel);
				}
				n -= (int32_t)(header.zlength + 3 /* 3 byte slab header */);
			}
		}
	}

	return true;
}

#undef wrap
#undef wrapBool

#define wrapBool(read)                                                                                                 \
	if ((read) == false) {                                                                                             \
		Log::error("Could not write kv6 file: Not enough space in stream " CORE_STRINGIFY(read));                      \
		return false;                                                                                                  \
	}

bool KVXFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
						   const io::ArchivePtr &archive, const SaveContext &ctx) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	core_assert(node);

	const voxel::Region &region = node->volume()->region();
	const glm::ivec3 &dim = region.getDimensionsInVoxels();

	if (dim.x > 256 || dim.z > 256 || dim.y > 255) {
		Log::error("Dimensions exceeded: w: %i, h: %i, d: %i", dim.x, dim.y, dim.z);
		return false;
	}

	const voxel::RawVolume &volume = *node->volume();

	const int64_t numBytesPos = stream->pos();
	core_assert(numBytesPos == 0);
	wrapBool(stream->writeUInt32(0)) // numbytes

	// flip y and z here
	wrapBool(stream->writeUInt32(dim.x))
	wrapBool(stream->writeUInt32(dim.z))
	wrapBool(stream->writeUInt32(dim.y))
	Log::debug("Dimensions: %i:%i:%i", dim.x, dim.z, dim.y);

	// TODO: VOXELFORMAT: support the pivot
	glm::ivec3 pivot(0); // normalized pivot
	wrapBool(stream->writeInt32(-pivot.x))
	wrapBool(stream->writeInt32(pivot.z))
	wrapBool(stream->writeInt32(-pivot.y))

	const int64_t offsetPos = stream->pos();
	const size_t xoffsetSize = (dim.x + 1) * sizeof(uint32_t);
	const size_t xyoffsetSize = dim.x * (dim.z + 1) * sizeof(uint16_t);
	// skip offset tables for now - filled later
	if (stream->seek(xoffsetSize + xyoffsetSize, SEEK_CUR) == -1) {
		Log::error("Can't seek past offset tables");
		return false;
	}

	priv::VoxtypeKVX voxdat;
	int32_t xoffsets[256 + 1];
	uint16_t xyoffsets[256][256 + 1];
	int32_t xoffset = xoffsetSize + xyoffsetSize;
	for (int32_t x = 0; x < dim.x; x++) {
		xoffsets[x] = xoffset;
		for (int32_t y = 0; y < dim.z; y++) {
			xyoffsets[x][y] = (uint16_t)(xoffset - xoffsets[x]);
			int32_t bytes = 0;
			for (int32_t z = 0; z < dim.y; z++) {
				const int nx = region.getLowerX() + x;
				const int ny = region.getUpperY() - z;
				const int nz = region.getLowerZ() + y;
				priv::SLABVisibility vis = priv::calculateVisibility(&volume, nx, ny, nz);
				if (vis == priv::SLABVisibility::None) {
					continue;
				}

				if (!bytes || z > voxdat.ztop + voxdat.zlength) {
					if (bytes) {
						stream->writeUInt8(voxdat.ztop);
						stream->writeUInt8(voxdat.zlength);
						stream->writeUInt8((uint8_t)voxdat.vis);
						for (uint8_t k = 0u; k < voxdat.zlength; ++k) {
							stream->writeUInt8(voxdat.colors[k]);
						}

						xoffset += bytes;
					}
					voxdat.ztop = (uint8_t)z;
					voxdat.zlength = 0;
					voxdat.vis = priv::SLABVisibility::None;
					voxdat.colors.clear();
					bytes = 3; // header bytes
				}
				voxdat.zlength++;
				voxdat.vis |= vis;
				const voxel::Voxel &voxel = volume.voxel(nx, ny, nz);
				const uint8_t palIdx = voxel.getColor();
				voxdat.colors.push_back(palIdx);
				bytes++;
			}
			if (bytes) {
				stream->writeUInt8(voxdat.ztop);
				stream->writeUInt8(voxdat.zlength);
				stream->writeUInt8((uint8_t)voxdat.vis);
				for (uint8_t k = 0u; k < voxdat.zlength; ++k) {
					stream->writeUInt8(voxdat.colors[k]);
				}
				xoffset += bytes;
			}
		}
		xyoffsets[x][dim.z] = (uint16_t)(xoffset - xoffsets[x]);
	}
	xoffsets[dim.x] = xoffset;

	// palette is last
	const palette::Palette &palette = node->palette();
	for (int i = 0; i < palette.colorCount(); ++i) {
		const color::RGBA color = palette.color(i);
		wrapBool(priv::writeRGBScaledColor(*stream, color))
	}
	for (int i = palette.colorCount(); i < palette::PaletteMaxColors; ++i) {
		color::RGBA color(0);
		wrapBool(priv::writeRGBScaledColor(*stream, color))
	}

	if (stream->seek(offsetPos) == -1) {
		Log::error("Can't seek to offset tables");
		return false;
	}
	for (int x = 0u; x <= dim.x; ++x) {
		wrapBool(stream->writeInt32(xoffsets[x]))
	}
	for (int x = 0u; x < dim.x; ++x) {
		for (int y = 0; y <= dim.z; ++y) {
			wrapBool(stream->writeUInt16(xyoffsets[x][y]))
		}
	}
	if (stream->seek(numBytesPos) == -1) {
		Log::error("Can't seek to numbytes");
		return false;
	}
	wrapBool(stream->writeUInt32(xoffsets[dim.x] + 24)); // header size

	if (stream->seek(0, SEEK_END) == -1) {
		Log::error("Can't seek to end of file");
		return false;
	}

	return true;
}

#undef wrapBool

} // namespace voxelformat
