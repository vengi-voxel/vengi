/**
 * @file
 */

#include "KV6Format.h"
#include "SLABShared.h"
#include "core/Color.h"
#include "core/Common.h"
#include "core/Enum.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/RGBA.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "io/Stream.h"
#include "scenegraph/SceneGraph.h"
#include "voxel/Face.h"
#include "voxel/Palette.h"
#include "voxel/RawVolume.h"
#include "voxelutil/VolumeVisitor.h"
#include <glm/common.hpp>

namespace voxelformat {

namespace priv {

constexpr uint32_t MAXVOXS = 1048576;

struct VoxtypeKV6 {
	/** z coordinate of this surface voxel (height - our y) */
	uint8_t z = 0;
	/** palette index */
	uint8_t col = 0;
	/** Low 6 bits say if neighbor is solid or air - @sa priv::SLABVisibility */
	SLABVisibility vis = SLABVisibility::None;
	/** Uses 256-entry lookup table - lighting bit - @sa priv::directions */
	uint8_t dir = 0;
};

struct State {
	VoxtypeKV6 voxdata[MAXVOXS];
	int32_t xoffsets[256]{};
	uint16_t xyoffsets[256][256]{};
};

// lighting value that distributes above a radius of 3 around the position
static uint8_t calculateDir(const voxel::RawVolume *, int, int, int, const voxel::Voxel &) {
	return 0u; // TODO
}

} // namespace priv

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load kv6 file: Not enough data in stream " CORE_STRINGIFY(read));                        \
		return 0;                                                                                                      \
	}

size_t KV6Format::loadPalette(const core::String &filename, io::SeekableReadStream &stream, voxel::Palette &palette,
							  const LoadContext &ctx) {
	uint32_t magic;
	wrap(stream.readUInt32(magic))
	if (magic != FourCC('K', 'v', 'x', 'l')) {
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
			if (palMagic == FourCC('S', 'P', 'a', 'l')) {
				palette.setSize(voxel::PaletteMaxColors);
				for (int i = 0; i < voxel::PaletteMaxColors; ++i) {
					uint8_t r, g, b;
					wrap(stream.readUInt8(b))
					wrap(stream.readUInt8(g))
					wrap(stream.readUInt8(r))
					palette.color(i) = core::RGBA(r, g, b, 255u);
				}
			}
			return palette.size();
		}
	}

	stream.seek(headerSize);

	for (uint32_t c = 0u; c < numvoxs; ++c) {
		uint8_t palr, palg, palb;
		wrap(stream.readUInt8(palb))
		wrap(stream.readUInt8(palg))
		wrap(stream.readUInt8(palr))
		core::RGBA color(palr, palg, palb, 255);
		palette.addColorToPalette(color);
		stream.skip(5);
	}

	return palette.size();
}

#undef wrap
#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load kv6 file: Not enough data in stream " CORE_STRINGIFY(read));                        \
		return false;                                                                                                  \
	}

