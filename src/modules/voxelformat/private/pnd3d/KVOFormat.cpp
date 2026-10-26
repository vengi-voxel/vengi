/**
 * @file
 */

#include "KVOFormat.h"
#include "core/FourCC.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/collection/Buffer.h"
#include "io/Stream.h"
#include "palette/NormalPalette.h"
#include "palette/Palette.h"
#include "palette/PaletteLookup.h"
#include "scenegraph/SceneGraph.h"
#include "voxel/RawVolume.h"
#include "voxelformat/private/slab6/SLABShared.h"
#include <math.h>

namespace voxelformat {
namespace {

// KVO packs fields least significant bit first, including fields crossing byte boundaries.
class Bits {
private:
	io::SeekableReadStream &_stream;
	uint8_t _byte = 0;
	int _remaining = 0;

public:
	explicit Bits(io::SeekableReadStream &stream) : _stream(stream) {
	}
	bool read(int count, uint32_t &value) {
		value = 0;
		for (int i = 0; i < count; ++i) {
			if (_remaining == 0) {
				if (_stream.readUInt8(_byte) != 0) {
					return false;
				}
				_remaining = 8;
			}
			value |= (uint32_t)(_byte & 1) << i;
			_byte >>= 1;
			--_remaining;
		}
		return true;
	}
};

// Solid-mask codebooks from the PND3D KVO format.
static const uint8_t hsollut[116] = //log2(n),0,..,n-1
{
	0,0,
	1,0,1, 1,0,2, 1,0,3, 1,0,4, 1,0,5, 1,0,7, 1,0,8, 1,0,10, 1,0,11, 1,0,12, 1,0,13,
	1,0,14, 1,0,16, 1,0,17, 1,0,19, 1,0,21, 1,0,23, 1,0,32, 1,0,34, 1,0,35, 1,0,42, 1,0,43,
	2,0,1,8,9, 2,0,1,32,33, 2,0,2,4,6, 2,0,2,16,18, 2,0,4,16,20, 2,0,8,32,40,
	3,0,1,8,9,32,33,40,41, 3,0,2,4,6,16,18,20,22,
};
static const uint8_t hsolind[256] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 2, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 5, 0, 0,
	0, 0, 0, 0, 0, 0, 2, 0, 0, 5, 0, 0, 8, 5, 2, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0,11, 0,11, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 2, 0, 0,11,14,11, 0, 0, 2, 0,
	0,38, 0,38,0,38,41,38,50,107,47,88,44,83,41,38,
	0, 0, 0, 0, 0, 0, 2, 0,17,78,14,11, 8, 5, 2, 0,
	0, 0, 0, 0, 0, 0,20,20, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0,53,53,65,62,98,93, 0,56,53,53,59,56,73,53,
	0, 0, 0, 0, 0,23,20,20, 0, 5, 0, 0, 0, 5, 0, 0,
	0, 0, 0, 0,26,23,68,20, 0, 5, 0, 0, 8, 5, 2, 0,
	0, 0, 0,29, 0, 0,20,20, 0,11, 0,11, 0, 0, 0, 0,
	0, 0,32,29, 0, 0,68,20, 0,11,14,11, 0, 0, 2, 0,
	0,35, 0,29, 0,23,20,20, 0,78, 0,11, 0, 5, 0, 0,
	0, 0, 0, 0, 0, 0, 2, 0, 0, 5, 0, 0, 0, 0, 0, 0,
};

uint8_t importNormal(palette::NormalPalette &palette, glm::vec3 normal) {
	// KVO uses a downward Y axis, just like its voxel coordinates.
	normal.y = -normal.y;
	if (glm::dot(normal, normal) <= 0.000001f) {
		return NO_NORMAL;
	}
	const color::RGBA encoded = palette::NormalPalette::toRGBA(glm::normalize(normal));
	for (size_t i = 0; i < palette.size(); ++i) {
		if (palette.normal(i) == encoded) {
			return i + NORMAL_PALETTE_OFFSET;
		}
	}
	if (palette.size() < palette::NormalPaletteMaxNormals) {
		const uint8_t index = (uint8_t)palette.size();
		palette.setNormal(index, encoded);
		return index + NORMAL_PALETTE_OFFSET;
	}
	return palette.getClosestMatch(normal) + NORMAL_PALETTE_OFFSET;
}

struct Node {
	glm::ivec3 pos{0};
	int size = 0;
	uint8_t chi = 0;
	uint8_t sol = 0;
};

glm::ivec3 childPos(const glm::ivec3 &pos, int child, int size) {
	return pos + glm::ivec3(child & 1, (child >> 1) & 1, (child >> 2) & 1) * size;
}

// Surface colors are stored in depth-first octree order (x, y, z child bits).
void surfaces(const voxel::RawVolume &volume, const glm::ivec3 &pos, int size, int side,
			  uint8_t wrap, uint8_t solid, core::Buffer<glm::ivec3> &positions) {
	if (size > 1) {
		for (int i = 0; i < 8; ++i) {
			surfaces(volume, childPos(pos, i, size / 2), size / 2, side, wrap, solid, positions);
		}
		return;
	}
	if (voxel::isAir(volume.voxel(pos.x, side - 1 - pos.y, pos.z).getMaterial())) {
		return;
	}
	for (int axis = 0; axis < 3; ++axis) {
		for (int direction = -1; direction <= 1; direction += 2) {
			glm::ivec3 p = pos;
			p[axis] += direction;
			if (p[axis] < 0 || p[axis] >= side) {
				const int bit = axis + (direction > 0 ? 3 : 0);
				if (wrap & (1 << bit)) {
					p[axis] = (p[axis] + side) % side;
				} else if (solid & (1 << bit)) {
					continue;
				} else {
					positions.push_back(pos);
					return;
				}
			}
			if (voxel::isAir(volume.voxel(p.x, side - 1 - p.y, p.z).getMaterial())) {
				positions.push_back(pos);
				return;
			}
		}
	}
}

} // namespace

