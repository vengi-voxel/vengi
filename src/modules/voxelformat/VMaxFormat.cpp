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
#include "io/ZipArchive.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/Palette.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"

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

	auto objects = json.find("objects");
	if (objects == json.end() || !objects->is_array()) {
		Log::error("Failed to parse the scene json - expected an array of objects");
		return false;
	}
	for (const auto &obj : objects.value()) {
		VMaxObject o;
		jsonBool(obj, s, o);
		jsonString(obj, n, o);
		jsonString(obj, data, o);
		jsonString(obj, pal, o);
		jsonString(obj, pid, o);
		jsonString(obj, hist, o);
		jsonString(obj, id, o);
		jsonString(obj, t_al, o);
		jsonString(obj, t_pa, o);
		jsonString(obj, t_pf, o);
		jsonVec(obj, ind, o);
		jsonVec(obj, e_c, o);
		jsonVec(obj, e_mi, o);
		jsonVec(obj, e_ma, o);
		jsonVec(obj, t_p, o);
		jsonVec(obj, t_s, o);
		jsonVec(obj, t_r, o);
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
		if (!loadObject(archive, sceneGraph, ctx, obj)) {
			Log::error("Failed to load object %s", obj.n.c_str());
			return false;
		}
	}
	return true;
}

voxel::RawVolume *VMaxFormat::loadVolume(io::ZipArchive &archive, const LoadContext &ctx, const VMaxObject &obj) const {
	io::BufferedReadWriteStream data;
	if (!archive.load(obj.data, data)) {
		Log::error("Failed to load %s", obj.data.c_str());
		return nullptr;
	}
	if (data.seek(0) == -1) {
		Log::error("Failed to seek to the beginning of the sub stream");
		return nullptr;
	}
	io::LZFSEReadStream lzfseStream(data);
	// Apple binary property list
	uint32_t magic0;
	lzfseStream.readUInt32(magic0);
	if (magic0 != FourCC('b', 'p', 'l', 'i')) {
		Log::error("Unexpected magic0 byte");
		return nullptr;
	}
	uint32_t magic1;
	lzfseStream.readUInt32(magic1);
	if (magic1 != FourCC('s', 't', '0', '0')) {
		Log::error("Unexpected magic1 byte");
		return nullptr;
	}

	// https://github.com/opensource-apple/CF/blob/master/CFBinaryPList.c
	voxel::Region region(0, 1);
	voxel::RawVolume *v = new voxel::RawVolume(region);
	Log::error("Not yet supported to load the voxel data");
	return v;
}

bool VMaxFormat::loadObject(io::ZipArchive &archive, scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx,
							const VMaxObject &obj) const {
	voxel::Palette palette;
	if (!loadPalette(archive, obj.pal, palette, ctx)) {
		return false;
	}

	voxel::RawVolume *v = loadVolume(archive, ctx, obj);
	if (v == nullptr) {
		return false;
	}

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