bool KV6Format::loadGroupsPalette(const core::String &filename, io::SeekableReadStream &stream,
								  scenegraph::SceneGraph &sceneGraph, voxel::Palette &palette, const LoadContext &ctx) {
	uint32_t magic;
	wrap(stream.readUInt32(magic))
	if (magic != FourCC('K', 'v', 'x', 'l')) {
		Log::error("Invalid magic");
		return false;
	}

	// Dimensions of voxel. (our depth is kv6 height)
	uint32_t width, depth, height;
	wrap(stream.readUInt32(width))
	wrap(stream.readUInt32(depth))
	wrap(stream.readUInt32(height))

	if (width > 256 || depth > 256 || height > 255) {
		Log::error("Dimensions exceeded: w: %i, h: %i, d: %i", width, height, depth);
		return false;
	}

	scenegraph::SceneGraphTransform transform;
	glm::vec3 pivot;
	wrap(stream.readFloat(pivot.x)) // width
	wrap(stream.readFloat(pivot.z)) // depth
	wrap(stream.readFloat(pivot.y)) // height

	glm::vec3 normalizedPivot = pivot / glm::vec3(width, height, depth);

	const voxel::Region region(0, 0, 0, (int)width - 1, (int)height - 1, (int)depth - 1);
	if (!region.isValid()) {
		Log::error("Invalid region: %i:%i:%i", width, height, depth);
		return false;
	}

	uint32_t numvoxs;
	wrap(stream.readUInt32(numvoxs))
	Log::debug("numvoxs: %u", numvoxs);
	if (numvoxs > priv::MAXVOXS) {
		Log::error("Max allowed voxels exceeded: %u (max is %u)", numvoxs, priv::MAXVOXS);
		return false;
	}

	const int64_t headerSize = 32;
	const int64_t xLenSize = (int64_t)(width * sizeof(uint32_t));
	const int64_t xyLenSize = (int64_t)((size_t)width * (size_t)depth * sizeof(uint16_t));
	const int64_t paletteOffset = headerSize + (int64_t)numvoxs * (int64_t)8 + xLenSize + xyLenSize;
	if (stream.seek(paletteOffset) != -1) {
		if (stream.remaining() != 0) {
			uint32_t palMagic;
			wrap(stream.readUInt32(palMagic))
			if (palMagic == FourCC('S', 'P', 'a', 'l')) {
				palette.setSize(voxel::PaletteMaxColors);
				for (int i = 0; i < voxel::PaletteMaxColors; ++i) {
					uint8_t r, g, b;
					wrap(stream.readUInt8(b))
					wrap(stream.readUInt8(g))
					wrap(stream.readUInt8(r))
					palette.color(i) = core::RGBA(r, g, b, 255u);
				}
			}
		}
	}
	stream.seek(headerSize);

	core::ScopedPtr<priv::State> state(new priv::State());
	for (uint32_t c = 0u; c < numvoxs; ++c) {
		uint8_t palr, palg, palb, pala;
		wrap(stream.readUInt8(palb))
		wrap(stream.readUInt8(palg))
		wrap(stream.readUInt8(palr))
		wrap(stream.readUInt8(pala)) // always 128
		core::RGBA color(palr, palg, palb, 255);
		palette.addColorToPalette(color, false, &state->voxdata[c].col);
		wrap(stream.readUInt8(state->voxdata[c].z))
		uint8_t zhigh;
		wrap(stream.readUInt8(zhigh))
		wrap(stream.readUInt8((uint8_t &)state->voxdata[c].vis))
		wrap(stream.readUInt8(state->voxdata[c].dir))
		Log::debug("voxel %u/%u z: %u, vis: %i. dir: %u, pal: %u", c, numvoxs, state->voxdata[c].z,
				   (uint8_t)state->voxdata[c].vis, state->voxdata[c].dir, state->voxdata[c].col);
	}
	for (uint32_t x = 0u; x < width; ++x) {
		wrap(stream.readInt32(state->xoffsets[x]))
		Log::debug("xlen[%u]: %i", x, state->xoffsets[x]);
	}

	for (uint32_t x = 0u; x < width; ++x) {
		for (uint32_t y = 0u; y < depth; ++y) {
			wrap(stream.readUInt16(state->xyoffsets[x][y]))
			Log::debug("xyoffset[%u][%u]: %u", x, y, state->xyoffsets[x][y]);
		}
	}

	voxel::RawVolume *volume = new voxel::RawVolume(region);

	int idx = 0;
	for (uint32_t x = 0; x < width; ++x) {
		for (uint32_t y = 0; y < depth; ++y) {
			for (int end = idx + state->xyoffsets[x][y]; idx < end; ++idx) {
				const priv::VoxtypeKV6 &vox = state->voxdata[idx];
				const voxel::Voxel col = voxel::createVoxel(palette, vox.col);
				volume->setVoxel((int)x, (int)((height - 1) - vox.z), (int)y, col);
			}
		}
	}

	idx = 0;
	for (uint32_t x = 0; x < width; ++x) {
		for (uint32_t y = 0; y < depth; ++y) {
			voxel::Voxel lastCol;
			uint32_t lastZ = 256;
			for (int end = idx + state->xyoffsets[x][y]; idx < end; ++idx) {
				const priv::VoxtypeKV6 &vox = state->voxdata[idx];
				if ((vox.vis & priv::SLABVisibility::Up) != priv::SLABVisibility::None) {
					lastZ = vox.z;
					lastCol = voxel::createVoxel(palette, vox.col);
				}
				if ((vox.vis & priv::SLABVisibility::Down) != priv::SLABVisibility::None) {
					for (; lastZ < vox.z; ++lastZ) {
						volume->setVoxel((int)x, (int)((height - 1) - lastZ), (int)y, lastCol);
					}
				}
			}
		}
	}

	scenegraph::SceneGraphNode node;
	node.setVolume(volume, true);
	node.setName(filename);
	scenegraph::KeyFrameIndex keyFrameIdx = 0;
	node.setPivot(normalizedPivot);
	node.setTransform(keyFrameIdx, transform);
	node.setPalette(palette);
	sceneGraph.emplace(core::move(node));

	return true;
}

#undef wrap

#define wrapBool(read)                                                                                                 \
	if ((read) == false) {                                                                                             \
		Log::error("Could not write kv6 file: Not enough space in stream " CORE_STRINGIFY(read));                      \
		return false;                                                                                                  \
	}