bool KVOFormat::loadGroups(const core::String &filename, const io::ArchivePtr &archive,
						   scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		return false;
	}
	uint32_t magic, prenodes, surfaceCount, nodeCount;
	uint8_t version, depth, wrap, solid, normalBits, reserved;
	uint16_t flags;
	if (stream->readUInt32(magic) != 0 || magic != FourCC('K', 'V', 'O', 'x') || stream->readUInt8(version) != 0 ||
		version != 1 || stream->readUInt8(depth) != 0 || depth < 1 || depth > 12 || stream->readUInt16(flags) != 0 ||
		(flags & ~0x1bf) != 0 || (flags & 384) == 384 || ((flags >> 2) & 3) == 3 || stream->readUInt8(wrap) != 0 ||
		stream->readUInt8(solid) != 0 || stream->readUInt8(normalBits) != 0 || normalBits > 24 ||
		stream->readUInt8(reserved) != 0) {
		return false;
	}
	glm::vec3 pivot;
	if (stream->readFloat(pivot.x) != 0 || stream->readFloat(pivot.y) != 0 || stream->readFloat(pivot.z) != 0 ||
		stream->skip(40) < 0 || stream->readUInt32(prenodes) != 0 || stream->readUInt32(surfaceCount) != 0 ||
		stream->readUInt32(nodeCount) != 0 || prenodes > (uint64_t)(stream->size() - stream->pos())) {
		return false;
	}
	const int side = 1 << depth;
	const voxel::Region region(0, 0, 0, side - 1, side - 1, side - 1);
	if (!checkValidRegion(region)) {
		return false;
	}
	core::Buffer<Node> nodes;
	nodes.push_back(Node{glm::ivec3(0), side / 2, 0, 0});
	for (uint32_t i = 0; i < prenodes; ++i) {
		if (i >= nodes.size() || nodes[i].size <= 1 || stream->readUInt8(nodes[i].chi) != 0) {
			return false;
		}
		const Node parent = nodes[i];
		for (int child = 0; child < 8; ++child) {
			if (parent.chi & (1 << child)) {
				nodes.push_back(Node{childPos(parent.pos, child, parent.size), parent.size / 2, 0, 0});
			}
		}
	}
	// All non-leaf nodes must be present in the breadth-first table.
	for (size_t i = prenodes; i < nodes.size(); ++i) {
		if (nodes[i].size != 1) {
			return false;
		}
	}
	const int mode = (flags >> 7) & 3;
	if (mode != 0) {
		for (size_t i = prenodes; i < nodes.size(); ++i) {
			if (stream->readUInt8(nodes[i].chi) != 0) {
				return false;
			}
			nodes[i].sol = nodes[i].chi;
		}
	}
	Bits topology(*stream);
	for (size_t n = 0; n < nodes.size(); ++n) {
		Node &node = nodes[n];
		if (mode == 1) {
			continue;
		}
		if (mode == 2) {
			uint32_t value;
			if (n < prenodes) {
				if (!topology.read(8, value)) {
					return false;
				}
			} else {
				const int offset = hsolind[node.chi];
				const int bits = hsollut[offset];
				if (!topology.read(bits + 1, value)) {
					return false;
				}
				const int count = 1 << bits;
				value = value < (uint32_t)count ? hsollut[offset + 1 + value] :
					(hsollut[offset + 1 + (value ^ (2 * count - 1))] ^ 255);
			}
			node.sol = n < prenodes ? value : value | node.chi;
			continue;
		}
		for (int child = 0; child < 8; ++child) {
			if (!(node.chi & (1 << child))) {
				uint32_t bit;
				if (!topology.read(1, bit)) {
					return false;
				}
				node.sol |= bit << child;
			}
		}
	}
	core::Buffer<color::RGBA> colors;
	int paletteBits = 1;
	if (!(flags & 2)) {
		uint16_t count;
		if (stream->readUInt16(count) != 0 || count == 0 || count > 4096) {
			return false;
		}
		while ((1 << paletteBits) < count) {
			++paletteBits;
		}
		for (int i = 0; i < count; ++i) {
			color::RGBA c(0, 0, 0, 255);
			if (stream->readUInt8(c.b) != 0 || stream->readUInt8(c.g) != 0 || stream->readUInt8(c.r) != 0) {
				return false;
			}
			colors.push_back(c);
		}
	}
	scenegraph::SceneGraphNode model(scenegraph::SceneGraphNodeType::Model);
	model.createVolume(region);
	voxel::RawVolume &volume = *model.volume();
	const voxel::Voxel filler = voxel::createVoxel(voxel::VoxelType::Generic, 0);
	for (const Node &node : nodes) {
		for (int child = 0; child < 8; ++child) {
			if (!(node.sol & (1 << child))) {
				continue;
			}
			const glm::ivec3 p = childPos(node.pos, child, node.size);
			for (int z = p.z; z < p.z + node.size; ++z) {
				for (int y = p.y; y < p.y + node.size; ++y) {
					for (int x = p.x; x < p.x + node.size; ++x) {
						volume.setVoxel(x, side - 1 - y, z, filler);
					}
				}
			}
		}
	}
	core::Buffer<glm::ivec3> positions;
	if (mode == 0) {
		surfaces(volume, glm::ivec3(0), side, side, wrap, solid, positions);
	} else {
		for (size_t i = prenodes; i < nodes.size(); ++i) {
			for (int child = 0; child < 8; ++child) {
				if (nodes[i].chi & (1 << child)) {
					positions.push_back(childPos(nodes[i].pos, child, 1));
				}
			}
		}
	}
	core::Buffer<color::RGBA> surfaceColors;
	core::Buffer<uint8_t> surfaceNormals;
	palette::NormalPalette normalPalette;
	Bits data(*stream);
	for (size_t i = 0; i < positions.size(); ++i) {
		uint32_t value;
		color::RGBA c;
		if (flags & 2) {
			if (!data.read(24, value)) {
				return false;
			}
			c = color::RGBA((value >> 16) & 255, (value >> 8) & 255, value & 255, 255);
		} else {
			if (!data.read(paletteBits, value) || value >= colors.size()) {
				return false;
			}
			c = colors[value];
		}
		const int normalFormat = (flags >> 2) & 3;
		if (!data.read(normalFormat == 1 ? normalBits : normalFormat == 2 ? 24 : 0, value)) {
			return false;
		}
		glm::vec3 normal(0.0f);
		if (normalFormat == 1) {
			// Equivec distributes directions on a spherical Fibonacci spiral.
			const double z = (2.0 * value + 1.0) / (double)(1u << normalBits) - 1.0;
			const double radius = std::sqrt(1.0 - z * z);
			const double angle = (double)value * 0.3819660112501052 * 6.283185307179586;
			normal = glm::vec3(std::cos(angle) * radius, std::sin(angle) * radius, z);
		} else if (normalFormat == 2) {
			normal = glm::vec3((int8_t)(value & 255), (int8_t)((value >> 8) & 255), (int8_t)((value >> 16) & 255));
		}
		surfaceNormals.push_back(importNormal(normalPalette, normal));
		if (flags & 16) {
			if (!data.read(8, value)) {
				return false;
			}
			c.a = value;
		}
		if ((flags & 32) && !data.read(8, value)) {
			return false;
		}
		surfaceColors.push_back(c);
	}
	palette::Palette palette;
	if (!surfaceColors.empty()) {
		palette.quantize(surfaceColors.data(), surfaceColors.size());
	} else {
		palette.tryAdd(color::RGBA(128, 128, 128, 255));
	}
	palette::PaletteLookup lookup(palette);
	for (size_t i = 0; i < positions.size(); ++i) {
		const glm::ivec3 &p = positions[i];
		volume.setVoxel(p.x, side - 1 - p.y, p.z,
						voxel::createVoxel(voxel::VoxelType::Generic, lookup.findClosestIndex(surfaceColors[i]),
										   surfaceNormals[i]));
	}
	model.setPalette(palette);
	if (normalPalette.size() > 0) {
		model.setNormalPalette(normalPalette);
	}
	model.setName(core::string::extractFilename(filename));
	if (!(flags & 1)) {
		model.setPivot(glm::vec3(pivot.x, (float)side - pivot.y, pivot.z) / (float)side);
	}
	return sceneGraph.emplace(core::move(model)) != InvalidNodeId;
}

