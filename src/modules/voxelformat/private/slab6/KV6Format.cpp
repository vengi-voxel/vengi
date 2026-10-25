/**
 * @file
 */

#include "KV6Format.h"
#include "KFAFormat.h"
#include "SLABShared.h"
#include "core/Common.h"
#include "core/ConfigVar.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "color/RGBA.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "core/collection/Buffer.h"
#include "core/collection/DynamicArray.h"
#include "io/Archive.h"
#include "io/Stream.h"
#include "palette/NormalPalette.h"
#include "palette/PaletteLookup.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "palette/Palette.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelformat/Format.h"
#include "voxelutil/VolumeVisitor.h"
#include "voxelutil/VoxelUtil.h"
#include <float.h>
#include <glm/common.hpp>
#include <glm/gtc/constants.hpp>

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
	/** Uses 256-entry lookup table - lighting bit - see built-in:slab6 */
	uint8_t normal = 0;
};

struct State {
	VoxtypeKV6 voxdata[MAXVOXS];
	int32_t xoffsets[256]{};
	uint16_t xyoffsets[256][256]{};
};

// lighting value that distributes above a radius of 3 around the position
static uint8_t calculateDir(const voxel::RawVolume *v, int x, int y, int z, const voxel::Voxel &) {
	const int radius = 3;
	const int radiusVal = (radius + 1) * (radius + 1);
	int offsetX = 0, offsetY = 0, offsetZ = 0;
	voxel::RawVolume::Sampler sampler(*v);
	for (int xr = -radius; xr <= radius; xr++) {
		sampler.setPosition(x + xr, y - radius, z - radius);
		if (!sampler.currentPositionValid()) {
			continue;
		}
		const int xVal = xr * xr;
		for (int yr = -radius; yr <= radius; yr++) {
			if (!sampler.currentPositionValid()) {
				continue;
			}
			const int yVal = yr * yr;
			const int sum = xVal + yVal;
			for (int zr = -radius; zr <= radius; zr++) {
				if (!sampler.currentPositionValid()) {
					continue;
				}
				if (sum + zr * zr <= radiusVal) {
					offsetX += xr;
					offsetY += yr;
					offsetZ += zr;
				}
				sampler.movePositiveZ();
			}
			sampler.movePositiveY();
		}
	}

	// If voxels aren't directional (thin), return the 0 vector (no direction)
	const int contribution = offsetX * offsetX + offsetY * offsetY + offsetZ * offsetZ;
	if (contribution < 32 * 32) {
		return 255u;
	}

	// this is disabled in slab6 - but appears to be easier to re-implement. Let's see if it works...
	double maxF = DBL_MIN;
	uint8_t j = 0u;
	double zmulk = 2.0 / 255.0;
	double zaddk = zmulk * 0.5 - 1.0;
	const double GOLDRAT = 0.3819660112501052;
	const float goldratpi2 = GOLDRAT * glm::two_pi<double>();

	for (uint8_t i = 0; i < 255u; ++i) {
		glm::dvec3 result;
		result.z = i * zmulk + zaddk;
		const double r = glm::sqrt(1.0 - result.z * result.z);
		const double val = i * goldratpi2;
		result.x = glm::cos(val) * r;
		result.y = glm::sin(val) * r;
		const double f2 = result.x * (double)offsetX + result.z * (double)offsetY - result.y * (double)offsetZ;
		if (f2 > maxF) {
			maxF = f2;
			j = i;
		}
	}
	return j;
}

} // namespace priv

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load kv6 file: Not enough data in stream " CORE_STRINGIFY(read));                        \
		return 0;                                                                                                      \
	}
#define wrap2(read)                                                                                                     \
	if ((read) == -1) {                                                                                                 \
		Log::error("Could not load kv6 file: Not enough data in stream " CORE_STRINGIFY(read));                        \
		return false;                                                                                                  \
	}
#define wrapBool(read)                                                                                                 \
	if ((read) == false) {                                                                                             \
		Log::error("Could not load kv6 file: Not enough space in stream " CORE_STRINGIFY(read));                       \
		return false;                                                                                                  \
	}

