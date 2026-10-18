/**
 * @file
 */

#include "TroveBlueprintFormat.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "io/Stream.h"
#include "io/ZipReadStream.h"
#include "io/ZipWriteStream.h"
#include "palette/PaletteLookup.h"
#include "scenegraph/SceneGraph.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

namespace voxelformat {

namespace {

constexpr uint8_t Magic[5] = {'k', 'i', 'w', 'i', 'b'};
constexpr uint32_t VersionV3 = 3u;
constexpr uint32_t VersionV4 = 4u;
constexpr uint32_t VersionV5 = 5u;
constexpr uint16_t DefaultType = 0x15; // solid voxel
constexpr int MaxAxisSize = 2048;

struct TroveVoxel {
	int32_t x = 0;
	int32_t y = 0;
	int32_t z = 0;
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;
	uint8_t w = 0;
	uint16_t type = DefaultType;
};

struct DecodedBlueprint {
	uint32_t version = 0;
	glm::ivec3 size{0};
	glm::ivec3 pos{0};
	core::DynamicArray<TroveVoxel> voxels;
};

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load Trove blueprint: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)",     \
				   (int)__LINE__);                                                                                     \
		return false;                                                                                                  \
	}

#define wrapBool(read)                                                                                                 \
	if ((read) != true) {                                                                                              \
		Log::error("Could not load Trove blueprint: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)",     \
				   (int)__LINE__);                                                                                     \
		return false;                                                                                                  \
	}

static bool readUleb128(io::ReadStream &stream, uint32_t &out) {
	uint32_t result = 0u;
	uint32_t shift = 0u;
	for (;;) {
		uint8_t byte = 0u;
		if (stream.readUInt8(byte) != 0) {
			return false;
		}
		result |= (uint32_t)(byte & 0x7fu) << shift;
		if ((byte & 0x80u) == 0u) {
			out = result;
			return true;
		}
		shift += 7u;
		if (shift >= 35u) {
			return false;
		}
	}
}

static bool readSvarint(io::ReadStream &stream, int32_t &out) {
	uint32_t raw = 0u;
	if (!readUleb128(stream, raw)) {
		return false;
	}
	out = (int32_t)((raw >> 1u) ^ (uint32_t)-(int32_t)(raw & 1u));
	return true;
}

static bool decodeV34(io::SeekableReadStream &stream, uint32_t version, DecodedBlueprint &out) {
	uint32_t count = 0u;
	wrapBool(readUleb128(stream, count))
	if (count == 0u) {
		Log::error("Trove blueprint contains no voxels");
		return false;
	}
	if (count > 64u * 1024u * 1024u) {
		Log::error("Trove blueprint voxel count is too large: %u", count);
		return false;
	}

	core::DynamicArray<TroveVoxel> raw;
	raw.reserve(count);
	int32_t minX = 0;
	int32_t minY = 0;
	int32_t minZ = 0;
	int32_t maxX = 0;
	int32_t maxY = 0;
	int32_t maxZ = 0;
	for (uint32_t i = 0u; i < count; ++i) {
		TroveVoxel v;
		wrapBool(readSvarint(stream, v.x))
		wrapBool(readSvarint(stream, v.y))
		wrapBool(readSvarint(stream, v.z))
		wrap(stream.readUInt16(v.type))
		wrap(stream.readUInt8(v.b))
		wrap(stream.readUInt8(v.g))
		wrap(stream.readUInt8(v.r))
		wrap(stream.readUInt8(v.w))
		if (i == 0u) {
			minX = maxX = v.x;
			minY = maxY = v.y;
			minZ = maxZ = v.z;
		} else {
			minX = core_min(minX, v.x);
			minY = core_min(minY, v.y);
			minZ = core_min(minZ, v.z);
			maxX = core_max(maxX, v.x);
			maxY = core_max(maxY, v.y);
			maxZ = core_max(maxZ, v.z);
		}
		raw.push_back(v);
	}

	out.version = version;
	out.size = glm::ivec3(maxX - minX + 1, maxY - minY + 1, maxZ - minZ + 1);
	out.pos = glm::ivec3(-(out.size.x / 2), -(out.size.y / 2), -(out.size.z / 2));
	out.voxels.reserve(raw.size());
	for (const TroveVoxel &src : raw) {
		TroveVoxel v = src;
		v.x -= minX;
		v.y -= minY;
		v.z -= minZ;
		out.voxels.push_back(v);
	}
	return true;
}

static bool decodeV5(io::SeekableReadStream &stream, DecodedBlueprint &out) {
	const int64_t remaining = stream.remaining();
	if (remaining <= 0) {
		Log::error("Trove blueprint v5 body is empty");
		return false;
	}
	io::ZipReadStream zip(stream, (int)remaining);
	int32_t px = 0;
	int32_t py = 0;
	int32_t pz = 0;
	int32_t sx = 0;
	int32_t sy = 0;
	int32_t sz = 0;
	int32_t count = 0;
	int32_t start = 0;
	wrap(zip.readInt32(px))
	wrap(zip.readInt32(py))
	wrap(zip.readInt32(pz))
	wrap(zip.readInt32(sx))
	wrap(zip.readInt32(sy))
	wrap(zip.readInt32(sz))
	wrap(zip.readInt32(count))
	wrap(zip.readInt32(start))
	if (count <= 0 || sx <= 0 || sy <= 0 || sz <= 0) {
		Log::error("Trove blueprint v5 has invalid bounds or empty voxel list");
		return false;
	}
	if (sx > MaxAxisSize || sy > MaxAxisSize || sz > MaxAxisSize) {
		Log::error("Trove blueprint exceeds max size: %i:%i:%i", sx, sy, sz);
		return false;
	}
	if (count > sx * sy * sz) {
		Log::error("Trove blueprint voxel count %i exceeds volume capacity", count);
		return false;
	}

	core::DynamicArray<int32_t> indices;
	indices.reserve((size_t)count);
	indices.push_back(start);
	for (int32_t i = 1; i < count; ++i) {
		int32_t delta = 0;
		wrap(zip.readInt32(delta))
		indices.push_back(indices.back() + delta);
	}

	core::DynamicArray<uint16_t> types;
	types.reserve((size_t)count);
	for (int32_t i = 0; i < count; ++i) {
		uint16_t type = 0u;
		wrap(zip.readUInt16(type))
		types.push_back(type);
	}

	out.version = VersionV5;
	out.size = glm::ivec3(sx, sy, sz);
	out.pos = glm::ivec3(px, py, pz);
	out.voxels.reserve((size_t)count);
	const int32_t plane = sx * sz;
	for (int32_t i = 0; i < count; ++i) {
		uint32_t color = 0u;
		wrap(zip.readUInt32(color))
		const int32_t L = indices[i];
		const int32_t y = L / plane;
		const int32_t rem = L % plane;
		TroveVoxel v;
		v.x = rem % sx;
		v.y = y;
		v.z = rem / sx;
		v.b = (uint8_t)(color & 0xffu);
		v.g = (uint8_t)((color >> 8u) & 0xffu);
		v.r = (uint8_t)((color >> 16u) & 0xffu);
		v.w = (uint8_t)((color >> 24u) & 0xffu);
		v.type = types[i];
		if (v.x < 0 || v.y < 0 || v.z < 0 || v.x >= sx || v.y >= sy || v.z >= sz) {
			Log::error("Trove blueprint voxel index %i is outside bounds", L);
			return false;
		}
		out.voxels.push_back(v);
	}
	return true;
}

static bool decodeBlueprint(io::SeekableReadStream &stream, DecodedBlueprint &out) {
	uint8_t magic[5];
	for (int i = 0; i < 5; ++i) {
		wrap(stream.readUInt8(magic[i]))
	}
	if (magic[0] != Magic[0] || magic[1] != Magic[1] || magic[2] != Magic[2] || magic[3] != Magic[3] ||
		magic[4] != Magic[4]) {
		Log::error("Invalid Trove blueprint magic");
		return false;
	}
	uint32_t version = 0u;
	wrap(stream.readUInt32(version))
	if (version == VersionV3 || version == VersionV4) {
		return decodeV34(stream, version, out);
	}
	if (version == VersionV5) {
		return decodeV5(stream, out);
	}
	Log::error("Unsupported Trove blueprint version: %u", version);
	return false;
}

static bool encodeV5(io::WriteStream &stream, const voxel::RawVolume &volume, const palette::Palette &palette) {
	const voxel::Region &region = volume.region();
	const int32_t sx = region.getWidthInVoxels();
	const int32_t sy = region.getHeightInVoxels();
	const int32_t sz = region.getDepthInVoxels();
	if (sx <= 0 || sy <= 0 || sz <= 0 || sx > MaxAxisSize || sy > MaxAxisSize || sz > MaxAxisSize) {
		Log::error("Invalid volume size for Trove blueprint: %i:%i:%i", sx, sy, sz);
		return false;
	}

	struct Cell {
		int32_t index;
		uint16_t type;
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t w;
	};
	core::DynamicArray<Cell> cells;
	const glm::ivec3 mins = region.getLowerCorner();
	const int32_t plane = sx * sz;
	voxel::RawVolume::Sampler sampler(volume);
	for (int32_t y = 0; y < sy; ++y) {
		for (int32_t z = 0; z < sz; ++z) {
			for (int32_t x = 0; x < sx; ++x) {
				core_assert_always(sampler.setPosition(mins.x + x, mins.y + y, mins.z + z));
				const voxel::Voxel &voxel = sampler.voxel();
				if (voxel::isAir(voxel.getMaterial())) {
					continue;
				}
				color::RGBA rgba = palette.color(voxel.getColor());
				if (rgba.a == 0u) {
					continue;
				}
				Cell cell;
				cell.index = x + z * sx + y * plane;
				cell.type = DefaultType;
				cell.r = rgba.r;
				cell.g = rgba.g;
				cell.b = rgba.b;
				cell.w = 0u;
				cells.push_back(cell);
			}
		}
	}
	if (cells.empty()) {
		Log::error("Cannot encode an empty Trove blueprint");
		return false;
	}
	// DynamicArray::sort expects true when lhs > rhs for ascending order.
	cells.sort([](const Cell &a, const Cell &b) { return a.index > b.index; });

	wrapBool(stream.writeUInt8(Magic[0]))
	wrapBool(stream.writeUInt8(Magic[1]))
	wrapBool(stream.writeUInt8(Magic[2]))
	wrapBool(stream.writeUInt8(Magic[3]))
	wrapBool(stream.writeUInt8(Magic[4]))
	wrapBool(stream.writeUInt32(VersionV5))

	io::ZipWriteStream zip(stream, 9);
	const int32_t px = -(sx / 2);
	const int32_t py = 0;
	const int32_t pz = -(sz / 2);
	wrapBool(zip.writeInt32(px))
	wrapBool(zip.writeInt32(py))
	wrapBool(zip.writeInt32(pz))
	wrapBool(zip.writeInt32(sx))
	wrapBool(zip.writeInt32(sy))
	wrapBool(zip.writeInt32(sz))
	wrapBool(zip.writeInt32((int32_t)cells.size()))
	wrapBool(zip.writeInt32(cells[0].index))
	for (size_t i = 1; i < cells.size(); ++i) {
		wrapBool(zip.writeInt32(cells[i].index - cells[i - 1].index))
	}
	for (const Cell &cell : cells) {
		wrapBool(zip.writeUInt16(cell.type))
	}
	for (const Cell &cell : cells) {
		const uint32_t color = (uint32_t)cell.b | ((uint32_t)cell.g << 8u) | ((uint32_t)cell.r << 16u) |
							   ((uint32_t)cell.w << 24u);
		wrapBool(zip.writeUInt32(color))
	}
	// empty entity section
	wrapBool(zip.writeUInt32(0u))
	return zip.flush();
}

#undef wrap
#undef wrapBool

} // namespace