bool KV6Format::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
						   io::SeekableWriteStream &stream, const SaveContext &ctx) {
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	core_assert(node);

	const voxel::Region &region = node->region();
	const glm::ivec3 &dim = region.getDimensionsInVoxels();

	if (dim.x > 256 || dim.z > 256 || dim.y > 255) {
		Log::error("Dimensions exceeded: w: %i, h: %i, d: %i", dim.x, dim.y, dim.z);
		return false;
	}

	int32_t xoffsets[256]{};
	uint16_t xyoffsets[256][256]{}; // our z

	constexpr uint32_t MAXVOXS = 1048576;
	core::DynamicArray<priv::VoxtypeKV6> voxdata;
	voxdata.reserve(MAXVOXS);

	const uint32_t numvoxs = voxelutil::visitSurfaceVolume(
		*node->volume(),
		[&](int x, int y, int z, const voxel::Voxel &voxel) {
			const int shiftedX = x - region.getLowerX();
			const int shiftedY = y - region.getLowerY();
			const int shiftedZ = z - region.getLowerZ();
			++xoffsets[shiftedX];
			++xyoffsets[shiftedX][shiftedZ];

			priv::VoxtypeKV6 vd;
			vd.z = region.getHeightInCells() - shiftedY;
			vd.col = voxel.getColor();
			vd.vis = priv::calculateVisibility(node->volume(), x, y, z);
			vd.dir = priv::calculateDir(node->volume(), x, y, z, voxel);
			voxdata.push_back(vd);
		},
		voxelutil::VisitorOrder::XZY);

	if (numvoxs > MAXVOXS) {
		Log::error("Max allowed voxels exceeded: %u (max is %u)", numvoxs, MAXVOXS);
		return false;
	}

	wrapBool(stream.writeUInt32(FourCC('K', 'v', 'x', 'l')))

	const int xsiz_w = dim.x;
	// flip y and z here
	const int ysiz_d = dim.z;
	const int zsiz_h = dim.y;
	wrapBool(stream.writeUInt32(xsiz_w))
	wrapBool(stream.writeUInt32(ysiz_d))
	wrapBool(stream.writeUInt32(zsiz_h))

	glm::vec3 pivot = node->pivot() * glm::vec3(region.getDimensionsInVoxels());
	wrapBool(stream.writeFloat(pivot.x))
	wrapBool(stream.writeFloat(pivot.z))
	wrapBool(stream.writeFloat(pivot.y))

	wrapBool(stream.writeUInt32(numvoxs))

	for (const priv::VoxtypeKV6 &data : voxdata) {
		const core::RGBA color = node->palette().color(data.col);
		wrapBool(stream.writeUInt8(color.b))
		wrapBool(stream.writeUInt8(color.g))
		wrapBool(stream.writeUInt8(color.r))
		wrapBool(stream.writeUInt8(0)) // 128
		wrapBool(stream.writeUInt8(data.z))
		wrapBool(stream.writeUInt8(0))
		wrapBool(stream.writeUInt8((uint8_t)data.vis))
		wrapBool(stream.writeUInt8(data.dir))
		Log::debug("voxel z-low: %u, vis: %i. dir: %u, pal: %u", data.z, (uint8_t)data.vis, data.dir, data.col);
	}

	for (int x = 0u; x < xsiz_w; ++x) {
		wrapBool(stream.writeInt32(xoffsets[x]))
		Log::debug("xoffsets[%u]: %i", x, xoffsets[x]);
	}

	for (int x = 0; x < xsiz_w; ++x) {
		for (int y = ysiz_d - 1; y >= 0; --y) {
			wrapBool(stream.writeUInt16(xyoffsets[x][y]))
			Log::debug("xyoffsets[%u][%u]: %u", x, y, xyoffsets[x][y]);
		}
	}

	const uint32_t palMagic = FourCC('S', 'P', 'a', 'l');
	wrapBool(stream.writeUInt32(palMagic))
	for (int i = 0; i < node->palette().colorCount(); ++i) {
		const core::RGBA color = node->palette().color(i);
		wrapBool(stream.writeUInt8(color.b))
		wrapBool(stream.writeUInt8(color.g))
		wrapBool(stream.writeUInt8(color.r))
	}
	for (int i = node->palette().colorCount(); i < voxel::PaletteMaxColors; ++i) {
		wrapBool(stream.writeUInt8(0))
		wrapBool(stream.writeUInt8(0))
		wrapBool(stream.writeUInt8(0))
	}

	return true;
}

#undef wrapBool

} // namespace voxelformat
