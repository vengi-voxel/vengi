/**
 * @file
 */

#include "VMaxFormat.h"
#include "app/App.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/collection/Array3DView.h"
#include "external/json.hpp"
#include "image/Image.h"
#include "io/Archive.h"
#include "io/BufferedReadWriteStream.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "io/LZFSEReadStream.h"
#include "io/MemoryReadStream.h"
#include "io/StdStreamBuf.h"
#include "io/Stream.h"
#include "io/ZipArchive.h"
#include "private/BinaryPList.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/Morton.h"
#include "voxel/Palette.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelformat/Format.h"
#include <glm/common.hpp>
#include <glm/gtx/transform.hpp>

namespace voxelformat {

#define jsonVec(json, name, obj)                                                                                       \
	if ((json).find(#name) == (json).end() || !(json)[#name].is_array()) {                                             \
		const std::string dump = (json).dump();                                                                        \
		Log::debug("Failed to parse json array " #name ": %s", dump.c_str());                                          \
	} else {                                                                                                           \
		for (int i = 0; i < (obj).name.length(); ++i) {                                                                \
			(obj).name[i] = (json)[#name][i].get<float>();                                                             \
		}                                                                                                              \
	}

#define jsonInt(json, name, obj)                                                                                       \
	if ((json).find(#name) == (json).end() || !(json)[#name].is_number_integer()) {                                    \
		const std::string dump = (json).dump();                                                                        \
		Log::debug("Failed to parse json integer " #name ": %s", dump.c_str());                                        \
	} else {                                                                                                           \
		(obj).name = (json)[#name].get<int>();                                                                         \
	}

#define jsonFloat(json, name, obj)                                                                                     \
	if ((json).find(#name) == (json).end() || !(json)[#name].is_number_float()) {                                      \
		const std::string dump = (json).dump();                                                                        \
		Log::debug("Failed to parse json float " #name ": %s", dump.c_str());                                          \
	} else {                                                                                                           \
		(obj).name = (json)[#name].get<float>();                                                                       \
	}

#define jsonBool(json, name, obj)                                                                                      \
	if ((json).find(#name) == (json).end() || !(json)[#name].is_boolean()) {                                           \
		const std::string dump = (json).dump();                                                                        \
		Log::debug("Failed to parse json bool " #name ": %s", dump.c_str());                                           \
	} else {                                                                                                           \
		(obj).name = (json)[#name].get<bool>();                                                                        \
	}

#define jsonString(json, name, obj)                                                                                    \
	if ((json).find(#name) == (json).end() || !(json)[#name].is_string()) {                                            \
		const std::string dump = (json).dump();                                                                        \
		Log::debug("Failed to parse json string " #name ": %s", dump.c_str());                                         \
	} else {                                                                                                           \
		(obj).name = (json)[#name].get<std::string>().c_str();                                                         \
	}

#define wrap(action)                                                                                                   \
	if ((action) == -1) {                                                                                              \
		Log::error("Error: Failed to execute " CORE_STRINGIFY(action) " (line %i)", (int)__LINE__);                    \
		return false;                                                                                                  \
	}

namespace vmax {
constexpr int MaxVolumeSize = 256u;
} // namespace vmax

bool VMaxFormat::loadSceneJson(const io::ArchivePtr &archive, VMaxScene &scene) const {
	io::BufferedReadWriteStream stream;
	if (!archive->load("scene.json", stream)) {
		Log::error("Failed to load scene.json");
		return false;
	}

	stream.seek(0);
	io::StdIStreamBuf buf(stream);
	std::istream is(&buf);
	nlohmann::json json;
	is >> json;
	if (json.is_null()) {
		Log::error("Failed to parse the json");
		return false;
	}

	jsonString(json, af, scene);
	jsonFloat(json, aint, scene);
	jsonFloat(json, eint, scene);
	jsonFloat(json, outlinesz, scene);
	jsonFloat(json, sat, scene);
	jsonFloat(json, shadowint, scene);
	jsonFloat(json, temp, scene);
	jsonFloat(json, cont, scene);
	jsonFloat(json, tint, scene);
	jsonString(json, background, scene);
	jsonString(json, lcolor, scene);
	jsonFloat(json, bloombrad, scene);
	jsonFloat(json, bloomint, scene);
	jsonFloat(json, bloomthr, scene);
	jsonInt(json, v, scene);
	jsonFloat(json, outlineint, scene);
	jsonBool(json, nrn, scene);
	jsonBool(json, ssr, scene);
	jsonFloat(json, lint, scene);

	auto objects = json.find("objects");
	if (objects == json.end() || !objects->is_array()) {
		Log::error("Failed to parse the scene json - expected an array of objects");
		return false;
	}
	for (const auto &obj : objects.value()) {
		VMaxObject o;
		jsonBool(obj, s, o);
		jsonBool(obj, h, o);
		jsonString(obj, n, o);
		jsonString(obj, data, o);
		jsonString(obj, pal, o);
		jsonString(obj, pid, o);
		jsonString(obj, hist, o);
		jsonString(obj, id, o);
		jsonString(obj, t_al, o);
		jsonString(obj, t_pa, o);
		jsonString(obj, t_po, o);
		jsonString(obj, t_pf, o);
		jsonVec(obj, ind, o);
		jsonVec(obj, e_c, o);
		jsonVec(obj, e_mi, o);
		jsonVec(obj, e_ma, o);
		jsonVec(obj, t_p, o);
		jsonVec(obj, t_s, o);
		jsonVec(obj, t_r, o);
		o.e_c = glm::ceil(o.e_c);
		o.e_mi = glm::ceil(o.e_mi);
		o.e_ma = glm::ceil(o.e_ma);
		scene.objects.push_back(o);
	}

	return true;
}

bool VMaxFormat::loadGroupsPalette(const core::String &filename, io::SeekableReadStream &stream,
								   scenegraph::SceneGraph &sceneGraph, voxel::Palette &palette,
								   const LoadContext &ctx) {
	VMaxScene scene;
	const io::ArchivePtr &archive = io::openArchive(filename, &stream);
	if (!archive) {
		Log::error("Failed to create archive for %s", filename.c_str());
		return false;
	}
	if (!loadSceneJson(archive, scene)) {
		return false;
	}

	Log::debug("Load %i scene objects", (int)scene.objects.size());
	const core::String &ext = core::string::extractExtension(filename);
	const core::String &objName = core::string::extractFilenameWithExtension(filename);
	const bool onlyOneObject = ext == "vmaxb";
	for (size_t i = 0; i < scene.objects.size(); ++i) {
		if (stopExecution()) {
			return false;
		}
		const VMaxObject &obj = scene.objects[i];
		if (onlyOneObject && obj.data != objName) {
			Log::debug("Skip to load object %s", obj.data.c_str());
			continue;
		}
		voxel::Palette palette;
		if (!loadPaletteFromArchive(archive, obj.pal, palette, ctx)) {
			return false;
		}
		if (!loadObjectFromArchive(filename, archive, sceneGraph, ctx, obj, palette)) {
			Log::error("Failed to load object %s", obj.n.c_str());
			return false;
		}
		Log::debug("Load scene object %i of %i", (int)i, (int)scene.objects.size());
		if (onlyOneObject) {
			break;
		}
	}
	return true;
}

VMaxFormat::VolumeStats VMaxFormat::parseStats(const priv::BinaryPList &snapshot) const {
	VolumeStats volumeStats;
	const priv::BinaryPList &stats = snapshot.getDictEntry("st");
	const priv::BinaryPList &extent = stats.getDictEntry("extent");
	volumeStats.count = (int)stats.getDictEntry("count").asInt();
	volumeStats.scount = (int)stats.getDictEntry("scount").asInt();
	const priv::PListArray &statsMins = stats.getDictEntry("min").asArray();
	const priv::PListArray &statsMaxs = stats.getDictEntry("max").asArray();
	const priv::PListArray &statsSmins = stats.getDictEntry("smin").asArray();
	const priv::PListArray &statsSmaxs = stats.getDictEntry("smax").asArray();
	for (int i = 0; i < 4; ++i) {
		volumeStats.min[i] = (int)statsMins[i].asInt();
		volumeStats.max[i] = (int)statsMaxs[i].asInt();
		volumeStats.smin[i] = (int)statsSmins[i].asInt();
		volumeStats.smax[i] = (int)statsSmaxs[i].asInt();
	}
	// TODO: is this extent.mins/maxs ?? volumeStats.emin
	// TODO: is this extent.mins/maxs ?? volumeStats.emax
	volumeStats.extent.o = (int)extent.getDictEntry("o").asInt();
	// const priv::BinaryPList &regionBounds = extent.getDictEntry("r");
	// const priv::PListArray &extentMins = regionBounds.getDictEntry("min").asArray();
	// const priv::PListArray &extentMaxs = regionBounds.getDictEntry("max").asArray();
	// for (int i = 0; i < 3; ++i) {
	// 	volumeStats.extent.min[i] = (int)extentMins[i].asInt();
	// 	volumeStats.extent.max[i] = (int)extentMaxs[i].asInt();
	// }

	return volumeStats;
}

VMaxFormat::VolumeId VMaxFormat::parseId(const priv::BinaryPList &snapshot) const {
	VolumeId volumeId;
	const priv::BinaryPList &identifier = snapshot.getDictEntry("id");
	const priv::BinaryPList &identifierC = identifier.getDictEntry("c");
	const priv::BinaryPList &identifierS = identifier.getDictEntry("s");
	const priv::BinaryPList &identifierT = identifier.getDictEntry("t");

	if (identifierC.isInt()) {
		volumeId.mortonChunkIdx = (int)identifierC.asInt();
	}
	if (identifierS.isInt()) {
		volumeId.idTimeline = (int)identifierS.asInt();
	}
	if (identifierT.isInt()) {
		volumeId.type = (SnapshotType)identifierT.asUInt8();
	}

	Log::debug("identifier: c(%i), s(%i), t(%i)", volumeId.mortonChunkIdx, volumeId.idTimeline, (int)volumeId.type);

	return volumeId;
}

bool VMaxFormat::loadObjectFromArchive(const core::String &filename, const io::ArchivePtr &archive,
									   scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx,
									   const VMaxObject &obj, const voxel::Palette &palette) const {
	io::BufferedReadWriteStream data;
	if (!archive->load(obj.data, data)) {
		Log::error("Failed to load %s", obj.data.c_str());
		return false;
	}
	if (data.seek(0) == -1) {
		Log::error("Failed to seek to the beginning of the sub stream");
		return false;
	}

	io::LZFSEReadStream stream(data);
	priv::BinaryPList plist = priv::BinaryPList::parse(stream);
	if (!plist.isDict()) {
		Log::error("Expected a bplist dict");
		return false;
	}

	const priv::PListDict &dict = plist.asDict();
	auto snapshots = dict.find("snapshots");
	if (snapshots == dict.end()) {
		Log::error("No 'snapshots' node found in bplist");
		return false;
	}
	if (!snapshots->value.isArray()) {
		Log::error("Node 'snapshots' has unexpected type");
		return false;
	}
	const priv::PListArray &snapshotsArray = snapshots->value.asArray();
	if (snapshotsArray.empty()) {
		Log::debug("Node 'snapshots' is empty");
		return true;
	}

	int parent = sceneGraph.root().id();
	if (!obj.pid.empty()) {
		if (scenegraph::SceneGraphNode *parentNode = sceneGraph.findNodeByPropertyValue("uuid", obj.pid)) {
			parent = parentNode->id();
		}
	}

	scenegraph::SceneGraph objectSceneGraph;
	for (size_t i = 0; i < snapshotsArray.size(); ++i) {
		Log::debug("Load snapshot %i of %i", (int)i, (int)snapshotsArray.size());
		const priv::BinaryPList &snapshot = snapshotsArray[i].getDictEntry("s");
		if (snapshot.empty()) {
			Log::error("Node 'snapshots' child %i doesn't contain node 's'", (int)i);
			return false;
		}

		// const priv::BinaryPList &deselectedLayerColorUsage = snapshot.getDictEntry("dlc");
		const priv::BinaryPList &data = snapshot.getDictEntry("ds");
		// const priv::BinaryPList &layerColorUsage = snapshot.getDictEntry("lc");
		const VolumeId &volumeId = parseId(snapshot);
		const VolumeStats &volumeStats = parseStats(snapshot);
		const VolumeExtent &extent = volumeStats.extent;

		Log::debug("volumestats.extent: mins(%i, %i, %i), maxs(%i, %i, %i)", extent.min[0], extent.min[1],
				   extent.min[2], extent.max[0], extent.max[1], extent.max[2]);

		const int maxChunkSize = 1 << extent.o;
		const int maxVolumeChunks = vmax::MaxVolumeSize / maxChunkSize;
		const int maxChunks = maxVolumeChunks * maxVolumeChunks * maxVolumeChunks;

		if (volumeId.mortonChunkIdx > maxChunks) {
			Log::error("identifier: c(%i) is out of range", volumeId.mortonChunkIdx);
			return false;
		}

		const size_t dsSize = data.size();
		if (dsSize == 0u) {
			Log::error("Node 'ds' is empty");
			return false;
		}

		io::MemoryReadStream dsStream(data.asData().data(), dsSize);
		Log::debug("Found voxel data with size %i", (int)dsStream.size());

		// search the chunk world position by getting the morton index for the snapshot id
		uint8_t chunkX, chunkY, chunkZ;
		// y and z are swapped here
		if (!voxel::mortonIndexToCoord(volumeId.mortonChunkIdx, chunkX, chunkZ, chunkY)) {
			Log::error("Failed to lookup chunk position for morton index %i", volumeId.mortonChunkIdx);
			return false;
		}

		// now loop over the 'voxels' array and create a volume from it
		const voxel::Region region(0, maxChunkSize - 1);
		voxel::RawVolume *v = new voxel::RawVolume(region);
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setVolume(v, true);
		node.setPalette(palette);

		const int mortonStartIdx = volumeStats.min[3];
		uint8_t chunkOffsetX, chunkOffsetY, chunkOffsetZ;
		// y and z are swapped here
		if (!voxel::mortonIndexToCoord(mortonStartIdx, chunkOffsetX, chunkOffsetZ, chunkOffsetY)) {
			Log::error("Failed to get chunk offset from morton index %i", mortonStartIdx);
			return false;
		}
		Log::debug("chunkOffset: %i, %i, %i", chunkOffsetX, chunkOffsetY, chunkOffsetZ);
		uint32_t mortonIdx = 0;
		voxel::RawVolumeWrapper wrapper(v);

		Log::debug("start voxel: %i", volumeStats.scount);
		Log::debug("amount of voxels: %i", volumeStats.count);
		while (!dsStream.eos()) {
			// there are only 8 materials used for now 0-7 and 8 selected versions for them 8-15,
			// with option to add more in the future up to 128
			uint8_t extendedLayerInfo;
			// palette index 0 means air
			// clang-format off
			uint8_t palIdx;
			wrap(dsStream.readUInt8(extendedLayerInfo))
			wrap(dsStream.readUInt8(palIdx))
			if (palIdx == 0) {
				++mortonIdx;
				continue;
			}
			// clang-format on
			uint8_t x, y, z;
			// the voxels are stored in morton order - use the index to find the voxel position
			// y and z are swapped here
			if (!voxel::mortonIndexToCoord(mortonIdx, x, z, y)) {
				Log::error("Failed to lookup voxel position for morton index %i", mortonIdx);
				return false;
			}
			++mortonIdx;
			if (!wrapper.setVoxel(chunkOffsetX + x, chunkOffsetY + y, chunkOffsetZ + z,
								  voxel::createVoxel(voxel::VoxelType::Generic, palIdx))) {
				Log::warn("Failed to set voxel at %i, %i, %i (morton index: %u)", chunkOffsetX + x, chunkOffsetY + y,
						  chunkOffsetZ + z, mortonIdx);
			}
		}
		const glm::ivec3 mins(chunkX * maxChunkSize, chunkY * maxChunkSize, chunkZ * maxChunkSize);
		v->translate(mins);

		if (objectSceneGraph.emplace(core::move(node)) == InvalidNodeId) {
			return false;
		}
	}
	const scenegraph::SceneGraph::MergedVolumePalette &merged = objectSceneGraph.merge();
	if (merged.first == nullptr) {
		Log::error("No volumes found in the scene graph");
		return false;
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setName(obj.n);

	scenegraph::SceneGraphTransform transform;
	const glm::mat4x4 matrix =
		glm::translate(obj.t_p) * glm::mat4_cast(glm::quat(glm::radians(obj.t_r))) * glm::scale(obj.t_s);
	transform.setLocalMatrix(matrix);

	scenegraph::KeyFrameIndex keyFrameIdx = 0;
	node.setTransform(keyFrameIdx, transform);
	node.setProperty("uuid", obj.id);
	if (!obj.pid.empty()) {
		node.setProperty("parent-uuid", obj.pid);
	}
	node.setVisible(!obj.h);
	node.setPalette(merged.second);
	node.setVolume(merged.first, true);
	return sceneGraph.emplace(core::move(node), parent) != InvalidNodeId;
}

image::ImagePtr VMaxFormat::loadScreenshot(const core::String &filename, io::SeekableReadStream &stream,
										   const LoadContext &ctx) {
	const io::ArchivePtr &archive = io::openArchive(filename, &stream);
	if (!archive) {
		Log::error("Failed to create archive for %s", filename.c_str());
		return image::ImagePtr{};
	}
	io::BufferedReadWriteStream contentsStream;
	const core::String &thumbnailPath = core::string::path("QuickLook", "Thumbnail.png");
	if (!archive->load(thumbnailPath, contentsStream)) {
		Log::error("Failed to load %s from %s", thumbnailPath.c_str(), filename.c_str());
		return image::ImagePtr();
	}

	if (contentsStream.seek(0) == -1) {
		Log::error("Failed to seek to the beginning of the sub stream for %s", filename.c_str());
		return image::ImagePtr();
	}
	return image::loadImage(core::string::extractFilenameWithExtension(thumbnailPath), contentsStream);
}

bool VMaxFormat::loadPaletteFromArchive(const io::ArchivePtr &archive, const core::String &paletteName,
										voxel::Palette &palette, const LoadContext &ctx) const {
	io::BufferedReadWriteStream stream;
	if (!archive->load(paletteName, stream)) {
		Log::error("Failed to load %s", paletteName.c_str());
		return false;
	}

	if (stream.seek(0) == -1) {
		Log::error("Failed to seek to the beginning of the sub stream for the palette %s", paletteName.c_str());
		return false;
	}

	const image::ImagePtr &img = image::loadImage(paletteName, stream);
	if (!img->isLoaded()) {
		Log::error("Failed to load image %s", paletteName.c_str());
		return false;
	}
	if (!palette.load(img)) {
		Log::error("Failed to load palette from image %s", paletteName.c_str());
		return false;
	}
	return true;
}

size_t VMaxFormat::loadPalette(const core::String &filename, io::SeekableReadStream &stream, voxel::Palette &palette,
							   const LoadContext &ctx) {
	const io::ArchivePtr &archive = io::openArchive(filename, &stream);
	if (!archive) {
		Log::error("Failed to create archive for %s", filename.c_str());
		return 0u;
	}

	// TODO: there is also a "pal" dict in the vmaxb plist file for some files
	// pal->dict
	//      colors->data
	//      materials->array
	//            dict
	//      name->string

	const core::String &paletteName = "palette.png";
	if (!loadPaletteFromArchive(archive, paletteName, palette, ctx)) {
		return 0u;
	}
	return palette.colorCount();
}

#undef jsonVec
#undef jsonInt
#undef jsonFloat
#undef jsonBool
#undef jsonString
#undef wrap

} // namespace voxelformat
