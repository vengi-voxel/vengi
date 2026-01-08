/**
 * @file
 */

#include "VMaxFormat.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "image/Image.h"
#include "io/Archive.h"
#include "io/LZFSEReadStream.h"
#include "io/MemoryReadStream.h"
#include "io/Stream.h"
#include "io/ZipArchive.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/Morton.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelformat/Format.h"
#include <glm/common.hpp>
#include "json/JSON.h"

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/transform.hpp>

namespace voxelformat {

#define jsonVec(json, name, obj)                                                                                       \
	if ((json).find(#name) == (json).end() || !(json)[#name].is_array()) {                                             \
		const std::string dump = (json).dump();                                                                        \
		Log::debug("Failed to parse json array '" #name "': %s", dump.c_str());                                          \
	} else {                                                                                                           \
		for (int i = 0; i < (obj).name.length(); ++i) {                                                                \
			(obj).name[i] = (json)[#name][i].get<float>();                                                             \
		}                                                                                                              \
	}

#define jsonInt(json, name, obj)                                                                                       \
	if ((json).find(#name) == (json).end() || !(json)[#name].is_number_integer()) {                                    \
		const std::string dump = (json).dump();                                                                        \
		Log::debug("Failed to parse json integer '" #name "': %s", dump.c_str());                                        \
	} else {                                                                                                           \
		(obj).name = (json)[#name].get<int>();                                                                         \
	}

#define jsonFloat(json, name, obj)                                                                                     \
	if ((json).find(#name) == (json).end()) {                                                                          \
		const std::string dump = (json).dump();                                                                        \
		Log::debug("Failed to parse json float " #name ": %s", dump.c_str());                                          \
	} else if ((json)[#name].is_number_float()) {                                                                      \
		(obj).name = (json)[#name].get<float>();                                                                       \
	} else if ((json)[#name].is_number_integer()) {                                                                    \
		(obj).name = (float)(json)[#name].get<int>();                                                                  \
	} else {                                                                                                           \
		const std::string dump = (json).dump();                                                                        \
		Log::debug("Failed to parse json float '" #name "': %s", dump.c_str());                                          \
	}

#define jsonBool(json, name, obj)                                                                                      \
	if ((json).find(#name) == (json).end() || !(json)[#name].is_boolean()) {                                           \
		const std::string dump = (json).dump();                                                                        \
		Log::debug("Failed to parse json bool '" #name "': %s", dump.c_str());                                           \
	} else {                                                                                                           \
		(obj).name = (json)[#name].get<bool>();                                                                        \
	}

#define jsonString(json, name, obj)                                                                                    \
	if ((json).find(#name) == (json).end() || !(json)[#name].is_string()) {                                            \
		const std::string dump = (json).dump();                                                                        \
		Log::debug("Failed to parse json string '" #name "': %s", dump.c_str());                                         \
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
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream("scene.json"));
	if (!stream) {
		Log::error("Failed to load scene.json");
		return false;
	}

	core::String jsonStr;
	stream->readString(stream->size(), jsonStr);
	nlohmann::json json = nlohmann::json::parse(jsonStr, nullptr, false, true);
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

	auto groups = json.find("groups");
	if (groups != json.end() && groups->is_array()) {
		for (const auto &obj : groups.value()) {
			VMaxGroup o;
			jsonBool(obj, s, o);
			jsonString(obj, pid, o);
			jsonString(obj, id, o);
			jsonVec(obj, e_c, o);
			jsonVec(obj, e_mi, o);
			jsonVec(obj, e_ma, o);
			jsonVec(obj, t_p, o);
			jsonVec(obj, t_s, o);
			jsonVec(obj, t_r, o);
			o.e_c = glm::ceil(o.e_c);
			o.e_mi = glm::ceil(o.e_mi);
			o.e_ma = glm::ceil(o.e_ma);
			scene.groups.push_back(o);
		}
	}

	return true;
}

bool VMaxFormat::loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
								   scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
								   const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	io::ArchivePtr zipArchive = io::openZipArchive(stream);
	VMaxScene scene;
	if (!loadSceneJson(zipArchive, scene)) {
		return false;
	}

	Log::debug("Load %i scene objects", (int)scene.objects.size());
	Log::debug("Load %i scene groups", (int)scene.groups.size());
	const core::String &ext = core::string::extractExtension(filename);
	const core::String &objName = core::string::extractFilenameWithExtension(filename);
	const bool onlyOneObject = ext == "vmaxb";
	for (size_t i = 0; i < scene.groups.size(); ++i) {
		if (stopExecution()) {
			return false;
		}
		const VMaxGroup &obj = scene.groups[i];
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Group, obj.id);
		node.setName(obj.name);
		scenegraph::SceneGraphTransform transform;
		const glm::mat4x4 matrix =
			glm::translate(obj.t_p) * glm::mat4_cast(glm::quat(glm::radians(obj.t_r))) * glm::scale(obj.t_s);
		transform.setLocalMatrix(matrix);
		scenegraph::KeyFrameIndex keyFrameIdx = 0;
		node.setTransform(keyFrameIdx, transform);
		if (obj.pid.isValid()) {
			node.setProperty(scenegraph::PropParentUUID, obj.pid.str());
		}
		node.setVisible(!obj.s);
		if (sceneGraph.emplace(core::move(node)) == InvalidNodeId) {
			const core::String uuidStr = obj.id.str();
			Log::error("Failed to add group %s to the scene graph", uuidStr.c_str());
			return false;
		}
	}
	for (size_t i = 0; i < scene.objects.size(); ++i) {
		if (stopExecution()) {
			return false;
		}
		const VMaxObject &obj = scene.objects[i];
		if (onlyOneObject && obj.data != objName) {
			Log::debug("Skip to load object %s", obj.data.c_str());
			continue;
		}
		palette::Palette vmaxPalette;
		if (!loadPaletteFromArchive(zipArchive, obj.pal, vmaxPalette, ctx)) {
			return false;
		}
		if (!loadObjectFromArchive(filename, zipArchive, sceneGraph, ctx, obj, vmaxPalette)) {
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

VMaxFormat::VolumeStats VMaxFormat::parseStats(const util::BinaryPList &snapshot) const {
	VolumeStats volumeStats;
	const util::BinaryPList &stats = snapshot.getDictEntry("st");
	const util::BinaryPList &extent = stats.getDictEntry("extent");
	volumeStats.count = (int)stats.getDictEntry("count").asInt();
	volumeStats.scount = (int)stats.getDictEntry("scount").asInt();
	const util::PListArray &statsMins = stats.getDictEntry("min").asArray();
	const util::PListArray &statsMaxs = stats.getDictEntry("max").asArray();
	const util::PListArray &statsSmins = stats.getDictEntry("smin").asArray();
	const util::PListArray &statsSmaxs = stats.getDictEntry("smax").asArray();
	for (int i = 0; i < 4; ++i) {
		volumeStats.min[i] = (int)statsMins[i].asInt();
		volumeStats.max[i] = (int)statsMaxs[i].asInt();
		volumeStats.smin[i] = (int)statsSmins[i].asInt();
		volumeStats.smax[i] = (int)statsSmaxs[i].asInt();
	}
	// TODO: VOXELFORMAT: is this extent.mins/maxs ?? volumeStats.emin
	// TODO: VOXELFORMAT: is this extent.mins/maxs ?? volumeStats.emax
	volumeStats.extent.o = (int)extent.getDictEntry("o").asInt();
	// const util::BinaryPList &regionBounds = extent.getDictEntry("r");
	// const util::PListArray &extentMins = regionBounds.getDictEntry("min").asArray();
	// const util::PListArray &extentMaxs = regionBounds.getDictEntry("max").asArray();
	// for (int i = 0; i < 3; ++i) {
	// 	volumeStats.extent.min[i] = (int)extentMins[i].asInt();
	// 	volumeStats.extent.max[i] = (int)extentMaxs[i].asInt();
	// }

	return volumeStats;
}

VMaxFormat::VolumeId VMaxFormat::parseId(const util::BinaryPList &snapshot) const {
	VolumeId volumeId;
	const util::BinaryPList &identifier = snapshot.getDictEntry("id");
	const util::BinaryPList &identifierC = identifier.getDictEntry("c");
	const util::BinaryPList &identifierS = identifier.getDictEntry("s");
	const util::BinaryPList &identifierT = identifier.getDictEntry("t");

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
									   const VMaxObject &obj, const palette::Palette &palette) const {
	core::ScopedPtr<io::SeekableReadStream> data(archive->readStream(obj.data));
	if (!data) {
		Log::error("Failed to load %s", obj.data.c_str());
		return false;
	}
	if (data->seek(0) == -1) {
		Log::error("Failed to seek to the beginning of the sub stream");
		return false;
	}

	io::LZFSEReadStream stream(*data);

	// io::filesystem()->write(filename + ".plist", stream);
	// stream.seek(0);

	util::BinaryPList plist = util::BinaryPList::parse(stream);
	if (!plist.isDict()) {
		Log::error("Expected a bplist dict");
		return false;
	}

	const util::PListDict &dict = plist.asDict();
	auto snapshots = dict.find("snapshots");
	if (snapshots == dict.end()) {
		Log::error("No 'snapshots' node found in bplist");
		return false;
	}
	if (!snapshots->value.isArray()) {
		Log::error("Node 'snapshots' has unexpected type");
		return false;
	}
	const util::PListArray &snapshotsArray = snapshots->value.asArray();
	if (snapshotsArray.empty()) {
		Log::debug("Node 'snapshots' is empty");
		return true;
	}

	int parent = sceneGraph.root().id();
	if (obj.pid.isValid()) {
		if (scenegraph::SceneGraphNode *parentNode = sceneGraph.findNodeByUUID(obj.pid)) {
			parent = parentNode->id();
		}
	}

	scenegraph::SceneGraph objectSceneGraph;
	for (size_t i = 0; i < snapshotsArray.size(); ++i) {
		Log::debug("Load snapshot %i of %i", (int)i, (int)snapshotsArray.size());
		const util::BinaryPList &snapshot = snapshotsArray[i].getDictEntry("s");
		if (snapshot.empty()) {
			Log::error("Node 'snapshots' child %i doesn't contain node 's'", (int)i);
			return false;
		}

		// const util::BinaryPList &deselectedLayerColorUsage = snapshot.getDictEntry("dlc");
		const util::BinaryPList &dsData = snapshot.getDictEntry("ds");
		// const util::BinaryPList &layerColorUsage = snapshot.getDictEntry("lc");
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

		const size_t dsSize = dsData.size();
		if (dsSize == 0u) {
			Log::error("Node 'ds' is empty");
			return false;
		}

		io::MemoryReadStream dsStream(dsData.asData().data(), dsSize);
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
			uint8_t material;
			// palette index 0 means air
			uint8_t palIdx;
			wrap(dsStream.readUInt8(material))
			wrap(dsStream.readUInt8(palIdx))
			if (palIdx == 0) {
				++mortonIdx;
				continue;
			}
			uint8_t x, y, z;
			// the voxels are stored in morton order - use the index to find the voxel position
			// y and z are swapped here
			if (!voxel::mortonIndexToCoord(mortonStartIdx + mortonIdx, x, z, y)) {
				Log::error("Failed to lookup voxel position for morton index %i", mortonIdx);
				return false;
			}
			++mortonIdx;
			if (!wrapper.setVoxel(x, y, z,
								  voxel::createVoxel(palette, palIdx))) {
				Log::warn("Failed to set voxel at %i, %i, %i (morton index: %u)", x, y,
						  z, mortonIdx);
			}
		}
		const glm::ivec3 mins(chunkX * maxChunkSize, chunkY * maxChunkSize, chunkZ * maxChunkSize);
		v->translate(mins);

		if (objectSceneGraph.emplace(core::move(node)) == InvalidNodeId) {
			return false;
		}
	}
	const scenegraph::SceneGraph::MergeResult &merged = objectSceneGraph.merge();
	if (!merged.hasVolume()) {
		Log::error("No volumes found in the scene graph");
		return false;
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model, obj.id);
	node.setName(obj.n);

	scenegraph::SceneGraphTransform transform;
	const glm::mat4x4 matrix =
		glm::translate(obj.t_p) * glm::mat4_cast(glm::quat(glm::radians(obj.t_r))) * glm::scale(obj.t_s);
	transform.setLocalMatrix(matrix);

	scenegraph::KeyFrameIndex keyFrameIdx = 0;
	node.setTransform(keyFrameIdx, transform);
	if (obj.pid.isValid()) {
		node.setProperty(scenegraph::PropParentUUID, obj.pid.str());
	}
	node.setVisible(!obj.h);
	node.setVolume(merged.volume(), true);
	node.setPalette(merged.palette);
	node.setNormalPalette(merged.normalPalette);
	return sceneGraph.emplace(core::move(node), parent) != InvalidNodeId;
}

image::ImagePtr VMaxFormat::loadScreenshot(const core::String &filename, const io::ArchivePtr &archive,
										   const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return image::ImagePtr();
	}

	const core::String &thumbnailPath = core::string::path("QuickLook", "Thumbnail.png");
	core::ScopedPtr<io::SeekableReadStream> thumbnailStream;
	const io::ArchivePtr &zipArchive = io::openZipArchive(stream);
	if (zipArchive) {
		thumbnailStream = zipArchive->readStream(thumbnailPath);
	} else {
		const core::String fullPath = core::string::path(core::string::extractDir(filename), thumbnailPath);
		thumbnailStream = archive->readStream(fullPath);
	}
	if (!thumbnailStream) {
		Log::error("Failed to load %s from %s", thumbnailPath.c_str(), filename.c_str());
		return image::ImagePtr();
	}
	Log::debug("Found thumbnail %s in archive %s", thumbnailPath.c_str(), filename.c_str());

	const core::String &name = core::string::extractFilenameWithExtension(thumbnailPath);
	return image::loadImage(name, *thumbnailStream);
}

bool VMaxFormat::loadPaletteFromArchive(const io::ArchivePtr &archive, const core::String &paletteName,
										palette::Palette &palette, const LoadContext &ctx) const {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(paletteName));
	if (!stream) {
		Log::error("Failed to load %s", paletteName.c_str());
		return false;
	}

	const image::ImagePtr &img = image::loadImage(paletteName, *stream);
	if (!img->isLoaded()) {
		Log::error("Failed to load image %s", paletteName.c_str());
		return false;
	}
	if (!palette.load(img)) {
		Log::error("Failed to load palette from image %s", paletteName.c_str());
		return false;
	}

	core::String settingsName = core::string::stripExtension(paletteName);
	settingsName.append(".settings.vmaxpsb");
	core::ScopedPtr<io::SeekableReadStream> paletteSettingsStream(archive->readStream(settingsName));
	if (!paletteSettingsStream) {
		paletteSettingsStream = archive->readStream("palette.settings.vmaxpsb");
	}

	if (paletteSettingsStream) {
		const util::BinaryPList &plist = util::BinaryPList::parse(*paletteSettingsStream);
		if (plist.isDict()) {
			const util::PListDict &dict = plist.asDict();

			const auto &name = plist.getDictEntry("name");
			if (name.isString()) {
				palette.setName(name.asString());
			}

			auto materials = dict.find("materials");
			if (materials != dict.end()) {
				if (materials->value.isArray()) {
					const util::PListArray &materialsArray = materials->value.asArray();
					Log::debug("Found %i materials", (int)materialsArray.size());
					// should always be 8 materials
					for (size_t i = 0; i < materialsArray.size(); ++i) {
						VmaxMaterial vmaxmaterial;
						const auto &material = materialsArray[i];
						// const auto &materialIndex = material.getDictEntry("mi");
						const auto &transmission = material.getDictEntry("tc");
						const auto &emission = material.getDictEntry("sic");
						const auto &roughness = material.getDictEntry("rc");
						const auto &metallic = material.getDictEntry("mc");
						// const auto &enableShadow = material.getDictEntry("sh");
						if (transmission.isReal()) {
							vmaxmaterial.transmission = transmission.asReal();
						}
						if (emission.isReal()) {
							vmaxmaterial.emission = emission.asReal();
						}
						if (roughness.isReal()) {
							vmaxmaterial.roughness = roughness.asReal();
						}
						if (metallic.isReal()) {
							vmaxmaterial.metalness = metallic.asReal();
						}
						// TODO: MATERIAL: use the material properties
					}
				} else {
					Log::debug("Node 'materials' has unexpected type");
				}
			} else {
				Log::debug("No 'materials' node found in bplist");
			}
		}
	} else {
		Log::debug("No 'palette.settings.vmaxpsb' node found in archive");
	}

	return true;
}

size_t VMaxFormat::loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
							   const LoadContext &ctx) {
	// TODO: VOXELFORMAT: there is also a "pal" dict in the vmaxb plist file for some files
	// pal->dict
	//      colors->data
	//      materials->array
	//            dict
	//      name->string
	core::ScopedPtr<io::SeekableReadStream> archiveStream(archive->readStream(filename));
	if (!archiveStream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	io::ArchivePtr zipArchive = io::openZipArchive(archiveStream);
	io::ArchivePtr readArchive = zipArchive ? zipArchive : archive;
	const core::String &paletteName = "palette.png";
	if (zipArchive) {
		Log::debug("Found zip archive %s", filename.c_str());
		if (!loadPaletteFromArchive(readArchive, paletteName, palette, ctx)) {
			Log::error("Failed to load palette from %s", paletteName.c_str());
			return 0u;
		}
	} else {
		const core::String fullPath = core::string::path(core::string::extractDir(filename), paletteName);
		if (!loadPaletteFromArchive(readArchive, fullPath, palette, ctx)) {
			Log::error("Failed to load palette from %s", fullPath.c_str());
			return 0u;
		}
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
