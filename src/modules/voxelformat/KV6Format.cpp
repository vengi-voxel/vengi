/**
 * @file
 */

#include "KV6Format.h"
#include "core/Color.h"
#include "core/Common.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/RGBA.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "io/Stream.h"
#include "voxel/Face.h"
#include "voxel/Palette.h"
#include "voxel/PaletteLookup.h"
#include "voxel/RawVolume.h"
#include "voxelformat/SceneGraph.h"
#include "voxelutil/VolumeVisitor.h"
#include <glm/common.hpp>

namespace voxelformat {


namespace priv {

enum KV6Visibility {
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
		vis |= KV6Visibility::Left;
	}
	if ((visBits & voxel::FaceBits::PositiveX) != voxel::FaceBits::None) {
		vis |= KV6Visibility::Right;
	}
	// y (our z)
	if ((visBits & voxel::FaceBits::NegativeZ) != voxel::FaceBits::None) {
		vis |= KV6Visibility::Front;
	}
	if ((visBits & voxel::FaceBits::PositiveZ) != voxel::FaceBits::None) {
		vis |= KV6Visibility::Back;
	}
	// z (our y) is running from top to bottom
	if ((visBits & voxel::FaceBits::NegativeY) != voxel::FaceBits::None) {
		vis |= KV6Visibility::Up;
	}
	if ((visBits & voxel::FaceBits::PositiveY) != voxel::FaceBits::None) {
		vis |= KV6Visibility::Down;
	}
	return vis;
}

static uint8_t calculateDir(const voxel::RawVolume *, int, int, int, const voxel::Voxel &) {
	return 255; // TODO
}

struct voxtype {
	uint8_t z_low_h = 0;	//<! z coordinate of this surface voxel (height - our y)
	uint8_t z_high = 0; //<! always 0
	uint8_t col = 0;	//<! palette index
	uint8_t vis = 0;	//<! Low 6 bits say if neighbor is solid or air - @sa priv::KV6Visibility
	uint8_t dir = 0;	//<! Uses 256-entry lookup table - lighting bit - @sa priv::directions
};

} // namespace priv

#define wrap(read) \
	if ((read) != 0) { \
		Log::error("Could not load kv6 file: Not enough data in stream " CORE_STRINGIFY(read)); \
		return 0; \
	}

size_t KV6Format::loadPalette(const core::String &filename, io::SeekableReadStream& stream, voxel::Palette &palette) {
	uint32_t magic;
	wrap(stream.readUInt32(magic))
	if (magic != FourCC('K','v','x','l')) {
		Log::error("Invalid magic");
		return 0;
	}

	uint32_t xsiz_w, ysiz_d, zsiz_h;
	wrap(stream.readUInt32(xsiz_w))
	wrap(stream.readUInt32(ysiz_d))
	wrap(stream.readUInt32(zsiz_h))
	glm::vec3 pivot;
	wrap(stream.readFloat(pivot.x))
	wrap(stream.readFloat(pivot.y))
	wrap(stream.readFloat(pivot.z))

	uint32_t numvoxs;
	wrap(stream.readUInt32(numvoxs))

	const int64_t headerSize = 32;
	const int64_t xLenSize = (int64_t)(xsiz_w * sizeof(uint32_t));
	const int64_t yLenSize = (int64_t)((size_t)xsiz_w * (size_t)ysiz_d * sizeof(uint16_t));
	const int64_t paletteOffset = headerSize + (int64_t)(numvoxs * 8) + xLenSize + yLenSize;
	if (stream.seek(paletteOffset) != -1) {
		if (stream.remaining() != 0) {
			uint32_t palMagic;
			wrap(stream.readUInt32(palMagic))
			if (palMagic == FourCC('S','P','a','l')) {
				palette.colorCount = voxel::PaletteMaxColors;
				for (int i = 0; i < palette.colorCount; ++i) {
					uint8_t r, g, b;
					wrap(stream.readUInt8(b))
					wrap(stream.readUInt8(g))
					wrap(stream.readUInt8(r))
					palette.colors[i] = core::RGBA(r, g, b, 255u);
				}
			}
			return palette.colorCount;
		}
	}
	return 0;
}

#undef wrap
#define wrap(read) \
	if ((read) != 0) { \
		Log::error("Could not load kv6 file: Not enough data in stream " CORE_STRINGIFY(read)); \
		return false; \
	}

