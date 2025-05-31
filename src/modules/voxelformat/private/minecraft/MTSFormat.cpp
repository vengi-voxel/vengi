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
		Log::error("Invalid magic");
		return false;
	}
	uint16_t version;
	wrap(stream->readUInt16BE(version))
	if (version > 4u) {
		Log::error("Unsupported version: %i", (int)version);
		return false;
	}

	glm::i16vec3 size;
	wrap(stream->readInt16BE(size.x))
	wrap(stream->readInt16BE(size.y))
	wrap(stream->readInt16BE(size.z))

	Log::debug("Size: %i:%i:%i", size.x, size.y, size.z);

	core::Buffer<uint8_t> probs;
	probs.resize(size.y);

	if (version >= 3) {
		for (int16_t y = 0; y < size.y; ++y) {
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
		uint16_t param0; // material?
	};
	static_assert(sizeof(Node) == sizeof(uint16_t), "Unexpected node struct size");

	const int nodecount = (int)size.x * (int)size.y * (int)size.z;
	core::Buffer<Node> databuf;
	databuf.resize(nodecount);

	io::ZipReadStream zipStream(*stream);
	for (int i = 0; i < nodecount; ++i) {
		wrap(zipStream.readUInt16BE(databuf[i].param0))
	}

	palette.minecraft();
	const voxel::Region region(0, 0, 0, size.x - 1, size.y - 1, size.z - 1);
	voxel::RawVolume *volume = new voxel::RawVolume(region);
	core::Array3DView<Node> view(databuf.data(), size.x, size.y, size.z);
	for (int16_t x = 0; x < size.x; ++x) {
		for (int16_t y = 0; y < size.y; ++y) {
			for (int16_t z = 0; z < size.z; ++z) {
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
	wrapBool(stream->writeUInt16BE(4))						   // version 4
	wrapBool(stream->writeInt16BE(region.getWidthInVoxels()))  // size x
	wrapBool(stream->writeInt16BE(region.getHeightInVoxels())) // size y
	wrapBool(stream->writeInt16BE(region.getDepthInVoxels()))  // size z

	for (int32_t y = 0; y < region.getHeightInVoxels(); ++y) {
		wrapBool(stream->writeUInt8(0xFF))
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

	io::ZipWriteStream zipStream(*stream);
	for (int32_t x = 0; x < region.getWidthInVoxels(); ++x) {
		for (int32_t y = 0; y < region.getHeightInVoxels(); ++y) {
			for (int32_t z = 0; z < region.getDepthInVoxels(); ++z) {
				const voxel::Voxel &v = volume->voxel(x, y, z);
				wrapBool(zipStream.writeUInt16BE(v.getColor() + 1))
			}
		}
	}
	zipStream.flush();
	return true;
}

#undef wrapBool

} // namespace voxelformat