size_t KV6Format::loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
							  const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return 0;
	}
	uint32_t magic;
	wrap(stream->readUInt32(magic))
	if (magic != FourCC('K', 'v', 'x', 'l')) {
		Log::error("Invalid kv6 magic");
		return 0;
	}

	uint32_t xsiz_w, ysiz_d, zsiz_h;
	wrap(stream->readUInt32(xsiz_w))
	wrap(stream->readUInt32(ysiz_d))
	wrap(stream->readUInt32(zsiz_h))
	glm::vec3 pivot;
	wrap(stream->readFloat(pivot.x))
	wrap(stream->readFloat(pivot.y))
	wrap(stream->readFloat(pivot.z))

	uint32_t numvoxs;
	wrap(stream->readUInt32(numvoxs))

	const int64_t headerSize = 32;
	const int64_t xLenSize = (int64_t)(xsiz_w * sizeof(uint32_t));
	const int64_t yLenSize = (int64_t)((size_t)xsiz_w * (size_t)ysiz_d * sizeof(uint16_t));
	const int64_t paletteOffset = headerSize + (int64_t)(numvoxs * 8) + xLenSize + yLenSize;
	if (stream->seek(paletteOffset) != -1) {
		if (stream->remaining() != 0) {
			uint32_t palMagic;
			wrap(stream->readUInt32(palMagic))
			if (palMagic == FourCC('S', 'P', 'a', 'l')) {
				// slab6 suggest palette
				palette.setSize(palette::PaletteMaxColors);
				for (int i = 0; i < palette::PaletteMaxColors; ++i) {
					color::RGBA color;
					wrapBool(priv::readRGBScaledColor(*stream, color))
					palette.setColor(i, color);
				}
			}
			return palette.size();
		}
	}

	// SPal not found, most likely slab5
	stream->seek(headerSize);

	palette::RGBABuffer colors;
	for (uint32_t c = 0u; c < numvoxs; ++c) {
		color::RGBA color;
		wrapBool(priv::readBGRColor(*stream, color));
		colors.put(color, true);
		wrap2(stream->skip(5));
	}

	return createPalette(colors, palette);
}

#undef wrap
#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load kv6 file: Not enough data in stream " CORE_STRINGIFY(read));                        \
		return false;                                                                                                  \
	}