bool KV6Format::loadGroupsPalette(const core::String &filename, io::SeekableReadStream& stream, SceneGraph &sceneGraph, voxel::Palette &palette) {
	uint32_t magic;
	wrap(stream.readUInt32(magic))
	if (magic != FourCC('K','v','x','l')) {
		Log::error("Invalid magic");
		return false;
	}

	// Dimensions of voxel. (our depth is kvx height)
	uint32_t xsiz_w, ysiz_d, zsiz_h;
	wrap(stream.readUInt32(xsiz_w))
	wrap(stream.readUInt32(ysiz_d))
	wrap(stream.readUInt32(zsiz_h))

	if (xsiz_w > 256 || ysiz_d > 256 || zsiz_h > 255) {
		Log::error("Dimensions exceeded: w: %i, h: %i, d: %i", xsiz_w, zsiz_h, ysiz_d);
		return false;
	}

	SceneGraphTransform transform;
	glm::vec3 pivot;
	wrap(stream.readFloat(pivot.x))
	wrap(stream.readFloat(pivot.y))
	wrap(stream.readFloat(pivot.z))

	pivot.z = (float)zsiz_h - 1.0f - pivot.z;

	glm::vec3 normalizedPivot = pivot / glm::vec3(xsiz_w, ysiz_d, zsiz_h);
	core::exchange(normalizedPivot.y, normalizedPivot.z);
	transform.setPivot(normalizedPivot);

	const voxel::Region region(0, 0, 0, (int)xsiz_w - 1, (int)zsiz_h - 1, (int)ysiz_d - 1);
	if (!region.isValid()) {
		Log::error("Invalid region: %i:%i:%i", xsiz_w, zsiz_h, ysiz_d);
		return false;
	}

	uint32_t numvoxs;
	wrap(stream.readUInt32(numvoxs))
	Log::debug("numvoxs: %u", numvoxs);
	constexpr uint32_t MAXVOXS = 1048576;
	if (numvoxs > MAXVOXS) {
		Log::error("Max allowed voxels exceeded: %u (max is %u)", numvoxs, MAXVOXS);
		return false;
	}

	const int64_t headerSize = 32;
	const int64_t xLenSize = (int64_t)(xsiz_w * sizeof(uint32_t));
	const int64_t yLenSize = (int64_t)((size_t)xsiz_w * (size_t)ysiz_d * sizeof(uint16_t));
	const int64_t paletteOffset = headerSize + (int64_t)numvoxs * (int64_t)8 + xLenSize + yLenSize;
	if (stream.seek(paletteOffset) != -1) {
		if (stream.remaining() != 0) {
			uint32_t palMagic;
			wrap(stream.readUInt32(palMagic))
			if (palMagic == FourCC('S','P','a','l')) {
				palette.colorCount = voxel::PaletteMaxColors;
				for (int i = 0; i < palette.colorCount; ++i) {
					uint8_t r, g, b;
					wrap(stream.readUInt8(b))
					wrap(stream.readUInt8(g))
					wrap(stream.readUInt8(r))
					palette.colors[i] = core::RGBA(r, g, b, 255u);
				}
			}
		}
	}
	stream.seek(headerSize);

	priv::voxtype voxdata[MAXVOXS];
	voxel::PaletteLookup palLookup(palette);
	for (uint32_t c = 0u; c < numvoxs; ++c) {
		uint8_t palr, palg, palb, pala;
		wrap(stream.readUInt8(palb))
		wrap(stream.readUInt8(palg))
		wrap(stream.readUInt8(palr))
		wrap(stream.readUInt8(pala)) // always 128
		const glm::vec4& color = core::Color::fromRGBA(palr, palg, palb, 255);
		voxdata[c].col = palLookup.findClosestIndex(color);
		wrap(stream.readUInt8(voxdata[c].z_low_h))
		wrap(stream.readUInt8(voxdata[c].z_high))
		wrap(stream.readUInt8(voxdata[c].vis))
		wrap(stream.readUInt8(voxdata[c].dir))
		Log::debug("voxel %u/%u z-low: %u, z_high: %u, vis: %i. dir: %u, pal: %u",
				c, numvoxs, voxdata[c].z_low_h, voxdata[c].z_high, voxdata[c].vis, voxdata[c].dir, voxdata[c].col);
	}
	int32_t xlen[256] {};
	for (uint32_t x = 0u; x < xsiz_w; ++x) {
		wrap(stream.readInt32(xlen[x]))
		Log::debug("xlen[%u]: %i", x, xlen[x]);
	}

	uint16_t xyoffset[256][256] {};
	for (uint32_t x = 0u; x < xsiz_w; ++x) {
		for (uint32_t y = 0u; y < ysiz_d; ++y) {
			wrap(stream.readUInt16(xyoffset[x][y]))
			Log::debug("xyoffset[%u][%u]: %u", x, y, xyoffset[x][y]);
		}
	}

	voxel::RawVolume *volume = new voxel::RawVolume(region);

	int idx = 0;
	for (uint32_t x = 0; x < xsiz_w; ++x) {
		for (uint32_t y = 0; y < ysiz_d; ++y) {
			for (int end = idx + xyoffset[x][y]; idx < end; ++idx) {
				const priv::voxtype& vox = voxdata[idx];
				const voxel::Voxel col = voxel::createVoxel(voxel::VoxelType::Generic, vox.col);
				volume->setVoxel((int)x, (int)((zsiz_h - 1) - vox.z_low_h), (int)y, col);
			}
		}
	}

	idx = 0;
	for (uint32_t x = 0; x < xsiz_w; ++x) {
		for (uint32_t y = 0; y < ysiz_d; ++y) {
			voxel::Voxel lastCol;
			uint32_t lastZ = 256;
			for (int end = idx + xyoffset[x][y]; idx < end; ++idx) {
				const priv::voxtype& vox = voxdata[idx];
				if (vox.vis & priv::KV6Visibility::Up) {
					lastZ = vox.z_low_h;
					lastCol = voxel::createVoxel(voxel::VoxelType::Generic, vox.col);
				}
				if (vox.vis & priv::KV6Visibility::Down) {
					for (; lastZ < vox.z_low_h; ++lastZ) {
						volume->setVoxel((int)x, (int)((zsiz_h - 1) - lastZ), (int)y, lastCol);
					}
				}
			}
		}
	}

	SceneGraphNode node;
	node.setVolume(volume, true);
	node.setName(filename);
	KeyFrameIndex keyFrameIdx = 0;
	node.setTransform(keyFrameIdx, transform);
	node.setPalette(palLookup.palette());
	sceneGraph.emplace(core::move(node));

	return true;
}

