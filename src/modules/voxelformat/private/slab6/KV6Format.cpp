/**
 * @file
 */

#include "KV6Format.h"
#include "SLABShared.h"
#include "app/App.h"
#include "core/Color.h"
#include "core/Common.h"
#include "core/Enum.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/RGBA.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/Vector.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "io/Stream.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/Face.h"
#include "voxel/Palette.h"
#include "voxel/RawVolume.h"
#include "voxelutil/VolumeSplitter.h"
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
	// These are the hinge angles. 0 is 0 degrees, 16384 is 90 degrees, -16384 is -90 degrees
	core::Buffer<core::Buffer<int16_t>> frmval; //[numfrm][numhin]
	core::Buffer<KFASeqTyp> seq;				//[seqnum]
};

// lighting value that distributes above a radius of 3 around the position
static uint8_t calculateDir(const voxel::RawVolume *, int, int, int, const voxel::Voxel &) {
	return 0u; // TODO
}

const uint32_t MAXSPRITES = 1024;

} // namespace priv

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load kv6 file: Not enough data in stream " CORE_STRINGIFY(read));                        \
		return 0;                                                                                                      \
	}

#define wrapBool(read)                                                                                                 \
	if ((read) == false) {                                                                                             \
		Log::error("Could not load kv6 file: Not enough space in stream " CORE_STRINGIFY(read));                       \
		return false;                                                                                                  \
	}

