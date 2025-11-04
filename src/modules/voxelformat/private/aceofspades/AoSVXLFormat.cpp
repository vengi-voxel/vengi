/**
 * @file
 */

#include "AoSVXLFormat.h"
#include "app/Async.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "palette/Palette.h"
#include "palette/PaletteLookup.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphNodeProperties.h"
#include "util/IniParser.h"
#include "voxel/RawVolume.h"
#include "voxelutil/VolumeVisitor.h"
#define libvxl_assert core_assert_msg
#define libvxl_mem_malloc core_malloc
#define libvxl_mem_realloc core_realloc
#define libvxl_mem_free core_free
extern "C" {
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#endif
#include "voxelformat/external/libvxl.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
}

namespace voxelformat {

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load AoE vxl file: Not enough data in stream " CORE_STRINGIFY(read));                    \
		return false;                                                                                                  \
	}

static inline uint32_t vxl_color(core::RGBA rgba) {
	return (rgba.r << 16) | (rgba.g << 8) | rgba.b;
}
static inline uint8_t vxl_blue(uint32_t c) {
	return c & 0xFF;
}
static inline uint8_t vxl_green(uint32_t c) {
	return (c >> 8) & 0xFF;
}
static inline uint8_t vxl_red(uint32_t c) {
	return (c >> 16) & 0xFF;
}

static bool readVec3(io::SeekableReadStream *stream, glm::dvec3 &vec) {
	if (stream->readDouble(vec.x) != 0) {
		return false;
	}
	if (stream->readDouble(vec.y) != 0) {
		return false;
	}
	if (stream->readDouble(vec.z) != 0) {
		return false;
	}
	return true;
}

bool AoSVXLFormat::loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive,
								  scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
								  const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Failed to open stream for file: %s", filename.c_str());
		return false;
	}
	size_t mapSize = 0;
	size_t mapHeight = 0;
	uint32_t magic = 0;
	stream->peekUInt32(magic);
	const bool slab5 = magic == FourCC('\x00', '\x20', '\x07', '\x09') || magic == FourCC('\x09', '\x07', '\x20', '\x00');
	if (slab5) {
		stream->skip(4);
		Log::debug("Found slab5 vxl");
		uint32_t width, height;
		if (stream->readUInt32(width) != 0) {
			Log::error("Failed to read width");
			return false;
		}
		if (stream->readUInt32(height) != 0) {
			Log::error("Failed to read height");
			return false;
		}

		if (width != 1024 || height != 1024) {
			Log::error("Invalid dimensions: %u:%u", width, height);
			return false;
		}

		mapSize = width;
		mapHeight = 256;

		glm::dvec3 ipo;
		if (!readVec3(stream, ipo)) {
			Log::error("Failed to read ipo/camera position");
			return false;
		}
		glm::dvec3 ist;
		if (!readVec3(stream, ist)) {
			Log::error("Failed to read ist/unit right vector");
			return false;
		}
		glm::dvec3 ihe;
		if (!readVec3(stream, ihe)) {
			Log::error("Failed to read ihe/unit down vector");
			return false;
		}
		glm::dvec3 ifo;
		if (!readVec3(stream, ifo)) {
			Log::error("Failed to read ifo/unit forward vector");
			return false;
		}
	}
	const int64_t size = stream->remaining();
	uint8_t *data = (uint8_t *)core_malloc(size);
	if (stream->read(data, size) == -1) {
		Log::error("Failed to read vxl stream for %s of size %i", filename.c_str(), (int)size);
		core_free(data);
		return false;
	}

	if (!slab5 && !libvxl_size(&mapSize, &mapHeight, data, size)) {
		Log::error("Failed to determine vxl size");
		core_free(data);
		return false;
	}

	struct libvxl_map map;

	if (!libvxl_create(&map, mapSize, mapSize, mapHeight, data, size)) {
		Log::error("Failed to create libvxl map");
		libvxl_free(&map);
		core_free(data);
		return false;
	}

	Log::debug("Read vxl of size %i:%i:%i", (int)mapSize, (int)mapHeight, (int)mapSize);

	const voxel::Region region(0, 0, 0, (int)mapSize - 1, (int)mapHeight - 1, (int)mapSize - 1);
	voxel::RawVolume *volume = new voxel::RawVolume(region);
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(volume, true);

	palette::PaletteLookup palLookup(palette);
	auto fn = [volume, &map, &mapSize, &mapHeight, &palLookup, &palette, this](int start, int end) {
		voxel::RawVolume::Sampler sampler(*volume);
		sampler.setPosition(0, 0, start);
		for (int z = start; z < end; z++) {
			voxel::RawVolume::Sampler sampler2 = sampler;
			for (int y = 0; y < (int)mapHeight; y++) {
				voxel::RawVolume::Sampler sampler3 = sampler2;
				for (int x = 0; x < (int)mapSize; x++) {
					if (!libvxl_map_issolid(&map, x, z, (int)mapHeight - 1 - y)) {
						sampler3.movePositiveX();
						continue;
					}
					const uint32_t color = libvxl_map_get(&map, x, z, (int)mapHeight - 1 - y);
					const core::RGBA rgba = flattenRGB(vxl_red(color), vxl_green(color), vxl_blue(color));
					const uint8_t paletteIndex = palLookup.findClosestIndex(rgba);
					sampler3.setVoxel(voxel::createVoxel(palette, paletteIndex));
					sampler3.movePositiveX();
				}
				sampler2.movePositiveY();
			}
			sampler.movePositiveZ();
		}
	};
	app::for_parallel(0, mapSize, fn);
	libvxl_free(&map);
	core_free(data);

	node.setName(core::string::extractFilename(filename));
	node.setPalette(palette);
	loadMetadataTxt(sceneGraph.node(sceneGraph.root().id()), filename, archive);
	return sceneGraph.emplace(core::move(node)) != InvalidNodeId;
}