#undef wrap

#define wrapBool(read) \
	if ((read) == false) { \
		Log::error("Could not write kv6 file: Not enough space in stream " CORE_STRINGIFY(read)); \
		return false; \
	}

bool KV6Format::saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream, ThumbnailCreator thumbnailCreator) {
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

	core::DynamicArray<priv::voxtype> voxdata;
	const uint32_t numvoxs = voxelutil::visitSurfaceVolume(*merged.first, [&](int x, int y, int z, const voxel::Voxel &voxel) {
		priv::voxtype vd;
		const int x_low_w = x - region.getLowerX();
		// flip y and z here
		const int y_low_d = z - region.getLowerZ();
		vd.z_low_h = region.getHeightInCells() - (y - region.getLowerY());
		vd.z_high = 0;
		vd.col = voxel.getColor();
		vd.vis = priv::calculateVisibility(merged.first, x, y, z);
		vd.dir = priv::calculateDir(merged.first, x, y, z, voxel);
		voxdata.push_back(vd);
		++xlen[x_low_w];
		++xyoffset[x_low_w][y_low_d];
	}, voxelutil::VisitorOrder::XZY);

	constexpr uint32_t MAXVOXS = 1048576;
	if (numvoxs > MAXVOXS) {
		Log::error("Max allowed voxels exceeded: %u (max is %u)", numvoxs, MAXVOXS);
		return false;
	}

	wrapBool(stream.writeUInt32(FourCC('K','v','x','l')))

	const int xsiz_w = dim.x;
	// flip y and z here
	const int ysiz_d = dim.z;
	const int zsiz_h = dim.y;
	wrapBool(stream.writeUInt32(xsiz_w))
	wrapBool(stream.writeUInt32(ysiz_d))
	wrapBool(stream.writeUInt32(zsiz_h))

	glm::vec3 pivot(0.0f);
	wrapBool(stream.writeFloat(-pivot.x))
	wrapBool(stream.writeFloat(pivot.z))
	wrapBool(stream.writeFloat(-pivot.y))

	wrapBool(stream.writeUInt32(numvoxs))

	for (const priv::voxtype &data : voxdata) {
		const core::RGBA color = merged.second.colors[data.col];
		wrapBool(stream.writeUInt8(color.b))
		wrapBool(stream.writeUInt8(color.g))
		wrapBool(stream.writeUInt8(color.r))
		wrapBool(stream.writeUInt8(128))
		wrapBool(stream.writeUInt8(data.z_low_h))
		wrapBool(stream.writeUInt8(data.z_high))
		wrapBool(stream.writeUInt8(data.vis))
		wrapBool(stream.writeUInt8(data.dir))
		Log::debug("voxel z-low: %u, z_high: %u, vis: %i. dir: %u, pal: %u",
				data.z_low_h, data.z_high, data.vis, data.dir, data.col);
	}

	for (int x = 0u; x < xsiz_w; ++x) {
		wrapBool(stream.writeInt32(xlen[x]))
		Log::debug("xlen[%u]: %i", x, xlen[x]);
	}

	for (int x = 0u; x < xsiz_w; ++x) {
		for (int y = ysiz_d - 1; y >= 0; --y) {
			wrapBool(stream.writeUInt16(xyoffset[x][y]))
			Log::debug("xyoffset[%u][%u]: %u", x, y, xyoffset[x][y]);
		}
	}

	const uint32_t palMagic = FourCC('S','P','a','l');
	wrapBool(stream.writeUInt32(palMagic))
	for (int i = 0; i < merged.second.colorCount; ++i) {
		const core::RGBA color = merged.second.colors[i];
		wrapBool(stream.writeUInt8(color.b))
		wrapBool(stream.writeUInt8(color.g))
		wrapBool(stream.writeUInt8(color.r))
	}
	for (int i = merged.second.colorCount; i < voxel::PaletteMaxColors; ++i) {
		wrapBool(stream.writeUInt8(0))
		wrapBool(stream.writeUInt8(0))
		wrapBool(stream.writeUInt8(0))
	}

	return true;
}

#undef wrapBool

}