bool KV6Format::readColor(io::SeekableReadStream &stream, core::RGBA &color) const {
	uint8_t r, g, b;
	wrap(stream.readUInt8(b))
	wrap(stream.readUInt8(g))
	wrap(stream.readUInt8(r))
	color = core::RGBA(r, g, b);
	return true;
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
		core::RGBA color;
		wrap(readColor(stream, color))
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

bool KV6Format::loadKFA(const core::String &filename, const voxel::RawVolume *volume,
						scenegraph::SceneGraph &sceneGraph, const voxel::Palette &palette) {
	const io::FilesystemPtr &filesystem = io::filesystem();
	const io::FilePtr &kfaFile = filesystem->open(filename);
	if (!kfaFile->validHandle()) {
		// if there is no hva file, we still don't show an error
		return false;
	}
	io::FileStream stream(kfaFile);
	uint32_t magic;
	wrap(stream.readUInt32(magic))
	if (magic != FourCC('K', 'w', 'l', 'k')) {
		Log::error("Invalid magic number");
		return false;
	}
	core::String kv6Name;
	wrapBool(stream.readPascalStringUInt32LE(kv6Name))
	Log::debug("kv6Name: %s", kv6Name.c_str());

	priv::KFAData kfa;
	uint32_t numHinge;
	wrap(stream.readUInt32(numHinge))
	if (numHinge >= priv::MAXSPRITES) {
		Log::error("Max allowed hinges exceeded: %u (max is %u)", numHinge, priv::MAXSPRITES);
		return false;
	}
	Log::debug("numhinge: %u", numHinge);
	kfa.hinge.reserve(numHinge);
	for (uint32_t i = 0; i < numHinge; ++i) {
		priv::KFAHinge hinge;
		hinge.id = i;
		wrap(stream.readInt32(hinge.parent))
		for (int n = 0; n < 2; ++n) {
			wrap(stream.readFloat(hinge.p[n].x))
			wrap(stream.readFloat(hinge.p[n].z))
			wrap(stream.readFloat(hinge.p[n].y))
		}
		for (int n = 0; n < 2; ++n) {
			wrap(stream.readFloat(hinge.v[n].x))
			wrap(stream.readFloat(hinge.v[n].z))
			wrap(stream.readFloat(hinge.v[n].y))
		}
		wrap(stream.readInt16(hinge.vmin))
		wrap(stream.readInt16(hinge.vmax))
		wrap(stream.readInt8(hinge.type))
		wrap(stream.skip(7))
		kfa.hinge.push_back(hinge);
	}
	uint32_t numFrames;
	wrap(stream.readUInt32(numFrames))
	Log::debug("numfrm: %u", numFrames);
	kfa.frmval.resize(numFrames);
	for (uint32_t i = 0; i < numFrames; ++i) {
		kfa.frmval[i].reserve(numHinge);
		for (uint32_t j = 0; j < numHinge; ++j) {
			int16_t angle;
			wrap(stream.readInt16(angle))
			kfa.frmval[i].push_back(angle);
		}
	}
	uint32_t numSequences;
	wrap(stream.readUInt32(numSequences))
	Log::debug("numseq: %u", numSequences);
	kfa.seq.reserve(numSequences);
	for (uint32_t i = 0; i < numSequences; ++i) {
		priv::KFASeqTyp seq;
		wrap(stream.readInt32(seq.time))
		wrap(stream.readInt32(seq.frame))
		kfa.seq.push_back(seq);
	}

	core::DynamicArray<voxel::RawVolume *> volumes;
	// TODO: the order here matters for the references in the kfa structs
	voxelutil::splitObjects(volume, volumes, voxelutil::VisitorOrder::XYZ);
	if (volumes.empty()) {
		Log::error("Could not split volume into single objects");
		return false;
	}

	Log::debug("Split into %i objects", (int)volumes.size());
	for (const voxel::RawVolume *v : volumes) {
		const voxel::Region &region = v->region();
		Log::debug("region: %s", region.toString().c_str());
	}
	if (kfa.hinge.size() > volumes.size() + 1) {
		Log::error("kfa hinge count doesn't match kv6 objects");
		return false;
	}

	core::sort(kfa.hinge.begin(), kfa.hinge.end(),
			   [](const priv::KFAHinge &a, const priv::KFAHinge &b) { return a.parent < b.parent; });

	for (const priv::KFAHinge &hinge : kfa.hinge) {
		Log::debug("id: %i, parent: %i", hinge.id, hinge.parent);
	}

	for (voxel::RawVolume *v : volumes) {
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		scenegraph::SceneGraphTransform transform;
		const uint32_t fps = 20; // TODO fps?
		const uint32_t div = 1000 / fps;
		for (const priv::KFASeqTyp &seq : kfa.seq) {
			scenegraph::KeyFrameIndex keyFrameIdx = node.addKeyFrame(seq.time / div);
			if (keyFrameIdx == InvalidKeyFrame) {
				Log::error("Failed to load keyframe %i", seq.time);
				return false;
			}
			scenegraph::SceneGraphKeyFrame &keyFrame = node.keyFrame(keyFrameIdx);
			scenegraph::SceneGraphTransform &transform = keyFrame.transform();
			// TODO: implement keyframe loading
			(void)transform;
		}
		node.setVolume(v, true);
		node.setName(filename);
		node.setPalette(palette);
		// TODO: proper parenting
		int parent = 0;
		if (sceneGraph.emplace(core::move(node), parent) == InvalidNodeId) {
			Log::error("Failed to add node to scene graph");
			return false;
		}
	}

	return true;
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
					core::RGBA color;
					wrapBool(readColor(stream, color));
					palette.color(i) = color;
				}
			}
		}
	}
	stream.seek(headerSize);

	core::ScopedPtr<priv::State> state(new priv::State());
	for (uint32_t c = 0u; c < numvoxs; ++c) {
		core::RGBA color;
		wrapBool(readColor(stream, color));
		wrap(stream.skip(1))
		wrap(stream.readUInt8(state->voxdata[c].z))
		wrap(stream.skip(1))
		wrap(stream.readUInt8((uint8_t &)state->voxdata[c].vis))
		wrap(stream.readUInt8(state->voxdata[c].dir))

		palette.addColorToPalette(color, false, &state->voxdata[c].col);
		Log::debug("voxel %u/%u z: %u, vis: %i. dir: %u, pal: %u", c, numvoxs, state->voxdata[c].z,
				   (uint8_t)state->voxdata[c].vis, state->voxdata[c].dir, state->voxdata[c].col);
	}

	for (uint32_t x = 0u; x < width; ++x) {
		wrap(stream.readInt32(state->xoffsets[x]))
		Log::debug("xoffsets[%u]: %i", x, state->xoffsets[x]);
	}

	for (uint32_t x = 0u; x < width; ++x) {
		for (uint32_t y = 0u; y < depth; ++y) {
			wrap(stream.readUInt16(state->xyoffsets[x][y]))
			Log::debug("xyoffsets[%u][%u]: %u", x, y, state->xyoffsets[x][y]);
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

	const core::String &basename = core::string::stripExtension(filename);
	if (loadKFA(basename + ".kfa", volume, sceneGraph, palette)) {
		delete volume;
		return true;
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(volume, true);
	node.setName(filename);
	scenegraph::KeyFrameIndex keyFrameIdx = 0;
	node.setPivot(normalizedPivot);
	scenegraph::SceneGraphTransform transform;
	node.setTransform(keyFrameIdx, transform);
	node.setPalette(palette);
	return sceneGraph.emplace(core::move(node)) != InvalidNodeId;
}

#undef wrapBool
#undef wrap

#define wrapBool(read)                                                                                                 \
	if ((read) == false) {                                                                                             \
		Log::error("Could not write kv6 file: Not enough space in stream " CORE_STRINGIFY(read));                      \
		return false;                                                                                                  \
	}

bool KV6Format::writeColor(io::SeekableWriteStream &stream, core::RGBA color) const {
	wrapBool(stream.writeUInt8(color.b))
	wrapBool(stream.writeUInt8(color.g))
	wrapBool(stream.writeUInt8(color.r))
	return true;
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
		voxelutil::VisitorOrder::XZmY);

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
		wrapBool(writeColor(stream, color))
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
		wrapBool(writeColor(stream, color))
	}
	for (int i = node->palette().colorCount(); i < voxel::PaletteMaxColors; ++i) {
		core::RGBA color(0);
		wrapBool(writeColor(stream, color))
	}

	return true;
}

#undef wrapBool

} // namespace voxelformat
