/**
 * @file
 */

#include "KV6Format.h"
#include "SLABShared.h"
#include "core/Common.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "color/RGBA.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "io/Archive.h"
#include "io/Stream.h"
#include "palette/NormalPalette.h"
#include "palette/PaletteLookup.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "palette/Palette.h"
#include "voxel/Connectivity.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelformat/Format.h"
#include "voxelutil/VolumeSplitter.h"
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

struct KFAPoint3d {
	float x;
	float y;
	float z;
};

// Each hinge is a 1-D axis of rotation
struct KFAHinge {
	int32_t id;		// index to this sprite
	int32_t parent; // index to parent sprite (-1=none)
	// if parent is -1 the p[0] values are the mins of the object - the pivot
	KFAPoint3d p[2]; // "velcro" point of each object [0 = child (self), 1 = parent]
	// given vector v[0], returns a and b that makes (v[0], a, b) orthonormal
	// r1.x = a.x * cos(frmval) + b.x * sin(frmval);
	// r1.y = a.y * cos(frmval) + b.y * sin(frmval);
	// r1.z = a.z * cos(frmval) + b.z * sin(frmval);
	// r2.x = a.x * sin(frmval) - b.x * cos(frmval);
	// r2.y = a.y * sin(frmval) - b.y * cos(frmval);
	// r2.z = a.z * sin(frmval) - b.z * cos(frmval);
	KFAPoint3d v[2]; // axis vector of rotation for each object [0 = child (self), 1 = parent]
	int16_t vmin;	 // min value
	int16_t vmax;	 // max value
	int8_t type;	 // 0 == rotate
	int8_t filler[7]{};
};

struct KFASeqTyp {
	int32_t time;
	int32_t frame;
};

