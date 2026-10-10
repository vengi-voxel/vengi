/**
 * @file
 */

#include "AoSFormat.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/collection/Buffer.h"
#include "io/MemoryArchive.h"
#include "io/Stream.h"
#include "json/JSON.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNodeProperties.h"

namespace voxelformat {
namespace {

constexpr uint32_t VXLTag = FourCC('V', 'X', 'L', '\0');
constexpr uint32_t UGCTag = FourCC('U', 'G', 'C', '\0');
constexpr const char *TerrainFile = "map.vxl";
constexpr const char *MetadataProperty = "aos_ugc";

io::MemoryArchivePtr readContainer(const core::String &filename, const io::ArchivePtr &archive, core::String &metadata) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream || stream->size() > 64 * 1024 * 1024) {
		return {};
	}
	io::MemoryArchivePtr contents = io::openMemoryArchive();
	bool hasTerrain = false;
	bool hasMetadata = false;
	// Retail replaces repeated chunks and stops once both kinds have been read.
	while (!hasTerrain || !hasMetadata) {
		uint32_t tag, length;
		if (stream->readUInt32(tag) != 0 || stream->readUInt32(length) != 0 ||
			length > stream->remaining()) {
			Log::error("Invalid AoS chunk header or length");
			return {};
		}
		if (tag == VXLTag) {
			if (length == 0) {
				return {};
			}
			core::Buffer<uint8_t> data(length);
			if (stream->read(data.data(), length) != (int)length) {
				return {};
			}
			contents->remove(TerrainFile);
			contents->add(TerrainFile, data.data(), length);
			hasTerrain = true;
		} else if (tag == UGCTag) {
			if (length == 0 || length > 1024 * 1024 || !stream->readString(length, metadata, false)) {
				return {};
			}
			hasMetadata = true;
		} else {
			Log::error("Unknown AoS chunk tag");
			return {};
		}
	}
	if (!json::Json::parse(metadata).isObject()) {
		Log::error("AoS UGC metadata must be a JSON object");
		return {};
	}
	return contents;
}

} // namespace

size_t AoSFormat::loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
							 const LoadContext &ctx) {
	core::String metadata;
	const io::MemoryArchivePtr contents = readContainer(filename, archive, metadata);
	return contents ? AoSVXLFormat::loadPalette(TerrainFile, contents, palette, ctx) : 0;
}

bool AoSFormat::loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive,
							  scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette, const LoadContext &ctx) {
	core::String metadata;
	const io::MemoryArchivePtr contents = readContainer(filename, archive, metadata);
	if (!contents || !AoSVXLFormat::loadGroupsRGBA(TerrainFile, contents, sceneGraph, palette, ctx)) {
		return false;
	}
	const json::Json ugc = json::Json::parse(metadata);
	scenegraph::SceneGraphNode &root = sceneGraph.node(sceneGraph.root().id());
	// Keep the source JSON verbatim: publishing handles can exceed double precision.
	root.setProperty(MetadataProperty, metadata);
	root.setProperty(scenegraph::PropTitle, ugc.strVal("title", ""));
	root.setProperty(scenegraph::PropAuthor, ugc.strVal("author", ""));
	root.setProperty(scenegraph::PropDescription, ugc.strVal("description", ""));
	sceneGraph.firstModelNode()->setName(ugc.strVal("title", filename.c_str()));
	return true;
}

bool AoSFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
						  const io::ArchivePtr &archive, const SaveContext &ctx) {
	core::String metadata = sceneGraph.root().property(MetadataProperty);
	if (metadata.empty()) {
		json::Json ugc = json::Json::object();
		ugc.set("title", sceneGraph.root().property(scenegraph::PropTitle));
		ugc.set("author", sceneGraph.root().property(scenegraph::PropAuthor));
		ugc.set("description", sceneGraph.root().property(scenegraph::PropDescription));
		ugc.set("ugc_entities", json::Json::array());
		metadata = ugc.dump();
	}
	if (metadata.size() > 1024 * 1024 || !json::Json::parse(metadata).isObject()) {
		return false;
	}
	const io::MemoryArchivePtr contents = io::openMemoryArchive();
	if (!AoSVXLFormat::saveGroups(sceneGraph, TerrainFile, contents, ctx)) {
		return false;
	}
	core::ScopedPtr<io::SeekableReadStream> terrain(contents->readStream(TerrainFile));
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!terrain || !stream || terrain->size() + metadata.size() + 16 > 64 * 1024 * 1024) {
		return false;
	}
	if (!stream->writeUInt32(VXLTag) || !stream->writeUInt32((uint32_t)terrain->size()) ||
		!stream->writeStream(*terrain) || !stream->writeUInt32(UGCTag) ||
		!stream->writeUInt32((uint32_t)metadata.size())) {
		return false;
	}
	return stream->write(metadata.c_str(), metadata.size()) == (int)metadata.size();
}

} // namespace voxelformat
