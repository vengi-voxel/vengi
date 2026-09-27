/**
 * @file
 */

#include "VMaxFormat.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StandardLib.h"
#include "core/StringUtil.h"
#include "image/Image.h"
#include "io/Archive.h"
#include "io/LZFSEReadStream.h"
#include "io/MemoryReadStream.h"
#include "io/Stream.h"
#include "io/ZipArchive.h"
#include "palette/Material.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphTransform.h"
#include "voxel/Morton.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelformat/Format.h"
#include "voxelutil/VolumeCropper.h"
#include "json/JSON.h"
#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace voxelformat {

#define jsonVec(json, name, obj)                                                                                       \
	if (!(json).contains(#name) || !(json).get(#name).isArray()) {                                                     \
		const core::String dump = (json).dump();                                                                       \
		Log::debug("Failed to parse json array '" #name "': %s", dump.c_str());                                          \
	} else {                                                                                                           \
		for (int i = 0; i < (obj).name.length(); ++i) {                                                                \
			(obj).name[i] = (json).get(#name).get(i).floatVal();                                                       \
		}                                                                                                              \
	}

#define jsonInt(json, name, obj)                                                                                       \
	if (!(json).contains(#name) || !(json).get(#name).isNumberInteger()) {                                             \
		const core::String dump = (json).dump();                                                                       \
		Log::debug("Failed to parse json integer '" #name "': %s", dump.c_str());                                        \
	} else {                                                                                                           \
		(obj).name = (json).get(#name).intVal();                                                                       \
	}

#define jsonFloat(json, name, obj)                                                                                     \
	if (!(json).contains(#name)) {                                                                                     \
		const core::String dump = (json).dump();                                                                       \
		Log::debug("Failed to parse json float " #name ": %s", dump.c_str());                                          \
	} else if ((json).get(#name).isNumberFloat()) {                                                                    \
		(obj).name = (json).get(#name).floatVal();                                                                     \
	} else if ((json).get(#name).isNumberInteger()) {                                                                  \
		(obj).name = (float)(json).get(#name).intVal();                                                                \
	} else {                                                                                                           \
		const core::String dump = (json).dump();                                                                       \
		Log::debug("Failed to parse json float '" #name "': %s", dump.c_str());                                          \
	}

#define jsonBool(json, name, obj)                                                                                      \
	if (!(json).contains(#name) || !(json).get(#name).isBool()) {                                                      \
		const core::String dump = (json).dump();                                                                       \
		Log::debug("Failed to parse json bool '" #name "': %s", dump.c_str());                                           \
	} else {                                                                                                           \
		(obj).name = (json).get(#name).boolVal();                                                                      \
	}

#define jsonString(json, name, obj)                                                                                    \
	if (!(json).contains(#name) || !(json).get(#name).isString()) {                                                    \
		const core::String dump = (json).dump();                                                                       \
		Log::debug("Failed to parse json string '" #name "': %s", dump.c_str());                                         \
	} else {                                                                                                           \
		(obj).name = (json).get(#name).str().c_str();                                                                  \
	}

#define wrap(action)                                                                                                   \
	if ((action) == -1) {                                                                                              \
		Log::error("Error: Failed to execute " CORE_STRINGIFY(action) " (line %i)", (int)__LINE__);                    \
		return false;                                                                                                  \
	}

namespace vmax {
constexpr int MaxVolumeSize = 256u;
} // namespace vmax

// VoxelMax volumes are Z-up. Morton decode already stores (x, z_up, y).
static glm::vec3 vmaxToVengi(const glm::vec3 &v) {
	return glm::vec3(v.x, v.z, v.y);
}

static glm::quat vmaxAxisAngleToQuat(const glm::vec4 &t_r) {
	const glm::vec3 axis = vmaxToVengi(glm::vec3(t_r));
	// (x,y,z)->(x,z,y) has det -1, so R conjugates as R(C*axis, -angle).
	const float angle = -t_r.w;
	const float len2 = glm::dot(axis, axis);
	if (len2 <= 1.0e-12f || glm::abs(angle) <= 1.0e-8f) {
		return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	}
	return glm::angleAxis(angle, axis / glm::sqrt(len2));
}

static core::String vmaxPaletteFileForContents(const core::String &contentsPath) {
	const core::String name = core::string::extractFilenameWithExtension(contentsPath);
	int idx = 0;
	if (SDL_sscanf(name.c_str(), "contents%i.", &idx) == 1 && idx > 0) {
		return core::String::format("palette%i.png", idx);
	}
	return "palette.png";
}

static core::String vmaxJoin(const core::String &dir, const core::String &name) {
	if (name.empty()) {
		return name;
	}
	if (dir.empty()) {
		return name;
	}
	return core::string::path(dir, name);
}

static bool vmaxPlistNumber(const util::BinaryPList &value, double &out) {
	if (value.isReal()) {
		out = value.asReal();
		return true;
	}
	if (value.isInt()) {
		out = (double)value.asInt();
		return true;
	}
	return false;
}

// encodes as '0'..'8'. material index '1'..'8' map to voxel bytes 0..7.
static bool vmaxParseLayerIndex(const util::BinaryPList &value, int &layer) {
	if (value.isString()) {
		const core::String &s = value.asString();
		if (s.size() == 1u) {
			const char c = s.c_str()[0];
			if (c >= '1' && c <= '8') {
				layer = (int)(c - '1');
				return true;
			}
		}
		return false;
	}
	if (value.isInt()) {
		const int v = (int)value.asInt();
		if (v >= 1 && v <= 8) {
			layer = v - 1;
			return true;
		}
	}
	return false;
}

// NSFileWrapper packages are a directory (Name.vmax/) or a zip. scene.json is the catalog.
static bool vmaxIsPackage(const io::ArchivePtr &archive, const core::String &filename, core::String &packageDir) {
	const core::String dirScene = vmaxJoin(filename, "scene.json");
	if (archive->exists(dirScene)) {
		packageDir = filename;
		return true;
	}
	if (core::string::extractExtension(filename) == "vmaxb") {
		const core::String dir = core::string::extractDir(filename);
		if (archive->exists(vmaxJoin(dir, "scene.json"))) {
			packageDir = dir;
			return true;
		}
	}
	return false;
}

void VMaxFormat::applySceneTransform(scenegraph::SceneGraphNode &node, const glm::vec3 &t_p, const glm::vec4 &t_r,
									 const glm::vec3 &t_s) const {
	applySceneTransform(node, t_p, t_r, t_s, glm::vec3(0.0f));
}

void VMaxFormat::applySceneTransform(scenegraph::SceneGraphNode &node, const glm::vec3 &t_p, const glm::vec4 &t_r,
									 const glm::vec3 &t_s, const glm::vec3 &volumeMins) const {
	scenegraph::SceneGraphTransform transform;
	// VoxelMax uses T * R * S from t_p/t_r/t_s and applies that matrix on the parent chain.
	// Decoder stores e_c but does not fold it into the matrix.
	// Voxels stay in the 256 work-area; crop + T(volumeMins) restores those coords.
	// vmaxToVengi is (x,z,y) (det -1); vmaxAxisAngleToQuat uses -t_r.w so R conjugates.
	const glm::mat4 local = glm::translate(glm::mat4(1.0f), vmaxToVengi(t_p)) *
							glm::mat4_cast(vmaxAxisAngleToQuat(t_r)) *
							glm::scale(glm::mat4(1.0f), vmaxToVengi(t_s)) *
							glm::translate(glm::mat4(1.0f), volumeMins);
	transform.setLocalMatrix(local);
	const scenegraph::KeyFrameIndex keyFrameIdx = 0;
	node.setTransform(keyFrameIdx, transform);
}

bool VMaxFormat::loadSceneJson(const io::ArchivePtr &archive, VMaxScene &scene,
							   const core::String &sceneJsonPath) const {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(sceneJsonPath));
	if (!stream) {
		Log::error("Failed to load %s", sceneJsonPath.c_str());
		return false;
	}

	core::String jsonStr;
	stream->readString(stream->size(), jsonStr);
	json::Json json = json::Json::parse(jsonStr);
	if (json.isNull()) {
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

	if (!json.contains("objects") || !json.get("objects").isArray()) {
		Log::error("Failed to parse the scene json - expected an array of objects");
		return false;
	}
	const json::Json objects = json.get("objects");
	for (const json::Json &obj : objects) {
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
		if (o.n.empty() && !o.data.empty()) {
			o.n = core::string::extractFilename(o.data);
		}
		scene.objects.push_back(o);
	}

	if (json.contains("groups") && json.get("groups").isArray()) {
		const json::Json groups = json.get("groups");
		for (const json::Json &obj : groups) {
			VMaxGroup o;
			jsonBool(obj, s, o);
			jsonBool(obj, h, o);
			jsonString(obj, name, o);
			jsonString(obj, pid, o);
			jsonString(obj, id, o);
			jsonVec(obj, e_c, o);
			jsonVec(obj, e_mi, o);
			jsonVec(obj, e_ma, o);
			jsonVec(obj, t_p, o);
			jsonVec(obj, t_s, o);
			jsonVec(obj, t_r, o);
			scene.groups.push_back(o);
		}
	}

	return true;
}

bool VMaxFormat::loadScenePackage(const io::ArchivePtr &archive, const core::String &packageDir,
								  scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	VMaxScene scene;
	if (!loadSceneJson(archive, scene, vmaxJoin(packageDir, "scene.json"))) {
		return false;
	}

	Log::debug("Load %i scene objects", (int)scene.objects.size());
	Log::debug("Load %i scene groups", (int)scene.groups.size());
	for (VMaxObject &obj : scene.objects) {
		obj.data = vmaxJoin(packageDir, obj.data);
		obj.pal = vmaxJoin(packageDir, obj.pal);
	}
	for (size_t i = 0; i < scene.groups.size(); ++i) {
		ctx.report("group", (int)i, (int)scene.groups.size());
		if (stopExecution()) {
			return false;
		}
		const VMaxGroup &obj = scene.groups[i];
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Group, obj.id);
		node.setName(obj.name);
		applySceneTransform(node, obj.t_p, obj.t_r, obj.t_s);
		node.setVisible(!obj.h);
		if (sceneGraph.emplace(core::move(node)) == InvalidNodeId) {
			const core::String uuidStr = obj.id.str();
			Log::error("Failed to add group %s to the scene graph", uuidStr.c_str());
			return false;
		}
	}
	for (const VMaxGroup &obj : scene.groups) {
		if (!obj.pid.isValid()) {
			continue;
		}
		scenegraph::SceneGraphNode *node = sceneGraph.findNodeByUUID(obj.id);
		scenegraph::SceneGraphNode *parent = sceneGraph.findNodeByUUID(obj.pid);
		if (node == nullptr || parent == nullptr) {
			const core::String uuidStr = obj.id.str();
			const core::String parentStr = obj.pid.str();
			Log::warn("Could not parent group %s to %s", uuidStr.c_str(), parentStr.c_str());
			continue;
		}
		if (!sceneGraph.changeParent(node->id(), parent->id(), scenegraph::NodeMoveFlag::None)) {
			const core::String uuidStr = obj.id.str();
			Log::warn("Failed to reparent group %s", uuidStr.c_str());
		}
	}
	ctx.report("group", (int)scene.groups.size(), (int)scene.groups.size());
	for (size_t i = 0; i < scene.objects.size(); ++i) {
		ctx.report("object", (int)i, (int)scene.objects.size());
		if (stopExecution()) {
			return false;
		}
		const VMaxObject &obj = scene.objects[i];
		if (obj.data.empty()) {
			Log::error("Scene object %i has no contents path", (int)i);
			continue;
		}
		if (obj.pal.empty()) {
			Log::error("Failed to load object %s: empty palette path", obj.n.c_str());
			continue;
		}
		palette::Palette vmaxPalette;
		VmaxLayerMaterials layers;
		if (!loadPaletteFromArchive(archive, obj.pal, vmaxPalette, ctx, layers)) {
			Log::error("Failed to load palette %s for object %s", obj.pal.c_str(), obj.n.c_str());
			continue;
		}
		if (!loadObjectFromArchive(obj.data, archive, sceneGraph, ctx, obj, vmaxPalette, layers)) {
			Log::error("Failed to load object %s", obj.n.c_str());
			continue;
		}
		Log::debug("Load scene object %i of %i", (int)i, (int)scene.objects.size());
	}
	ctx.report("object", (int)scene.objects.size(), (int)scene.objects.size());
	return true;
}

bool VMaxFormat::loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
								   scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
								   const LoadContext &ctx) {
	core::String packageDir;
	if (vmaxIsPackage(archive, filename, packageDir)) {
		Log::debug("Load VoxelMax package from %s", vmaxJoin(packageDir, "scene.json").c_str());
		return loadScenePackage(archive, packageDir, sceneGraph, ctx);
	}

	const core::String &ext = core::string::extractExtension(filename);
	const bool onlyOneObject = ext == "vmaxb";
	if (onlyOneObject) {
		core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
		if (!stream) {
			Log::error("Could not load file %s", filename.c_str());
			return false;
		}
		palette::Palette vmaxPalette;
		VmaxLayerMaterials layers;
		const core::String palFile = vmaxPaletteFileForContents(filename);
		const core::String palPath = vmaxJoin(core::string::extractDir(filename), palFile);
		Log::debug("Standalone vmaxb palette: %s", palPath.c_str());
		if (!loadPaletteFromArchive(archive, palPath, vmaxPalette, ctx, layers) &&
			(palPath == palFile || !loadPaletteFromArchive(archive, palFile, vmaxPalette, ctx, layers))) {
			if (stream->seek(0) == -1 || !loadPaletteFromVmaxb(*stream, vmaxPalette, layers)) {
				return false;
			}
			if (stream->seek(0) == -1) {
				return false;
			}
		}
		VMaxObject obj;
		obj.data = core::string::extractFilenameWithExtension(filename);
		if (!loadObject(filename, stream, sceneGraph, ctx, obj, vmaxPalette, layers)) {
			Log::error("Failed to load object %s", obj.n.c_str());
			return false;
		}
		return true;
	}

	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	io::ArchivePtr zipArchive = io::openZipArchive(stream);
	if (!zipArchive) {
		Log::error("Failed to open VoxelMax archive %s", filename.c_str());
		return false;
	}
	return loadScenePackage(zipArchive, "", sceneGraph, ctx);
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
	// emin/emax are work-area extent when smaller than 256^3, not extent.r
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
									   const VMaxObject &obj, const palette::Palette &palette,
									   const VmaxLayerMaterials &layers) const {
	if (obj.data.empty()) {
		Log::error("Empty object contents path");
		return false;
	}
	core::ScopedPtr<io::SeekableReadStream> data(archive->readStream(obj.data));
	if (!data) {
		Log::error("Failed to load %s", obj.data.c_str());
		return false;
	}
	if (data->seek(0) == -1) {
		Log::error("Failed to seek to the beginning of the sub stream");
		return false;
	}
	return loadObject(filename, data, sceneGraph, ctx, obj, palette, layers);
}

bool VMaxFormat::loadObject(const core::String &filename, io::SeekableReadStream* data, scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx,
							const VMaxObject &obj, const palette::Palette &palette, const VmaxLayerMaterials &layers) const {
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
		if (dict.hasKey("chunks") || dict.hasKey("voxels")) {
			Log::error("File uses obsolete VoxelMax chunks/voxels storage");
		}
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

	palette::Palette pal = palette;
	VmaxLayerMaterials objectLayers = layers;
	const util::BinaryPList &embeddedPal = plist.getDictEntry("pal");
	if (embeddedPal.isDict()) {
		if (pal.colorCount() == 0) {
			const util::BinaryPList &colors = embeddedPal.getDictEntry("colors");
			if (colors.isData() && colors.size() >= 4u) {
				image::ImagePtr img = image::createEmptyImage("vmax-pal");
				const int ncolors = (int)(colors.size() / 4u);
				if (!img->loadRGBA(colors.asData().data(), ncolors, 1) || !pal.load(img)) {
					Log::debug("Failed to load embedded vmaxb pal.colors");
				}
			}
		}
		applyVmaxPaletteSettings(embeddedPal, pal, objectLayers);
	}
	uint8_t assignedLayer[256];
	core_memset(assignedLayer, 0xFF, sizeof(assignedLayer));

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
		node.setVolume(v);
		node.setPalette(pal);

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
			const uint8_t layer = material & 7;
			if (objectLayers.present[layer]) {
				if (assignedLayer[palIdx] == 0xFF) {
					assignedLayer[palIdx] = layer;
					applyVmaxLayerMaterial(pal, palIdx, objectLayers.layers[layer]);
				} else if (assignedLayer[palIdx] != layer) {
					Log::debug("Palette index %u used on VoxelMax layers %u and %u", palIdx,
							   assignedLayer[palIdx], layer);
				}
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
								  voxel::createVoxel(pal, palIdx))) {
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
	voxel::RawVolume *volume = merged.volume();
	voxel::RawVolume *cropped = voxelutil::cropVolume(volume);
	if (cropped != nullptr) {
		delete volume;
		volume = cropped;
	}
	const glm::ivec3 volumeMins = volume->region().getLowerCorner();
	volume->translate(-volumeMins);

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model, obj.id);
	node.setName(obj.n);
	applySceneTransform(node, obj.t_p, obj.t_r, obj.t_s, glm::vec3(volumeMins));
	if (obj.pid.isValid()) {
		node.setProperty(scenegraph::PropParentUUID, obj.pid.str());
	}
	node.setVisible(!obj.h);
	node.setVolume(volume);
	node.setPalette(pal);
	node.setNormalPalette(merged.normalPalette);
	return sceneGraph.emplace(core::move(node), parent) != InvalidNodeId;
}

image::ImagePtr VMaxFormat::loadScreenshot(const core::String &filename, const io::ArchivePtr &archive,
										   const LoadContext &ctx) {
	const core::String &thumbnailPath = core::string::path("QuickLook", "Thumbnail.png");
	core::ScopedPtr<io::SeekableReadStream> thumbnailStream;

	core::String packageDir;
	if (vmaxIsPackage(archive, filename, packageDir)) {
		thumbnailStream = archive->readStream(vmaxJoin(packageDir, thumbnailPath));
	} else {
		core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
		if (!stream) {
			Log::error("Could not load file %s", filename.c_str());
			return image::ImagePtr();
		}
		const io::ArchivePtr &zipArchive = io::openZipArchive(stream);
		if (zipArchive) {
			thumbnailStream = zipArchive->readStream(thumbnailPath);
		} else {
			const core::String fullPath = vmaxJoin(core::string::extractDir(filename), thumbnailPath);
			if (archive->exists(fullPath)) {
				thumbnailStream = archive->readStream(fullPath);
			}
		}
	}
	if (!thumbnailStream) {
		Log::error("Failed to load %s from %s", thumbnailPath.c_str(), filename.c_str());
		return image::ImagePtr();
	}
	Log::debug("Found thumbnail %s in archive %s", thumbnailPath.c_str(), filename.c_str());

	const core::String &name = core::string::extractFilenameWithExtension(thumbnailPath);
	return image::loadImage(name, *thumbnailStream);
}

bool VMaxFormat::parseVmaxMaterial(const util::BinaryPList &node, VmaxMaterial &material, int &layer) const {
	if (!node.isDict()) {
		return false;
	}
	layer = -1;
	vmaxParseLayerIndex(node.getDictEntry("mi"), layer);
	const util::BinaryPList &name = node.getDictEntry("name");
	if (name.isString()) {
		material.name = name.asString();
	}
	double value = 0.0;
	if (vmaxPlistNumber(node.getDictEntry("sic"), value)) {
		material.emission = value;
		material.hasEmission = true;
	}
	if (vmaxPlistNumber(node.getDictEntry("mc"), value)) {
		material.metalness = value;
		material.hasMetalness = true;
	}
	if (vmaxPlistNumber(node.getDictEntry("rc"), value)) {
		material.roughness = value;
		material.hasRoughness = true;
	}
	const util::BinaryPList &shadows = node.getDictEntry("sh");
	if (shadows.isBoolean()) {
		material.enableShadows = shadows.asBoolean();
	}
	const util::BinaryPList &medium = node.getDictEntry("medium");
	if (medium.isDict()) {
		if (vmaxPlistNumber(medium.getDictEntry("t"), value)) {
			material.transmission = value;
			material.hasTransmission = true;
		}
		if (vmaxPlistNumber(medium.getDictEntry("i"), value)) {
			material.ior = value;
			material.hasIor = true;
		}
		if (vmaxPlistNumber(medium.getDictEntry("a"), value)) {
			material.anisotropy = value;
		}
	}
	return layer >= 0 && layer < 8;
}

void VMaxFormat::applyVmaxLayerMaterial(palette::Palette &palette, uint8_t palIdx, const VmaxMaterial &material) const {
	if (material.hasMetalness) {
		palette.setMetal(palIdx, (float)material.metalness);
	}
	if (material.hasRoughness) {
		palette.setRoughness(palIdx, (float)material.roughness);
	}
	if (material.hasEmission && material.emission > 0.0) {
		palette.setEmit(palIdx, (float)material.emission);
		palette.setMaterialType(palIdx, palette::MaterialType::Emit);
	} else if (material.hasTransmission && material.transmission > 0.0) {
		palette.setMaterialType(palIdx, palette::MaterialType::Glass);
		if (material.hasIor) {
			palette.setIndexOfRefraction(palIdx, (float)material.ior);
		}
	}
}

void VMaxFormat::applyVmaxPaletteSettings(const util::BinaryPList &plist, palette::Palette &palette,
										  VmaxLayerMaterials &layers) const {
	if (!plist.isDict()) {
		return;
	}
	const util::BinaryPList &name = plist.getDictEntry("name");
	if (name.isString()) {
		palette.setName(name.asString());
	}
	int activeLayer = layers.activeLayer;
	if (vmaxParseLayerIndex(plist.getDictEntry("ali"), activeLayer)) {
		layers.activeLayer = activeLayer;
	}

	const util::BinaryPList &materialsNode = plist.getDictEntry("materials");
	if (materialsNode.isArray()) {
		const util::PListArray &materialsArray = materialsNode.asArray();
		Log::debug("Found %i VoxelMax layer materials", (int)materialsArray.size());
		for (size_t i = 0; i < materialsArray.size(); ++i) {
			VmaxMaterial material;
			int layer = -1;
			if (!parseVmaxMaterial(materialsArray[i], material, layer)) {
				if (i < 8) {
					layer = (int)i;
				} else {
					continue;
				}
			}
			layers.layers[layer] = material;
			layers.present[layer] = true;
		}
	}

	int applyLayer = layers.activeLayer;
	int firstLayer = -1;
	bool layersMatch = true;
	for (int i = 0; i < 8; ++i) {
		if (!layers.present[i]) {
			continue;
		}
		if (firstLayer == -1) {
			firstLayer = i;
			continue;
		}
		const VmaxMaterial &a = layers.layers[firstLayer];
		const VmaxMaterial &b = layers.layers[i];
		if (glm::abs(a.roughness - b.roughness) > 1.0e-4 || glm::abs(a.metalness - b.metalness) > 1.0e-4 ||
			glm::abs(a.emission - b.emission) > 1.0e-4 || glm::abs(a.transmission - b.transmission) > 1.0e-4) {
			layersMatch = false;
			break;
		}
	}
	if (layersMatch && firstLayer != -1) {
		applyLayer = firstLayer;
	}
	if (applyLayer < 0 || applyLayer > 7 || !layers.present[applyLayer]) {
		applyLayer = firstLayer;
	}

	const util::BinaryPList &lc = plist.getDictEntry("lc");
	if (applyLayer >= 0 && lc.isData() && layers.present[applyLayer]) {
		const util::PListByteArray &usage = lc.asData();
		const size_t n = core_min(usage.size(), (size_t)palette::PaletteMaxColors);
		for (size_t i = 1; i < n; ++i) {
			if (usage[i] == 0) {
				continue;
			}
			applyVmaxLayerMaterial(palette, (uint8_t)i, layers.layers[applyLayer]);
		}
	}

	const util::BinaryPList &voxmats = plist.getDictEntry("voxmats");
	if (!voxmats.isArray()) {
		return;
	}
	const util::PListArray &voxArray = voxmats.asArray();
	for (size_t i = 0; i < voxArray.size(); ++i) {
		const util::BinaryPList &entry = voxArray[i];
		if (!entry.isDict()) {
			continue;
		}
		const util::BinaryPList &idNode = entry.getDictEntry("id");
		if (!idNode.isInt()) {
			continue;
		}
		const int palIdx = (int)idNode.asInt();
		if (palIdx <= 0 || palIdx >= palette::PaletteMaxColors) {
			continue;
		}
		const util::BinaryPList &tp = entry.getDictEntry("tp");
		if (tp.isString() && tp.asString().size() == 1u) {
			switch (tp.asString().c_str()[0]) {
			case 'm':
				palette.setMaterialType((uint8_t)palIdx, palette::MaterialType::Metal);
				break;
			case 'g':
				palette.setMaterialType((uint8_t)palIdx, palette::MaterialType::Glass);
				break;
			case 'e':
				palette.setMaterialType((uint8_t)palIdx, palette::MaterialType::Emit);
				break;
			case 'b':
				palette.setMaterialType((uint8_t)palIdx, palette::MaterialType::Blend);
				break;
			case 'c':
				palette.setMaterialType((uint8_t)palIdx, palette::MaterialType::Media);
				break;
			default:
				palette.setMaterialType((uint8_t)palIdx, palette::MaterialType::Diffuse);
				break;
			}
		}
		double value = 0.0;
		if (vmaxPlistNumber(entry.getDictEntry("mt"), value)) {
			palette.setMetal((uint8_t)palIdx, (float)value);
		}
		if (vmaxPlistNumber(entry.getDictEntry("rg"), value)) {
			palette.setRoughness((uint8_t)palIdx, (float)value);
		}
		if (vmaxPlistNumber(entry.getDictEntry("io"), value)) {
			palette.setIndexOfRefraction((uint8_t)palIdx, (float)value);
		}
		if (vmaxPlistNumber(entry.getDictEntry("em"), value) && value > 0.0) {
			palette.setEmit((uint8_t)palIdx, (float)value);
		}
		if (vmaxPlistNumber(entry.getDictEntry("ap"), value)) {
			palette.setAlpha((uint8_t)palIdx, (float)value);
		}
		if (vmaxPlistNumber(entry.getDictEntry("fl"), value)) {
			palette.setFlux((uint8_t)palIdx, (float)value);
		}
		if (vmaxPlistNumber(entry.getDictEntry("ld"), value)) {
			palette.setLowDynamicRange((uint8_t)palIdx, (float)value);
		}
		if (vmaxPlistNumber(entry.getDictEntry("dn"), value)) {
			palette.setDensity((uint8_t)palIdx, (float)value);
		}
		if (vmaxPlistNumber(entry.getDictEntry("md"), value)) {
			palette.setMedia((uint8_t)palIdx, (float)value);
		}
		if (vmaxPlistNumber(entry.getDictEntry("ph"), value)) {
			palette.setPhase((uint8_t)palIdx, (float)value);
		}
	}
}

bool VMaxFormat::loadPaletteFromVmaxb(io::SeekableReadStream &stream, palette::Palette &palette,
									  VmaxLayerMaterials &layers) const {
	io::LZFSEReadStream lzfse(stream);
	const util::BinaryPList plist = util::BinaryPList::parse(lzfse);
	const util::BinaryPList &embeddedPal = plist.getDictEntry("pal");
	if (!embeddedPal.isDict()) {
		return false;
	}
	if (palette.colorCount() == 0) {
		const util::BinaryPList &colors = embeddedPal.getDictEntry("colors");
		if (colors.isData() && colors.size() >= 4u) {
			image::ImagePtr img = image::createEmptyImage("vmax-pal");
			const int ncolors = (int)(colors.size() / 4u);
			if (!img->loadRGBA(colors.asData().data(), ncolors, 1) || !palette.load(img)) {
				Log::error("Failed to load embedded vmaxb pal.colors");
				return false;
			}
		}
	}
	applyVmaxPaletteSettings(embeddedPal, palette, layers);
	return palette.colorCount() > 0;
}

bool VMaxFormat::loadPaletteFromArchive(const io::ArchivePtr &archive, const core::String &paletteName,
										palette::Palette &palette, const LoadContext &ctx,
										VmaxLayerMaterials &layers) const {
	if (paletteName.empty()) {
		Log::error("Empty palette path");
		return false;
	}
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
	if (!archive->exists(settingsName)) {
		const core::String fallback = vmaxJoin(core::string::extractDir(paletteName), "palette.settings.vmaxpsb");
		if (archive->exists(fallback)) {
			settingsName = fallback;
		} else if (archive->exists("palette.settings.vmaxpsb")) {
			settingsName = "palette.settings.vmaxpsb";
		} else {
			settingsName = "";
		}
	}

	if (!settingsName.empty()) {
		core::ScopedPtr<io::SeekableReadStream> paletteSettingsStream(archive->readStream(settingsName));
		if (paletteSettingsStream) {
			const util::BinaryPList &plist = util::BinaryPList::parse(*paletteSettingsStream);
			applyVmaxPaletteSettings(plist, palette, layers);
		}
	} else {
		Log::debug("No palette settings sidecar found for %s", paletteName.c_str());
	}

	return true;
}

size_t VMaxFormat::loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
							   const LoadContext &ctx) {
	const core::String &paletteName = "palette.png";
	VmaxLayerMaterials layers;
	core::String packageDir;
	if (vmaxIsPackage(archive, filename, packageDir)) {
		if (loadPaletteFromArchive(archive, vmaxJoin(packageDir, paletteName), palette, ctx, layers)) {
			return palette.colorCount();
		}
		return 0u;
	}
	core::ScopedPtr<io::SeekableReadStream> archiveStream(archive->readStream(filename));
	if (!archiveStream) {
		const core::String fullPath = vmaxJoin(core::string::extractDir(filename), paletteName);
		if (!loadPaletteFromArchive(archive, fullPath, palette, ctx, layers)) {
			Log::error("Failed to load palette from %s", fullPath.c_str());
			return 0u;
		}
		return palette.colorCount();
	}
	io::ArchivePtr zipArchive = io::openZipArchive(archiveStream);
	if (zipArchive) {
		Log::debug("Found zip archive %s", filename.c_str());
		if (!loadPaletteFromArchive(zipArchive, paletteName, palette, ctx, layers)) {
			Log::error("Failed to load palette from %s", paletteName.c_str());
			return 0u;
		}
	} else {
		const core::String fullPath = vmaxJoin(core::string::extractDir(filename), paletteName);
		if (!loadPaletteFromArchive(archive, fullPath, palette, ctx, layers)) {
			if (core::string::extractExtension(filename) == "vmaxb") {
				if (archiveStream->seek(0) != -1 && loadPaletteFromVmaxb(*archiveStream, palette, layers)) {
					return palette.colorCount();
				}
			}
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