void AoSVXLFormat::loadMetadataTxt(scenegraph::SceneGraphNode &node, const core::String &filename,
								   const io::ArchivePtr &archive) const {
	if (!archive->exists(filename + ".txt")) {
		Log::debug("No metadata file found for %s", filename.c_str());
		return;
	}
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename + ".txt"));
	if (!stream) {
		Log::debug("No metadata file found for %s", filename.c_str());
		return;
	}
	loadMetadataTxt(node, filename, stream);
}

// Helper to extract Python-style quoted string content, e.g. "hello world" or 'foo'
core::String extractQuotedString(const core::String &s) {
	core::String str = s.trim();
	if ((core::string::startsWith(str, "\"") && core::string::endsWith(str, "\"")) ||
		(core::string::startsWith(str, "'") && core::string::endsWith(str, "'"))) {
		return str.substr(1, str.size() - 2);
	}
	return str;
}

void AoSVXLFormat::loadMetadataTxt(scenegraph::SceneGraphNode &node, const core::String &filename,
								   io::SeekableReadStream *stream) const {
	core::StringMap<core::String> values;
	core::String currentLine;
	core::String key;
	core::String value;
	bool inMultiline = false;
	core::String multilineValue;

	while (!stream->eos()) {
		core::String line;
		if (!stream->readLine(line)) {
			break;
		}
		line = line.trim();
		if (line.empty())
			continue;

		if (inMultiline) {
			// continue collecting multiline value
			if (core::string::endsWith(line, ")")) {
				inMultiline = false;
				multilineValue += extractQuotedString(line.substr(0, line.size() - 1).trim());
				values.put(key, multilineValue.trim());
				multilineValue.clear();
			} else {
				multilineValue += extractQuotedString(line);
			}
			continue;
		}

		// Regular key = value line
		const size_t equalIdx = line.find("=");
		if (equalIdx == core::String::npos) {
			Log::debug("Invalid line (no '='): %s", line.c_str());
			continue;
		}
		key = line.substr(0, equalIdx).trim();
		value = line.substr(equalIdx + 1).trim();

		// handle multiline value
		if (core::string::startsWith(value, "(")) {
			inMultiline = true;
			value = value.substr(1).trim(); // strip '('
			if (core::string::endsWith(value, ")")) {
				inMultiline = false;
				value = value.substr(0, value.size() - 1).trim(); // strip ')'
				values.put(key, extractQuotedString(value));
			} else {
				multilineValue = extractQuotedString(value);
			}
		} else {
			// TODO: load the python dict for e.g. extensions to be able to save them afterwards
			// single-line quoted value
			values.put(key, extractQuotedString(value));
		}
	}

	// Now set properties
	node.setProperty(scenegraph::PropTitle, util::getIniSectionValue(values, "name", ""));
	node.setProperty(scenegraph::PropAuthor, util::getIniSectionValue(values, "author", ""));
	node.setProperty(scenegraph::PropVersion, util::getIniSectionValue(values, "version", ""));
	node.setProperty(scenegraph::PropDescription, util::getIniSectionValue(values, "description", ""));
}

