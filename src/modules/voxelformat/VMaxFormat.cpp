/**
 * @file
 */

#include "VMaxFormat.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "external/json.hpp"
#include "image/Image.h"
#include "io/BufferedReadWriteStream.h"
#include "io/LZFSEReadStream.h"
#include "io/MemoryReadStream.h"
#include "io/ZipArchive.h"
#include "private/BinaryPList.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/Palette.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxelformat/Format.h"
#include <glm/common.hpp>

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

bool VMaxFormat::loadSceneJson(io::ZipArchive &archive, VMaxScene &scene) const {
	io::BufferedReadWriteStream contentsStream;
	if (!archive.load("scene.json", contentsStream)) {
		Log::error("Failed to load scene.json");
		return false;
	}

	contentsStream.seek(0);

	std::string jsonStr((const char *)contentsStream.getBuffer(), contentsStream.size());
	nlohmann::json json = nlohmann::json::parse(jsonStr);
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
	io::ZipArchive archive;
	if (!archive.open(&stream)) {
		Log::error("Failed to open zip archive %s", filename.c_str());
		return false;
	}

	VMaxScene scene;
	if (!loadSceneJson(archive, scene)) {
		return false;
	}

	// layers are in own contents files. They start at contents, contents1, contents2, ... and so on. The same
	// is true for palettes
	for (const auto &obj : scene.objects) {
		if (!loadObject(filename, archive, sceneGraph, ctx, obj)) {
			Log::error("Failed to load object %s", obj.n.c_str());
			return false;
		}
	}
	return true;
}

bool VMaxFormat::loadVolume(const core::String &filename, io::ZipArchive &archive, const LoadContext &ctx,
							const VMaxObject &obj, voxel::RawVolume *v) const {
	io::BufferedReadWriteStream data;
	if (!archive.load(obj.data, data)) {
		Log::error("Failed to load %s", obj.data.c_str());
		return false;
	}
	if (data.seek(0) == -1) {
		Log::error("Failed to seek to the beginning of the sub stream");
		return false;
	}
	io::LZFSEReadStream stream(data);

	// io::filesystem()->write(filename + ".plist", stream);
	// stream.seek(0);

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
		Log::error("Node 'snapshots' is empty");
		return false;
	}

	for (size_t i = 0; i < snapshotsArray.size(); ++i) {
		const priv::BinaryPList &snapshotDict = snapshotsArray[i];
		if (!snapshotDict.isDict()) {
			Log::error("Node 'snapshots' child %i is no dict", (int)i);
			return false;
		}

		const priv::PListDict::iterator snapshotId = snapshotDict.asDict().find("s");
		if (snapshotId == snapshotDict.asDict().end()) {
			Log::error("Node 'snapshots' doesn't contain node 's'");
			return false;
		}
		if (!snapshotId->value.isDict()) {
			Log::error("Snapshot node 's' is no dict");
			return false;
		}

		const priv::PListDict &volumeDict = snapshotId->value.asDict();
		// auto dlcIter = volumeDict.find("dlc"); // data
		// auto lcIter = volumeDict.find("lc"); // data
		const priv::PListDict::iterator dsIter = volumeDict.find("ds");
		if (dsIter == volumeDict.end()) {
			Log::error("Failed to find the 'ds' node");
			return false;
		}
		if (!dsIter->value.isData()) {
			Log::error("Node 'ds' is no data node");
			return false;
		}
		const priv::PListByteArray &dsData = dsIter->value.asData();
		const size_t dsSize = dsData.size();
		if (dsSize == 0u) {
			Log::error("Node 'ds' is empty");
			return false;
		}

		io::MemoryReadStream dsStream(dsData.data(), dsSize);

		// TODO: find out the chunk position

		Log::debug("Found voxel data with size %i", (int)dsStream.size());
		// io::filesystem()->write(filename + ".ds.bin", dsStream);
	}

	Log::error("Not yet supported to load the voxel data");
	return false;
}

bool VMaxFormat::loadObject(const core::String &filename, io::ZipArchive &archive, scenegraph::SceneGraph &sceneGraph,
							const LoadContext &ctx, const VMaxObject &obj) const {
	voxel::Palette palette;
	if (!loadPalette(archive, obj.pal, palette, ctx)) {
		return false;
	}

	const glm::vec3 mins = obj.e_c + obj.e_mi;
	const glm::vec3 maxs = obj.e_c + obj.e_ma;
	const voxel::Region region(glm::ivec3(mins), glm::ivec3(maxs) - 1);
	if (!region.isValid()) {
		Log::error("Invalid region for object '%s': %s", obj.n.c_str(), region.toString().c_str());
		return false;
	}
	voxel::RawVolume *v = new voxel::RawVolume(region);

	loadVolume(filename, archive, ctx, obj, v);

	scenegraph::SceneGraphTransform transform;
	transform.setWorldScale(obj.t_s);
	transform.setWorldTranslation(obj.t_p);
	transform.setWorldOrientation(glm::quat(obj.t_r.w, obj.t_r.x, obj.t_r.y, obj.t_r.z));

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setName(obj.n);
	node.setPalette(palette);
	node.setVolume(v, true);
	scenegraph::KeyFrameIndex keyFrameIdx = 0;
	node.setTransform(keyFrameIdx, transform);
	return sceneGraph.emplace(core::move(node)) != InvalidNodeId;
}

image::ImagePtr VMaxFormat::loadScreenshot(const core::String &filename, io::SeekableReadStream &stream,
										   const LoadContext &ctx) {
	io::ZipArchive archive;
	if (!archive.open(&stream)) {
		Log::error("Failed to open zip archive %s", filename.c_str());
		return image::ImagePtr();
	}

	io::BufferedReadWriteStream contentsStream;
	if (!archive.load("QuickLook/Thumbnail.png", contentsStream)) {
		Log::error("Failed to load QuickLook/Thumbnail.png from %s", filename.c_str());
		return image::ImagePtr();
	}

	if (contentsStream.seek(0) == -1) {
		Log::error("Failed to seek to the beginning of the sub stream for %s", filename.c_str());
		return 0u;
	}
	return image::loadImage("Thumbnail.png", contentsStream);
}

bool VMaxFormat::loadPalette(io::ZipArchive &archive, const core::String &paletteName, voxel::Palette &palette,
							 const LoadContext &ctx) const {
	io::BufferedReadWriteStream contentsStream;
	if (!archive.load(paletteName, contentsStream)) {
		Log::error("Failed to load %s", paletteName.c_str());
		return false;
	}

	if (contentsStream.seek(0) == -1) {
		Log::error("Failed to seek to the beginning of the sub stream for the palette %s", paletteName.c_str());
		return false;
	}

	const image::ImagePtr &img = image::loadImage(paletteName, contentsStream);
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
	io::ZipArchive archive;
	if (!archive.open(&stream)) {
		Log::error("Failed to open zip archive %s", filename.c_str());
		return 0u;
	}

	if (!loadPalette(archive, "palette.png", palette, ctx)) {
		return 0u;
	}
	return palette.colorCount();
}

} // namespace voxelformat