bool KVOFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
						   const io::ArchivePtr &archive, const SaveContext &ctx) {
	const scenegraph::SceneGraphNode *model = sceneGraph.firstModelNode();
	if (model == nullptr) {
		return false;
	}
	const voxel::Region &sourceRegion = model->region();
	const glm::ivec3 dims = sourceRegion.getDimensionsInVoxels();
	int depth = 1;
	while ((1 << depth) < glm::max(dims.x, glm::max(dims.y, dims.z))) {
		if (++depth > 12) {
			return false;
		}
	}
	const int side = 1 << depth;
	const voxel::Region region(0, 0, 0, side - 1, side - 1, side - 1);
	if (!checkValidRegion(region)) {
		return false;
	}
	voxel::RawVolume volume(region);
	const glm::ivec3 lower = sourceRegion.getLowerCorner();
	for (int z = 0; z < dims.z; ++z) {
		for (int y = 0; y < dims.y; ++y) {
			for (int x = 0; x < dims.x; ++x) {
				volume.setVoxel(x, y, z, model->volume()->voxel(lower + glm::ivec3(x, y, z)));
			}
		}
	}
	// Build the mixed nodes breadth first. Uniform child blocks need only one solid/air bit.
	core::Buffer<Node> nodes;
	nodes.push_back(Node{glm::ivec3(0), side / 2, 0, 0});
	uint32_t prenodes = 0;
	for (size_t i = 0; i < nodes.size(); ++i) {
		const Node parent = nodes[i];
		if (parent.size > 1) {
			++prenodes;
		}
		for (int child = 0; child < 8; ++child) {
			const glm::ivec3 p = childPos(parent.pos, child, parent.size);
			const bool solid = voxel::isBlocked(volume.voxel(p.x, side - 1 - p.y, p.z).getMaterial());
			bool mixed = false;
			for (int z = p.z; z < p.z + parent.size && !mixed; ++z) {
				for (int y = p.y; y < p.y + parent.size && !mixed; ++y) {
					for (int x = p.x; x < p.x + parent.size; ++x) {
						if (voxel::isBlocked(volume.voxel(x, side - 1 - y, z).getMaterial()) != solid) {
							mixed = true;
							break;
						}
					}
				}
			}
			if (mixed) {
				nodes[i].chi |= 1 << child;
				nodes.push_back(Node{p, parent.size / 2, 0, 0});
			} else if (solid) {
				nodes[i].sol |= 1 << child;
			}
		}
	}
	core::Buffer<glm::ivec3> positions;
	surfaces(volume, glm::ivec3(0), side, side, 0, 0, positions);
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		return false;
	}
	// Mode 0 preserves the solid interior; RGB, signed XYZ normals and alpha are byte aligned.
	if (!stream->writeUInt32(FourCC('K', 'V', 'O', 'x')) || !stream->writeUInt8(1) || !stream->writeUInt8(depth) ||
		!stream->writeUInt16(26) || !stream->writeUInt32(0)) {
		return false;
	}
	const glm::vec3 pivot = model->pivot() * glm::vec3(dims) - priv::nodeTranslation(sceneGraph, *model);
	if (!stream->writeFloat(pivot.x) || !stream->writeFloat((float)side - pivot.y) || !stream->writeFloat(pivot.z)) {
		return false;
	}
	for (int i = 0; i < 9; ++i) {
		if (!stream->writeFloat(i % 4 == 0 ? 1.0f : 0.0f)) {
			return false;
		}
	}
	if (!stream->writeFloat((float)side) || !stream->writeUInt32(prenodes) ||
		!stream->writeUInt32((uint32_t)positions.size()) || !stream->writeUInt32((uint32_t)nodes.size())) {
		return false;
	}
	for (uint32_t i = 0; i < prenodes; ++i) {
		if (!stream->writeUInt8(nodes[i].chi)) {
			return false;
		}
	}
	uint8_t byte = 0;
	int bit = 0;
	for (const Node &node : nodes) {
		for (int child = 0; child < 8; ++child) {
			if (node.chi & (1 << child)) {
				continue;
			}
			byte |= ((node.sol >> child) & 1) << bit;
			if (++bit == 8) {
				if (!stream->writeUInt8(byte)) {
					return false;
				}
				byte = 0;
				bit = 0;
			}
		}
	}
	if (bit != 0 && !stream->writeUInt8(byte)) {
		return false;
	}
	for (const glm::ivec3 &p : positions) {
		const voxel::Voxel &v = volume.voxel(p.x, side - 1 - p.y, p.z);
		const color::RGBA c = model->palette().color(v.getColor());
		glm::vec3 normal(0.0f);
		if (model->hasNormalPalette() && v.getNormal() != NO_NORMAL &&
			v.getNormal() - NORMAL_PALETTE_OFFSET < (int)model->normalPalette().size()) {
			normal = glm::normalize(model->normalPalette().normal3f(v.getNormal() - NORMAL_PALETTE_OFFSET));
			normal.y = -normal.y;
		}
		if (!stream->writeUInt8(c.b) || !stream->writeUInt8(c.g) || !stream->writeUInt8(c.r) ||
			!stream->writeInt8((int8_t)std::round(normal.x * 127.0f)) ||
			!stream->writeInt8((int8_t)std::round(normal.y * 127.0f)) ||
			!stream->writeInt8((int8_t)std::round(normal.z * 127.0f)) || !stream->writeUInt8(c.a)) {
			return false;
		}
	}
	return true;
}

} // namespace voxelformat