size_t AoSVXLFormat::loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
								 const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Failed to open stream for file: %s", filename.c_str());
		return 0;
	}

	size_t mapSize = 0;
	size_t mapHeight = 0;
	uint32_t magic = 0;
	stream->peekUInt32(magic);
	const bool slab5 = magic == FourCC('\x00', '\x20', '\x07', '\x09') || magic == FourCC('\x09', '\x07', '\x20', '\x00');
	if (slab5) {
		stream->skip(4);
		Log::debug("Found slab5 vxl");
		uint32_t width, height;
		if (stream->readUInt32(width) != 0) {
			Log::error("Failed to read width");
			return false;
		}
		if (stream->readUInt32(height) != 0) {
			Log::error("Failed to read height");
			return false;
		}

		if (width != 1024 || height != 1024) {
			Log::error("Invalid dimensions: %u:%u", width, height);
			return false;
		}

		mapSize = width;
		mapHeight = 256;

		glm::dvec3 ipo;
		if (!readVec3(stream, ipo)) {
			Log::error("Failed to read ipo/camera position");
			return false;
		}
		glm::dvec3 ist;
		if (!readVec3(stream, ist)) {
			Log::error("Failed to read ist/unit right vector");
			return false;
		}
		glm::dvec3 ihe;
		if (!readVec3(stream, ihe)) {
			Log::error("Failed to read ihe/unit down vector");
			return false;
		}
		glm::dvec3 ifo;
		if (!readVec3(stream, ifo)) {
			Log::error("Failed to read ifo/unit forward vector");
			return false;
		}
	}
	const int64_t size = stream->remaining();
	uint8_t *data = (uint8_t *)core_malloc(size);
	if (stream->read(data, size) == -1) {
		Log::error("Failed to read vxl stream for %s of size %i", filename.c_str(), (int)size);
		core_free(data);
		return 0;
	}

	if (!slab5 && !libvxl_size(&mapSize, &mapHeight, data, size)) {
		Log::error("Failed to determine vxl size");
		core_free(data);
		return 0;
	}

	Log::debug("Read vxl of size %i:%i:%i", (int)mapSize, (int)mapHeight, (int)mapSize);

	struct libvxl_map map;

	if (!libvxl_create(&map, mapSize, mapSize, mapHeight, data, size)) {
		Log::error("Failed to create libvxl map");
		libvxl_free(&map);
		core_free(data);
		return 0;
	}

	palette::RGBABuffer colors;
	colors.reserve(mapSize * mapHeight);
	for (int x = 0; x < (int)mapSize; x++) {
		for (int y = 0; y < (int)mapSize; y++) {
			for (int z = 0; z < (int)mapHeight; z++) {
				if (!libvxl_map_issolid(&map, x, y, z)) {
					continue;
				}
				const uint32_t color = libvxl_map_get(&map, x, y, z);
				const core::RGBA rgba = flattenRGB(vxl_red(color), vxl_green(color), vxl_blue(color));
				colors.put(rgba, true);
			}
		}
	}
	libvxl_free(&map);
	core_free(data);

	return createPalette(colors, palette);
}