struct KFAData {
	core::Buffer<KFAHinge> hinge; //[numhinge]
	// These are the hinge euler angles.
	core::DynamicArray<core::Buffer<float>> frmval; //[numfrm][numhin]
	core::Buffer<KFASeqTyp> seq;				//[seqnum]
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

const uint32_t MAXSPRITES = 1024;

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
					core::RGBA color;
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
		core::RGBA color;
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

bool KV6Format::loadKFA(const core::String &filename, const io::ArchivePtr &archive, const voxel::RawVolume *volume,
						scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	uint32_t magic;
	wrap(stream->readUInt32(magic))
	if (magic != FourCC('K', 'w', 'l', 'k')) {
		Log::error("Invalid kwalk magic number");
		return false;
	}
	core::String kv6Name;
	wrapBool(stream->readPascalStringUInt32LE(kv6Name))
	Log::debug("kv6Name: %s", kv6Name.c_str());

	priv::KFAData kfa;
	uint32_t numHinge;
	wrap(stream->readUInt32(numHinge))
	if (numHinge >= priv::MAXSPRITES) {
		Log::error("Max allowed hinges exceeded: %u (max is %u)", numHinge, priv::MAXSPRITES);
		return false;
	}
	Log::debug("numhinge: %u", numHinge);
	kfa.hinge.reserve(numHinge);
	for (uint32_t i = 0; i < numHinge; ++i) {
		priv::KFAHinge hinge;
		hinge.id = i;
		wrap(stream->readInt32(hinge.parent))
		for (int n = 0; n < 2; ++n) {
			wrap(stream->readFloat(hinge.p[n].x))
			wrap(stream->readFloat(hinge.p[n].z))
			wrap(stream->readFloat(hinge.p[n].y))
		}
		for (int n = 0; n < 2; ++n) {
			wrap(stream->readFloat(hinge.v[n].x))
			wrap(stream->readFloat(hinge.v[n].z))
			wrap(stream->readFloat(hinge.v[n].y))
		}
		wrap(stream->readInt16(hinge.vmin))
		wrap(stream->readInt16(hinge.vmax))
		wrap(stream->readInt8(hinge.type))
		wrap2(stream->skip(7))
		kfa.hinge.push_back(hinge);
	}
	uint32_t numFrames;
	wrap(stream->readUInt32(numFrames))
	Log::debug("numfrm: %u", numFrames);
	if (numFrames > 65536) {
		Log::error("Max allowed frames exceeded: %u (max is %u)", numFrames, 65536);
		return false;
	}
	kfa.frmval.resize(numFrames);
	for (uint32_t i = 0; i < numFrames; ++i) {
		kfa.frmval[i].reserve(numHinge);
		for (uint32_t j = 0; j < numHinge; ++j) {
			// 0 is 0 degrees, 16384 is 90 degrees, -16384 is -90 degrees
			int16_t angle;
			wrap(stream->readInt16(angle))
			kfa.frmval[i].push_back((float)angle / 65536.0f);
		}
	}
	uint32_t numSequences;
	wrap(stream->readUInt32(numSequences))
	Log::debug("numseq: %u", numSequences);
	kfa.seq.reserve(numSequences);
	for (uint32_t i = 0; i < numSequences; ++i) {
		priv::KFASeqTyp seq;
		wrap(stream->readInt32(seq.time))
		wrap(stream->readInt32(seq.frame))
		kfa.seq.push_back(seq);
	}

	// TODO: VOXELFORMAT: the order here matters for the references in the kfa structs
	core::Buffer<voxel::RawVolume *> volumes =
		voxelutil::splitObjects(volume, voxelutil::VisitorOrder::XYZ, voxel::Connectivity::EighteenConnected);
	if (volumes.empty()) {
		Log::error("Could not split volume into single objects");
		return false;
	}

	Log::debug("Split into %i objects", (int)volumes.size());

	// In slab6, numspr can be <= numhin, but we can now create transform-only nodes for extra hinges
	if (kfa.hinge.size() > volumes.size()) {
		Log::debug("More hinges (%d) than volumes (%d) - creating transform-only nodes for extra hinges",
				  (int)kfa.hinge.size(), (int)volumes.size());
	}

	// Create sorting array to ensure proper parent-child ordering
	// Simple topological sort: process nodes with no unprocessed dependencies first
	core::Buffer<int32_t> hingeSort;
	hingeSort.reserve(kfa.hinge.size());
	core::Buffer<bool> processed(kfa.hinge.size(), 0);

	// First, add all root hinges (parent=-1)
	for (uint32_t i = 0; i < kfa.hinge.size(); ++i) {
		if (kfa.hinge[i].parent < 0) {
			hingeSort.push_back(i);
			processed[i] = true;
		}
	}

	// Then iteratively add hinges whose parents have been processed
	bool addedAny = true;
	while (addedAny) {
		addedAny = false;
		for (uint32_t i = 0; i < kfa.hinge.size(); ++i) {
			if (processed[i]) {
				continue;
			}
			const int32_t parentIdx = kfa.hinge[i].parent;
			if (parentIdx >= 0 && parentIdx < (int32_t)kfa.hinge.size() && processed[parentIdx]) {
				// Parent has been processed, so we can process this hinge
				hingeSort.push_back(i);
				processed[i] = true;
				addedAny = true;
			}
		}
	}

	// Add any remaining hinges that couldn't be sorted (broken parent references)
	for (uint32_t i = 0; i < kfa.hinge.size(); ++i) {
		if (!processed[i]) {
			Log::warn("Hinge %d has invalid parent reference, adding to end", i);
			hingeSort.push_back(i);
		}
	}

	// Map to track node IDs for parent-child relationships
	// This maps hinge ID (not index) to node ID
	core::Buffer<int> nodeIdsByHingeId(kfa.hinge.size(), InvalidNodeId);

	// Process ALL hinges in sorted order to ensure parents are created before children
	// Create nodes directly in the scene graph, handling parent relationships properly
	// In slab6, some hinges may not have corresponding volumes - create transform-only nodes for those
	uint32_t volumeIdx = 0;
	for (size_t sortIdx = 0; sortIdx < hingeSort.size(); ++sortIdx) {
		const uint32_t hingeIdx = hingeSort[sortIdx];
		const priv::KFAHinge &hinge = kfa.hinge[hingeIdx];

		// Use available volume if we have one, otherwise create a transform-only node
		voxel::RawVolume *v = nullptr;
		if (volumeIdx < volumes.size()) {
			v = volumes[volumeIdx];
			volumeIdx++; // Only increment when we actually use a volume
		}

		// Create pivot from hinge data - for root nodes, p[0] contains the position
		glm::vec3 pivot;
		if (hinge.parent < 0) {
			// For root nodes, slab6 stores the negative position in p[0]
			pivot.x = -hinge.p[0].x;
			pivot.y = -hinge.p[0].z; // swap y and z here
			pivot.z = -hinge.p[0].y;
		} else {
			pivot.x = hinge.p[0].x;
			pivot.y = hinge.p[0].z; // swap y and z here
			pivot.z = hinge.p[0].y;
		}

		scenegraph::SceneGraphNode node(v != nullptr ? scenegraph::SceneGraphNodeType::Model
													 : scenegraph::SceneGraphNodeType::Group);
		node.setPivot(pivot);
		core::String name;
		const core::String &basename = core::string::extractFilename(filename);
		if (v != nullptr) {
			node.setVolume(v, true);
			node.setPalette(palette);
			name = core::String::format("%s_hinge_%d", basename.c_str(), hinge.id);
		} else {
			name = core::String::format("%s_hinge_%d_transform", basename.c_str(), hinge.id);
		}
		node.setName(name);

		// Add keyframes based on sequence data
		// TODO: fix key frame idx
		scenegraph::KeyFrameIndex keyFrameIdx = 0;
		scenegraph::SceneGraphTransform transform;

#if 0
		if (v != nullptr) {
			// Get the volume's region and calculate offset from original volume
			const voxel::Region &volumeRegion = v->region();
			const voxel::Region &originalRegion = volume->region();

			// Calculate translation offset like slab6 does in spr[].p
			// In slab6: spr[numspr].p.x = x0-kv->xpiv; (where x0 is the min bound of the split volume)
			// Calculate the offset of this volume from the original volume's origin
			// Volume regions are already in vengi coordinate system, so no axis swap needed
			const glm::vec3 translation = volumeRegion.getLowerCornerf() - originalRegion.getLowerCornerf();

			// Apply the translation offset
			transform.setLocalTranslation(translation);
			v->region().shift(-translation);
		}
#endif
		// Set default rotation based on sequence data
		if (!kfa.seq.empty() && !kfa.frmval.empty()) {
			if (hinge.type == 0) { // rotation
				// Try to use the first valid frame for default pose
				for (const priv::KFASeqTyp &seq : kfa.seq) {
					int32_t actualFrame = seq.frame;
					if (actualFrame < 0) {
						actualFrame = ~actualFrame;
					}
					if (actualFrame >= 0 && actualFrame < (int32_t)kfa.frmval.size() &&
						hingeIdx < kfa.frmval[actualFrame].size()) {
						const glm::vec3 axis(hinge.v[0].x, hinge.v[0].z, hinge.v[0].y); // Convert slab6 coord to vengi
						const float frameValue = kfa.frmval[actualFrame][hingeIdx];
						const float angle = frameValue * glm::two_pi<float>();
						if (glm::length(axis) > 0.001f) {
							const glm::quat rotation = glm::angleAxis(angle, glm::normalize(axis));
							transform.setLocalOrientation(rotation);
						}
						break; // Use first valid frame as default pose
					}
				}
			} else {
				Log::warn("Unhandled hinge type: %i", hinge.type);
			}
		}

		// Set the transform on the node
		node.setTransform(keyFrameIdx, transform);

		// Determine parent node ID
		int parentNodeId = 0; // Default to root
		if (hinge.parent >= 0 && hinge.parent < (int32_t)nodeIdsByHingeId.size()) {
			parentNodeId = nodeIdsByHingeId[hinge.parent];
			if (parentNodeId == InvalidNodeId) {
				// Parent hasn't been created yet (shouldn't happen with proper sorting), use root
				Log::warn("Parent node not found for hinge %d (parent %d), using root", hinge.id, hinge.parent);
				parentNodeId = 0;
			}
		}

		// Add node to scene graph directly
		int nodeId = sceneGraph.emplace(core::move(node), parentNodeId);
		if (nodeId == InvalidNodeId) {
			Log::error("Failed to add node for hinge %d to scene graph", hinge.id);
			return false;
		}

		// Store node ID by hinge ID for parent-child relationships
		if (hinge.id >= 0 && hinge.id < (int32_t)nodeIdsByHingeId.size()) {
			nodeIdsByHingeId[hinge.id] = nodeId;
		}
	}
	return true;
}

bool KV6Format::loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
								  scenegraph::SceneGraph &sceneGraph, palette::Palette &palette, const LoadContext &ctx) {
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
	wrap(stream->readFloat(pivot.x)) // width
	wrap(stream->readFloat(pivot.z)) // depth
	wrap(stream->readFloat(pivot.y)) // height

	glm::vec3 normalizedPivot = pivot / glm::vec3(width, height, depth);

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
					core::RGBA color;
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
		core::RGBA color;
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

	voxel::RawVolume *volume = new voxel::RawVolume(region);

	int idx = 0;
	// TODO: PERF: use volume sampler
	for (uint32_t x = 0; x < width; ++x) {
		for (uint32_t y = 0; y < depth; ++y) {
			for (int end = idx + state->xyoffsets[x][y]; idx < end; ++idx) {
				const priv::VoxtypeKV6 &vox = state->voxdata[idx];
				const voxel::Voxel col = voxel::createVoxel(palette, vox.col, vox.normal);
				volume->setVoxel((int)x, (int)((height - 1) - vox.z), (int)y, col);
			}
		}
	}

	const core::String &basename = core::string::stripExtension(filename);
	if (archive->exists(basename + ".kfa")) {
		if (loadKFA(basename + ".kfa", archive, volume, sceneGraph, palette)) {
			delete volume;
			return true;
		}
	}
	palette::NormalPalette normalPalette;
	normalPalette.slab6();
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(volume, true);
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
				vd.normal = voxel.getNormal();
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

	glm::vec3 pivot = node->pivot() * glm::vec3(region.getDimensionsInVoxels());
	wrapBool(stream->writeFloat(pivot.x))
	wrapBool(stream->writeFloat(pivot.z))
	wrapBool(stream->writeFloat(pivot.y))

	wrapBool(stream->writeUInt32(numvoxs))

	for (const priv::VoxtypeKV6 &data : voxdata) {
		const core::RGBA color = node->palette().color(data.col);
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
		const core::RGBA color = node->palette().color(i);
		wrapBool(priv::writeRGBScaledColor(*stream, color)) // range 0..63
	}
	for (int i = node->palette().colorCount(); i < palette::PaletteMaxColors; ++i) {
		core::RGBA color(0);
		wrapBool(priv::writeRGBScaledColor(*stream, color))
	}

	return true;
}

#undef wrap2
#undef wrapBool

} // namespace voxelformat
