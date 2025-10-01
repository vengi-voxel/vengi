/**
 * @file
 */

#include "MTSFormat.h"
#include "core/Common.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/collection/Array3DView.h"
#include "core/collection/Buffer.h"
#include "core/collection/DynamicArray.h"
#include "io/ZipReadStream.h"
#include "io/ZipWriteStream.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "MinecraftPaletteMap.h"
#include "palette/Palette.h"

namespace voxelformat {

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load mts file: Not enough data in stream " CORE_STRINGIFY(read));                        \
		return false;                                                                                                  \
	}

bool MTSFormat::loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
								  scenegraph::SceneGraph &sceneGraph, palette::Palette &palette, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	uint32_t magic;
	wrap(stream->readUInt32(magic))
	if (magic != FourCC('M', 'T', 'S', 'M')) {
		Log::error("Invalid mts magic");
		return false;
	}
	uint16_t version;
	wrap(stream->readUInt16BE(version))
	if (version > 4u) {
		Log::error("Unsupported version: %i", (int)version);
		return false;
	}

	glm::u16vec3 size;
	wrap(stream->readUInt16BE(size.x))
	wrap(stream->readUInt16BE(size.y))
	wrap(stream->readUInt16BE(size.z))

	Log::debug("Size: %u:%u:%u", size.x, size.y, size.z);

	core::Buffer<uint8_t> probs;
	probs.resize(size.y);

	if (version >= 3) {
		for (uint16_t y = 0; y < size.y; ++y) {
			wrap(stream->readUInt8(probs[y]))
		}
	} else {
		probs.fill(0xFF);
	}

	uint16_t idmapcount;
	wrap(stream->readUInt16BE(idmapcount))
	Log::debug("idmapcount: %i", idmapcount);

	core::DynamicArray<core::String> names;
	names.reserve(idmapcount);
	for (uint16_t i = 0; i < idmapcount; ++i) {
		core::String name;
		if (!stream->readPascalStringUInt16BE(name)) {
			Log::error("Failed to read material name");
			return false;
		}
		if (name == "ignore") {
			name = "air";
		}
		Log::debug("Found material '%s'", name.c_str());
		names.emplace_back(core::move(name));
	}
	Log::debug("found %i materials", (int)names.size());

	struct Node {
		uint16_t param0;	 // color name index - the index param0[(Z-z)*Z*Y + y*X + x].
							 // The Z axis is mirrored.
		uint8_t probability; // param1 - ranges from 0 (0%) to 127 (100%). Bit 7 means force node placement,
							 // i.e. the node will be able to replace non-air nodes as well. (In legacy version 3,
							 // param1’s probability range was from 0 to 0xFF, there’s no force placement.)
		uint8_t param2;		 // param2 - an 8-bit value (0-255), the meaning depends on the node definition.
							 // See lua_api.md to learn more about param2 (keywords: “param2”, “paramtype2”).
	};

	const int nodecount = (int)size.x * (int)size.y * (int)size.z;
	core::Buffer<Node> databuf;
	databuf.resize(nodecount);

	io::ZipReadStream zipStream(*stream);
	// read param0 values
	for (int i = 0; i < nodecount; ++i) {
		wrap(zipStream.readUInt16BE(databuf[i].param0))
	}

	// skip the probability values (param1)
	// skip the param2 values

	palette.minecraft();
	const voxel::Region region(0, 0, 0, (int)size.x - 1, (int)size.y - 1, (int)size.z - 1);
	voxel::RawVolume *volume = new voxel::RawVolume(region);
	core::Array3DView<Node> view(databuf.data(), size.x, size.y, size.z);
	// TODO: PERF: use volume sampler
	// TODO: PERF: FOR_PARALLEL
	for (uint16_t x = 0; x < size.x; ++x) {
		for (uint16_t y = 0; y < size.y; ++y) {
			for (uint16_t z = 0; z < size.z; ++z) {
				const Node &node = view.get(x, y, z);
				if (node.param0 >= idmapcount) {
					continue;
				}
				const core::String &name = names[node.param0];
				if (name == "air") {
					continue;
				}
				volume->setVoxel(x, y, z, voxel::createVoxel(palette, findPaletteIndex(name, 0)));
			}
		}
	}

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(volume, true);
	node.setPalette(palette);
	node.setName(core::string::extractFilename(filename));
	return sceneGraph.emplace(core::move(node)) != InvalidNodeId;
}

#undef wrap

#define wrapBool(read)                                                                                                 \
	if ((read) == false) {                                                                                                 \
		Log::error("Could not save mts file: " CORE_STRINGIFY(read));                                                  \
		return false;                                                                                                  \
	}

bool MTSFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
						   const io::ArchivePtr &archive, const SaveContext &) {
	scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	if (node == nullptr) {
		Log::error("No model node found in scene graph");
		return false;
	}

	const voxel::RawVolume *volume = node->volume();
	const voxel::Region &region = volume->region();
	const palette::Palette &palette = node->palette();

	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}
	wrapBool(stream->writeUInt32(FourCC('M', 'T', 'S', 'M')))
	wrapBool(stream->writeUInt16BE(4))							// version 4
	wrapBool(stream->writeUInt16BE(region.getWidthInVoxels()))	// size x
	wrapBool(stream->writeUInt16BE(region.getHeightInVoxels())) // size y
	wrapBool(stream->writeUInt16BE(region.getDepthInVoxels()))	// size z

	for (int32_t y = 0; y < region.getHeightInVoxels(); ++y) {
		wrapBool(stream->writeUInt8(0x7f))
	}

	uint16_t idmapcount = node->palette().colorCount();
	wrapBool(stream->writeUInt16BE(idmapcount + 1))

	palette::Palette mcpal;
	mcpal.minecraft();

	stream->writePascalStringUInt16BE("air");
	for (int i = 0; i < palette.colorCount(); ++i) {
		const core::RGBA rgba = palette.color(i);
		const int palIdx = mcpal.getClosestMatch(rgba);
		const core::String &name = findPaletteName(palIdx);
		wrapBool(stream->writePascalStringUInt16BE(name))
	}

	// param0
	io::ZipWriteStream zipStream(*stream);
	for (int32_t x = 0; x < region.getWidthInVoxels(); ++x) {
		for (int32_t y = 0; y < region.getHeightInVoxels(); ++y) {
			for (int32_t z = 0; z < region.getDepthInVoxels(); ++z) {
				const voxel::Voxel &v = volume->voxel(x, y, z);
				if (voxel::isAir(v.getMaterial())) {
					wrapBool(zipStream.writeUInt16BE(0)) // air
					continue;
				}
				wrapBool(zipStream.writeUInt16BE(v.getColor() + 1))
			}
		}
	}
	// probability values (param1)
	for (int32_t n = 0; n < region.voxels(); ++n) {
		wrapBool(zipStream.writeUInt8(0x7f))
	}
	// param2
	for (int32_t n = 0; n < region.voxels(); ++n) {
		wrapBool(zipStream.writeUInt8(0x00))
	}
	zipStream.flush();
	return true;
}

#undef wrapBool

} // namespace voxelformat