glm::ivec3 AoSVXLFormat::maxSize() const {
	// TODO: VOXELFORMAT: slab5 with voxelstein3d has 1024,256,1024
	return glm::ivec3(512, 256, 512);
}

bool AoSVXLFormat::singleVolume() const {
	return true;
}

bool AoSVXLFormat::saveMetadataTxt(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
								   const io::ArchivePtr &archive) const {
	const core::String &metadataFilename = core::string::replaceExtension(filename, ".txt");
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(metadataFilename));
	if (!stream) {
		Log::error("Failed to open stream for file: %s", metadataFilename.c_str());
		return false;
	}

	const scenegraph::SceneGraphNode &node = sceneGraph.root();
	const core::String title = node.property(scenegraph::PropTitle);
	stream->writeStringFormat(false, "name = '%s'\n", title.c_str());
	const core::String author = node.property(scenegraph::PropAuthor);
	stream->writeStringFormat(false, "author = '%s'\n", author.c_str());
	const core::String version = node.property(scenegraph::PropVersion);
	stream->writeStringFormat(false, "version = '%s'\n", version.c_str());
	const core::String description = node.property(scenegraph::PropDescription);
	stream->writeStringFormat(false, "description = '%s'\n", description.c_str());
	// TODO: VOXELFORMAT: save extensions
	// TODO: VOXELFORMAT: save script
	return true;
}

bool AoSVXLFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
							  const io::ArchivePtr &archive, const SaveContext &ctx) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Failed to open stream for file: %s", filename.c_str());
		return false;
	}
	const voxel::Region &region = sceneGraph.region();
	glm::ivec3 size = region.getDimensionsInVoxels();
	glm::ivec3 targetSize(512, size.y, 512);
	if (targetSize.y <= 64) {
		targetSize.y = 64;
	} else if (targetSize.y <= 256) {
		targetSize.y = 256;
	} else {
		Log::error("Volume height exceeds the max allowed height of 256 voxels: %i", targetSize.y);
		return false;
	}
	const int mapSize = targetSize.x;
	const int mapHeight = targetSize.y;

	Log::debug("Save vxl of size %i:%i:%i", mapSize, mapHeight, mapSize);

	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	if (!node) {
		Log::error("No model node found in scene graph");
		return false;
	}

	struct libvxl_map map;
	if (!libvxl_create(&map, mapSize, mapSize, mapHeight, nullptr, 0)) {
		Log::error("Failed to create libvxl map");
		return false;
	}
	const palette::Palette &palette = node->palette();
	const voxel::RawVolume *v = node->volume();
	auto func = [&map, &palette, mapHeight](int x, int y, int z, const voxel::Voxel &voxel) {
		const core::RGBA rgba = palette.color(voxel.getColor());
		const uint32_t color = vxl_color(rgba);
		libvxl_map_set(&map, x, z, mapHeight - 1 - y, color);
	};
	voxelutil::visitVolume(*v, func);

	uint8_t buf[4096];
	struct libvxl_stream s;
	libvxl_stream(&s, &map, sizeof(buf));
	for (;;) {
		size_t read = libvxl_stream_read(&s, buf);
		if (read == 0) {
			break;
		}
		if (stream->write(buf, read) == -1) {
			Log::error("Could not write AoE vxl file to stream");
			libvxl_stream_free(&s);
			libvxl_free(&map);
			return false;
		}
	}
	libvxl_stream_free(&s);
	libvxl_free(&map);

	saveMetadataTxt(sceneGraph, filename, archive);

	return true;
}

#undef wrap

} // namespace voxelformat