bool KV6Format::loadRawVolume(const core::String &filename, const io::ArchivePtr &archive, voxel::RawVolume *&volume,
							  palette::Palette &palette, glm::vec3 &kenPivot, const LoadContext &ctx,
							  core::Buffer<KFAFormat::KenSpriteVox> *kenVoxels) {
	volume = nullptr;
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	uint32_t magic;
	wrap(stream->readUInt32(magic))
	if (magic != FourCC('K', 'v', 'x', 'l')) {
		Log::error("Invalid kv6 magic");
		return false;
	}

	// Dimensions of voxel. (our depth is kv6 height)
	uint32_t width, depth, height;
	wrap(stream->readUInt32(width))
	wrap(stream->readUInt32(depth))
	wrap(stream->readUInt32(height))

	if (width > 256 || depth > 256 || height > 255) {
		Log::error("Dimensions exceeded: w: %i, h: %i, d: %i", width, height, depth);
		return false;
	}

	glm::vec3 pivot;
	wrap(stream->readFloat(pivot.x)) // Ken x / width
	wrap(stream->readFloat(pivot.z)) // Ken y / depth stored in vengi z
	wrap(stream->readFloat(pivot.y)) // Ken z / height stored in vengi y
	kenPivot = glm::vec3(pivot.x, pivot.z, pivot.y);

	const voxel::Region region(0, 0, 0, (int)width - 1, (int)height - 1, (int)depth - 1);
	if (!region.isValid()) {
		Log::error("Invalid region: %i:%i:%i", width, height, depth);
		return false;
	}

	uint32_t numvoxs;
	wrap(stream->readUInt32(numvoxs))
	Log::debug("numvoxs: %u", numvoxs);
	if (numvoxs > priv::MAXVOXS) {
		Log::error("Max allowed voxels exceeded: %u (max is %u)", numvoxs, priv::MAXVOXS);
		return false;
	}

	const int64_t headerSize = 32;
	const int64_t xoffsetSize = (int64_t)(width * sizeof(uint32_t));
	const int64_t xyoffsetSize = (int64_t)((size_t)width * (size_t)depth * sizeof(uint16_t));
	const int64_t paletteOffset = headerSize + (int64_t)numvoxs * (int64_t)8 + xoffsetSize + xyoffsetSize;
	// palette SPal (suggested palette) added in slab6
	bool slab5 = true;
	if (stream->seek(paletteOffset) != -1) {
		if (stream->remaining() != 0) {
			uint32_t palMagic = 0u;
			wrap(stream->readUInt32(palMagic))
			if (palMagic == FourCC('S', 'P', 'a', 'l')) {
				Log::debug("Found embedded palette of slab6");
				slab5 = false; // slab6
				palette.setSize(palette::PaletteMaxColors);
				for (int i = 0; i < palette::PaletteMaxColors; ++i) {
					color::RGBA color;
					wrapBool(priv::readRGBScaledColor(*stream, color));
					palette.setColor(i, color);
				}
			}
		}
	}
	if (slab5) {
		Log::debug("Found slab5");
	} else {
		Log::debug("Found slab6");
	}
	stream->seek(headerSize);

	core::ScopedPtr<priv::State> state(new priv::State());
	palette::PaletteLookup paletteLookup(palette);
	for (uint32_t c = 0u; c < numvoxs; ++c) {
		if (c % core_max(1u, numvoxs / 100u) == 0u) {
			ctx.report("voxel", (int)c, (int)numvoxs);
		}
		color::RGBA color;
		wrapBool(priv::readBGRColor(*stream, color));
		wrap2(stream->skip(1)) // slab6 always 128
		wrap(stream->readUInt8(state->voxdata[c].z))
		wrap2(stream->skip(1)) // slab6 always 0
		wrap(stream->readUInt8((uint8_t &)state->voxdata[c].vis))
		wrap(stream->readUInt8(state->voxdata[c].normal))

		if (slab5) {
			palette.tryAdd(color, false, &state->voxdata[c].col, false);
		} else {
			state->voxdata[c].col = paletteLookup.findClosestIndex(color);
		}
		Log::debug("voxel %u/%u z: %u, vis: %i. dir: %u, pal: %u", c, numvoxs, state->voxdata[c].z,
				   (uint8_t)state->voxdata[c].vis, state->voxdata[c].normal, state->voxdata[c].col);
	}
	ctx.report("voxel", (int)numvoxs, (int)numvoxs);

	for (uint32_t x = 0u; x < width; ++x) {
		wrap(stream->readInt32(state->xoffsets[x]))
		Log::debug("xoffsets[%u]: %i", x, state->xoffsets[x]);
	}

	for (uint32_t x = 0u; x < width; ++x) {
		for (uint32_t y = 0u; y < depth; ++y) {
			wrap(stream->readUInt16(state->xyoffsets[x][y]))
			Log::debug("xyoffsets[%u][%u]: %u", x, y, state->xyoffsets[x][y]);
		}
	}

	volume = new voxel::RawVolume(region);
	if (kenVoxels != nullptr) {
		kenVoxels->clear();
		kenVoxels->reserve(numvoxs);
	}

	int idx = 0;
	// TODO: PERF: use volume sampler
	for (uint32_t x = 0; x < width; ++x) {
		for (uint32_t y = 0; y < depth; ++y) {
			for (int end = idx + state->xyoffsets[x][y]; idx < end; ++idx) {
				const priv::VoxtypeKV6 &vox = state->voxdata[idx];
				const uint8_t normalIndex = vox.normal == 255 ? NO_NORMAL : vox.normal + NORMAL_PALETTE_OFFSET;
				const voxel::Voxel col = voxel::createVoxel(palette, vox.col, normalIndex);
				volume->setVoxel((int)x, (int)((height - 1) - vox.z), (int)y, col);
				if (kenVoxels != nullptr) {
					KFAFormat::KenSpriteVox kv;
					kv.x = (uint16_t)x;
					kv.y = (uint16_t)y;
					kv.z = vox.z;
					kv.vis = (uint8_t)vox.vis;
					kenVoxels->push_back(kv);
				}
			}
		}
	}
	return true;
}

bool KV6Format::loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
								  scenegraph::SceneGraph &sceneGraph, palette::Palette &palette, const LoadContext &ctx) {
	voxel::RawVolume *volume = nullptr;
	glm::vec3 kenPivot(0.0f);
	core::Buffer<KFAFormat::KenSpriteVox> kenVoxels;
	if (!loadRawVolume(filename, archive, volume, palette, kenPivot, ctx, &kenVoxels)) {
		return false;
	}

	const core::String &basename = core::string::stripExtension(filename);
	const bool loadKfa = core::getVar(cfg::VoxformatKV6LoadKFA)->boolVal();
	if (loadKfa && archive->exists(basename + ".kfa")) {
		KFAFormat::Data kfa;
		if (KFAFormat::parse(basename + ".kfa", archive, kfa)) {
			if (KFAFormat::apply(kfa, volume, kenPivot, sceneGraph, palette, core::string::extractFilename(filename),
								 kenVoxels)) {
				delete volume;
				return true;
			}
		}
		Log::warn("Failed to apply kfa sidecar for %s - loading as a single volume", filename.c_str());
	}

	const voxel::Region &region = volume->region();
	const glm::vec3 normalizedPivot =
		glm::vec3(kenPivot.x, kenPivot.z, kenPivot.y) /
		glm::vec3(region.getWidthInVoxels(), region.getHeightInVoxels(), region.getDepthInVoxels());
	palette::NormalPalette normalPalette;
	normalPalette.slab6();
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(volume);
	node.setName(core::string::extractFilename(filename));
	scenegraph::KeyFrameIndex keyFrameIdx = 0;
	node.setPivot(normalizedPivot);
	scenegraph::SceneGraphTransform transform;
	node.setTransform(keyFrameIdx, transform);
	node.setPalette(palette);
	node.setNormalPalette(normalPalette);
	return sceneGraph.emplace(core::move(node)) != InvalidNodeId;
}

