/**
 * @file
 */

#include "SproxelFormat.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/Tokenizer.h"
#include "io/Stream.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/MaterialColor.h"
#include "palette/PaletteLookup.h"
#include "voxelformat/Format.h"

namespace voxelformat {

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load sproxel csv file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)",    \
				   (int)__LINE__);                                                                                     \
		return false;                                                                                                  \
	}

#define wrapBool(read)                                                                                                 \
	if ((read) == false) {                                                                                             \
		Log::error("Could not load sproxel csv file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)",    \
				   (int)__LINE__);                                                                                     \
		return false;                                                                                                  \
	}

static bool skipNewline(io::SeekableReadStream &stream) {
	uint8_t chr;
	if (stream.readUInt8(chr) == -1) {
		Log::error("Failed to read newline character from stream");
		return false;
	}
	if (chr == '\r') {
		if (stream.peekUInt8(chr) != -1) {
			if (chr == '\n') {
				if (stream.skip(1) == -1) {
					Log::error("Failed to skip newline character from stream");
					return false;
				}
			}
		}
	}
	return true;
}

static bool skipComma(io::SeekableReadStream &stream) {
	uint8_t chr;
	if (stream.readUInt8(chr) == -1) {
		Log::error("Failed to read comma character from stream");
		return false;
	}
	if (chr != ',') {
		Log::error("Got unexpected character, expected , - got %c", chr);
		return false;
	}
	return true;
}

size_t SproxelFormat::loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
								  const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return 0;
	}
	char buf[512];
	if (!stream->readLine(sizeof(buf), buf)) {
		Log::error("Could not load sproxel csv file");
		return 0u;
	}

	core::Tokenizer tok(buf, ",");
	if (tok.size() != 3u) {
		Log::error("Invalid size components found - expected x,y,z");
		return 0u;
	}

	const int32_t width = core::string::toInt(tok.tokens()[0]);
	const int32_t height = core::string::toInt(tok.tokens()[1]);
	const int32_t depth = core::string::toInt(tok.tokens()[2]);
	glm::ivec3 size(width, height, depth);

	palette::RGBABuffer colors;
	for (int y = size.y - 1; y >= 0; y--) {
		for (int z = 0; z < size.z; z++) {
			for (int x = 0; x < size.x; x++) {
				char hex[10];
				if ((stream->read(hex, 9)) == -1) {
					Log::error("Could not load sproxel csv color line");
					return 0u;
				}
				hex[sizeof(hex) - 1] = '\0';
				uint8_t r, g, b, a;
				const int n = core::string::parseHex(hex, r, g, b, a);
				if (n != 4) {
					Log::error("Failed to parse color %i (%s)", n, hex);
					return 0u;
				}
				if (a != 0) {
					const color::RGBA color(r, g, b, a);
					colors.put(color, true);
				}
				if (x != size.x - 1) {
					if (!skipComma(*stream)) {
						Log::error("Failed to skip 1 byte");
						return false;
					}
				}
			}
			if (!skipNewline(*stream)) {
				return false;
			}
		}
		if (!skipNewline(*stream)) {
			return false;
		}
	}
	return createPalette(colors, palette);
}

bool SproxelFormat::loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive,
								   scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
								   const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	char buf[512];
	wrapBool(stream->readLine(sizeof(buf), buf))

	core::Tokenizer tok(buf, ",");
	if (tok.size() != 3u) {
		Log::error("Invalid size components found - expected x,y,z");
		return false;
	}

	const int32_t sizex = core::string::toInt(tok.tokens()[0]);
	const int32_t sizey = core::string::toInt(tok.tokens()[1]);
	const int32_t sizez = core::string::toInt(tok.tokens()[2]);

	const voxel::Region region(0, 0, 0, sizex - 1, sizey - 1, sizez - 1);
	if (!region.isValid()) {
		Log::error("Invalid region %i:%i:%i", sizex, sizey, sizez);
		return false;
	}

	voxel::RawVolume *volume = new voxel::RawVolume(region);
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(volume, true);

	palette::PaletteLookup palLookup(palette);
	// TODO: PERF: use volume sampler
	for (int y = sizey - 1; y >= 0; y--) {
		for (int z = 0; z < sizez; z++) {
			for (int x = 0; x < sizex; x++) {
				char hex[10];
				if (stream->read(hex, 9) == -1) {
					Log::error("Could not load sproxel csv color line");
					return false;
				}
				hex[sizeof(hex) - 1] = '\0';
				uint8_t r, g, b, a;
				const int n = core::string::parseHex(hex, r, g, b, a);
				if (n != 4) {
					Log::error("Failed to parse color %i (%s)", n, hex);
					return false;
				}
				if (a != 0) {
					const color::RGBA color = flattenRGB(r, g, b, a);
					const uint8_t index = palLookup.findClosestIndex(color);
					const voxel::Voxel voxel = voxel::createVoxel(palette, index);
					volume->setVoxel(x, y, z, voxel);
				}
				if (x != sizex - 1) {
					if (!skipComma(*stream)) {
						Log::error("Failed to skip 1 byte");
						return false;
					}
				}
			}
			if (!skipNewline(*stream)) {
				return false;
			}
		}
		if (!skipNewline(*stream)) {
			return false;
		}
	}
	node.setName(core::string::extractFilename(filename));
	node.setPalette(palLookup.palette());
	return sceneGraph.emplace(core::move(node)) != InvalidNodeId;
}

bool SproxelFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
							   const io::ArchivePtr &archive, const SaveContext &ctx) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	core_assert(node);

	const voxel::Region &region = node->region();
	const palette::Palette &palette = node->palette();
	voxel::RawVolume::Sampler sampler(node->volume());
	const glm::ivec3 &lower = region.getLowerCorner();

	const int width = region.getWidthInVoxels();
	const int height = region.getHeightInVoxels();
	const int depth = region.getDepthInVoxels();
	if (!stream->writeStringFormat(false, "%i,%i,%i\n", width, height, depth)) {
		Log::error("Could not save sproxel csv file");
		return false;
	}

	for (int y = height - 1; y >= 0; y--) {
		for (int z = 0u; z < depth; ++z) {
			for (int x = 0u; x < width; ++x) {
				core_assert_always(sampler.setPosition(lower.x + x, lower.y + y, lower.z + z));
				const voxel::Voxel &voxel = sampler.voxel();
				if (voxel.getMaterial() == voxel::VoxelType::Air) {
					stream->writeString("#00000000", false);
				} else {
					const color::RGBA rgba = palette.color(voxel.getColor());
					stream->writeStringFormat(false, "#%02X%02X%02X%02X", rgba.r, rgba.g, rgba.b, rgba.a);
				}
				if (x != width - 1) {
					stream->writeString(",", false);
				}
			}
			stream->writeString("\n", false);
		}
		stream->writeString("\n", false);
	}
	return true;
}

#undef wrap
#undef wrapBool

} // namespace voxelformat