size_t TroveBlueprintFormat::loadPalette(const core::String &filename, const io::ArchivePtr &archive,
										 palette::Palette &palette, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return 0u;
	}
	DecodedBlueprint decoded;
	if (!decodeBlueprint(*stream, decoded)) {
		return 0u;
	}
	palette::RGBABuffer colors;
	for (const TroveVoxel &v : decoded.voxels) {
		colors.put(flattenRGB(v.r, v.g, v.b), true);
	}
	ctx.report("palette", 1, 1);
	return createPalette(colors, palette);
}

bool TroveBlueprintFormat::loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive,
										  scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
										  const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	DecodedBlueprint decoded;
	if (!decodeBlueprint(*stream, decoded)) {
		return false;
	}
	if (decoded.size.x <= 0 || decoded.size.y <= 0 || decoded.size.z <= 0 || decoded.size.x > MaxAxisSize ||
		decoded.size.y > MaxAxisSize || decoded.size.z > MaxAxisSize) {
		Log::error("Invalid Trove blueprint size: %i:%i:%i", decoded.size.x, decoded.size.y, decoded.size.z);
		return false;
	}

	const voxel::Region region(0, 0, 0, decoded.size.x - 1, decoded.size.y - 1, decoded.size.z - 1);
	if (!checkValidRegion(region)) {
		return false;
	}

	voxel::RawVolume *volume = new voxel::RawVolume(region);
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(volume);
	palette::PaletteLookup palLookup(palette);

	const size_t total = decoded.voxels.size();
	for (size_t i = 0; i < total; ++i) {
		const TroveVoxel &v = decoded.voxels[i];
		ctx.report("voxels", (int)i, (int)total);
		const color::RGBA color = flattenRGB(v.r, v.g, v.b);
		const uint8_t index = palLookup.findClosestIndex(color);
		volume->setVoxel(v.x, v.y, v.z, voxel::createVoxel(palette, index));
	}
	ctx.report("voxels", (int)total, (int)total);

	node.setName(core::string::extractFilename(filename));
	node.setPalette(palette);
	sceneGraph.emplace(core::move(node));
	return true;
}

bool TroveBlueprintFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
									  const io::ArchivePtr &archive, const SaveContext &ctx) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open stream for file %s", filename.c_str());
		return false;
	}
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	core_assert(node);
	core_assert(node->volume());
	ctx.report("save", 0, 1);
	if (!encodeV5(*stream, *node->volume(), node->palette())) {
		return false;
	}
	ctx.report("save", 1, 1);
	return true;
}

} // namespace voxelformat