#undef wrapBool
#undef wrap

#define wrapBool(read)                                                                                                 \
	if ((read) == false) {                                                                                             \
		Log::error("Could not write kv6 file: Not enough space in stream " CORE_STRINGIFY(read));                      \
		return false;                                                                                                  \
	}

bool KV6Format::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
						   const io::ArchivePtr &archive, const SaveContext &ctx) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}
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

	uint8_t normalRemap[palette::NormalPaletteMaxNormals];
	if (node->hasNormalPalette()) {
		palette::NormalPalette target;
		target.slab6();
		node->normalPalette().createRemap(target, normalRemap);
	}

	constexpr uint32_t MAXVOXS = 64 * 64 * 256;
	core::Buffer<priv::VoxtypeKV6> voxdata;
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
			if (!node->hasNormalPalette() || voxel.getNormal() == NO_NORMAL) {
				vd.normal = priv::calculateDir(node->volume(), x, y, z, voxel);
			} else {
				vd.normal = normalRemap[voxel.getNormal() - NORMAL_PALETTE_OFFSET];
			}
			voxdata.push_back(vd);
		},
		voxelutil::VisitorOrder::XZmY);

	if (numvoxs > MAXVOXS) {
		Log::error("Max allowed voxels exceeded: %u (max is %u)", numvoxs, MAXVOXS);
		return false;
	}

	wrapBool(stream->writeUInt32(FourCC('K', 'v', 'x', 'l')))

	const int xsiz_w = dim.x;
	// flip y and z here
	const int ysiz_d = dim.z;
	const int zsiz_h = dim.y;
	wrapBool(stream->writeUInt32(xsiz_w))
	wrapBool(stream->writeUInt32(ysiz_d))
	wrapBool(stream->writeUInt32(zsiz_h))

	// KV6 only stores a pivot. Bake node translation into it so a scene-mode
	// move (e.g. raising a model off the waterline) survives save/reload.
	const glm::vec3 nodePivot =
		priv::bakedNormalizedPivot(node->pivot(), priv::nodeTranslation(sceneGraph, *node), dim);
	const glm::vec3 pivot = nodePivot * glm::vec3(region.getDimensionsInVoxels());
	wrapBool(stream->writeFloat(pivot.x))
	wrapBool(stream->writeFloat(pivot.z))
	wrapBool(stream->writeFloat(pivot.y))

	wrapBool(stream->writeUInt32(numvoxs))

	for (const priv::VoxtypeKV6 &data : voxdata) {
		const color::RGBA color = node->palette().color(data.col);
		wrapBool(priv::writeBGRColor(*stream, color)) // range 0.255
		wrapBool(stream->writeUInt8(128))			 // 128 as we save slab6
		wrapBool(stream->writeUInt8(data.z))
		wrapBool(stream->writeUInt8(0)) // 0 as we save slab6
		wrapBool(stream->writeUInt8((uint8_t)data.vis))
		wrapBool(stream->writeUInt8(data.normal))
		Log::debug("voxel z-low: %u, vis: %i. dir: %u, pal: %u", data.z, (uint8_t)data.vis, data.normal, data.col);
	}

	for (int x = 0u; x < xsiz_w; ++x) {
		wrapBool(stream->writeInt32(xoffsets[x]))
		Log::debug("xoffsets[%u]: %i", x, xoffsets[x]);
	}

	for (int x = 0; x < xsiz_w; ++x) {
		for (int y = 0; y < ysiz_d; ++y) {
			wrapBool(stream->writeUInt16(xyoffsets[x][y]))
			Log::debug("xyoffsets[%u][%u]: %u", x, y, xyoffsets[x][y]);
		}
	}

	const uint32_t palMagic = FourCC('S', 'P', 'a', 'l');
	wrapBool(stream->writeUInt32(palMagic))
	for (int i = 0; i < node->palette().colorCount(); ++i) {
		const color::RGBA color = node->palette().color(i);
		wrapBool(priv::writeRGBScaledColor(*stream, color)) // range 0..63
	}
	for (int i = node->palette().colorCount(); i < palette::PaletteMaxColors; ++i) {
		color::RGBA color(0);
		wrapBool(priv::writeRGBScaledColor(*stream, color))
	}

	return true;
}

#undef wrap2
#undef wrapBool

} // namespace voxelformat
