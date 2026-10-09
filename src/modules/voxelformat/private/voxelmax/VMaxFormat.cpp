/**
 * @file
 */

#include "VMaxFormat.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StandardLib.h"
#include "core/StringUtil.h"
#include "core/collection/Buffer.h"
#include "core/collection/Map.h"
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
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelformat/Format.h"
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

static util::BinaryPList vmaxReadPlist(io::SeekableReadStream &stream) {
	char magic[8];
	const int64_t start = stream.pos();
	if (stream.read(magic, sizeof(magic)) != sizeof(magic) || stream.seek(start) == -1) {
		return {};
	}
	if (core_memcmp(magic, "bplist00", sizeof(magic)) == 0) {
		return util::BinaryPList::parse(stream);
	}
	io::LZFSEReadStream decoded(stream);
	return util::BinaryPList::parse(decoded);
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
			return false;
		}

		palette::Palette vmaxPalette;
		VmaxLayerMaterials layers;
		if (!loadPaletteFromArchive(archive, obj.pal, vmaxPalette, ctx, layers)) {
			core::ScopedPtr<io::SeekableReadStream> contents(archive->readStream(obj.data));
			if (!contents || !loadPaletteFromVmaxb(*contents, vmaxPalette, layers)) {
				Log::error("Failed to load palette for object %s", obj.n.c_str());
				return false;
			}
		}
		if (!loadObjectFromArchive(obj.data, archive, sceneGraph, ctx, obj, vmaxPalette, layers)) {
			Log::error("Failed to load object %s", obj.n.c_str());
			return false;
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
	const util::BinaryPList plist = vmaxReadPlist(*data);
	if (!plist.isDict()) {
		Log::error("Expected a bplist dict");
		return false;
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
	struct VoxelData {
		glm::ivec3 position;
		uint8_t material;
		uint8_t color;
	};
	core::DynamicArray<VoxelData> voxels;
	int order = 8;
	const util::BinaryPList &eo = plist.getDictEntry("eo");
	if (eo.isInt() && eo.asInt() >= 5 && eo.asInt() <= 9) {
		order = (int)eo.asInt();
	}
	glm::ivec3 workMin(0), workMax((1 << order) - 1);
	const util::BinaryPList &vp = plist.getDictEntry("tools").getDictEntry("vp");
	if (vp.isDict()) {
		const util::BinaryPList &lo = vp.getDictEntry("min");
		const util::BinaryPList &hi = vp.getDictEntry("max");
		if (!lo.isArray() || !hi.isArray() || lo.size() != 3 || hi.size() != 3) {
			Log::error("Invalid VoxelMax work area");
			return false;
		}
		for (int axis = 0; axis < 3; ++axis) {
			if (!lo.asArray()[axis].isInt() || !hi.asArray()[axis].isInt()) {
				return false;
			}
			workMin[axis] = (int)lo.asArray()[axis].asInt();
			workMax[axis] = (int)hi.asArray()[axis].asInt();
			if (workMin[axis] < 0 || workMax[axis] > 511 || workMin[axis] > workMax[axis]) {
				return false;
			}
		}
	}
	const auto decode = [&](const util::BinaryPList &bytes, const glm::ivec3 &base, int offset, int stride) {
		if (!bytes.isData() || bytes.size() % stride != 0 || offset < 0 ||
			(size_t)offset + bytes.size() / stride > 32768u) {
			Log::error("Invalid VoxelMax chunk data or Morton offset");
			return false;
		}
		const util::PListByteArray &ds = bytes.asData();
		// DynamicArray grows in fixed increments. Reserve geometrically before
		// appending a chunk to avoid repeatedly copying a large object's voxels.
		const size_t required = voxels.size() + ds.size() / stride;
		if (required > voxels.capacity()) {
			voxels.reserve(core_max(required, voxels.capacity() * 2));
		}
		for (size_t slot = 0; slot < ds.size() / stride; ++slot) {
			const uint8_t color = ds[slot * stride + stride - 1];
			if (color == 0) {
				continue;
			}
			uint8_t x, y, z;
			voxel::mortonIndexToCoord((uint32_t)(offset + slot), x, y, z);
			const glm::ivec3 position = base + glm::ivec3(x, y, z);
			if (glm::any(glm::lessThan(position, workMin)) || glm::any(glm::greaterThan(position, workMax))) {
				continue;
			}
			voxels.push_back({glm::ivec3(position.x, position.z, position.y),
							  (uint8_t)(stride == 2 ? ds[slot * stride] & 7 : 0), (uint8_t)(color - 1)});
		}
		return true;
	};

	const util::BinaryPList &version = plist.getDictEntry("v");
	const util::BinaryPList &legacyChunks = plist.getDictEntry("chunks");
	const util::BinaryPList &snapshots = plist.getDictEntry("snapshots");
	// Some standalone contents omit the version; accept their snapshots as modern storage.
	if ((version.isInt() && version.asInt() < 4) || (!snapshots.valid() && legacyChunks.valid())) {
		const util::BinaryPList &legacyVoxels = plist.getDictEntry("voxels");
		if (!legacyChunks.isData() || legacyChunks.size() % 16 != 0 || !legacyVoxels.isArray() ||
			legacyVoxels.size() != legacyChunks.size() / 16) {
			Log::error("Invalid legacy VoxelMax chunks/voxels storage");
			return false;
		}
		io::MemoryReadStream records(legacyChunks.asData().data(), legacyChunks.size());
		struct LegacyChunk {
			glm::ivec3 base;
			int stride;
		};
		core::DynamicArray<LegacyChunk> chunks;
		for (size_t i = 0; i < legacyVoxels.size(); ++i) {
			int32_t origin[3], flag;
			for (int axis = 0; axis < 3; ++axis) {
				wrap(records.readInt32(origin[axis]))
			}
			wrap(records.readInt32(flag))
			glm::ivec3 base;
			for (int axis = 0; axis < 3; ++axis) {
				// Floor division, including negative chunk origins.
				const int value = origin[axis];
				base[axis] = (value / 32 - (value % 32 < 0 ? 1 : 0)) * 32;
			}
			chunks.push_back({base, version.asInt() >= 1 && flag == 1 ? 2 : 1});
		}
		for (size_t i = 0; i < chunks.size(); ++i) {
			bool replaced = false;
			for (size_t j = i + 1; j < chunks.size(); ++j) {
				if (chunks[i].base == chunks[j].base) {
					replaced = true;
					break;
				}
			}
			if (!replaced && !decode(legacyVoxels.asArray()[i], chunks[i].base, 0, chunks[i].stride)) {
				return false;
			}
		}
	} else {
		if (!snapshots.isArray()) {
			Log::error("Expected VoxelMax snapshots");
			return false;
		}
		core::Map<uint32_t, size_t> latest;
		for (size_t i = 0; i < snapshots.size(); ++i) {
			const util::BinaryPList &storage = snapshots.asArray()[i].getDictEntry("s");
			const util::BinaryPList &cid = storage.getDictEntry("id").getDictEntry("c");
			// 512^3 workspaces have 16^3 chunks, encoded as Morton IDs 0..4095.
			if (!cid.isInt() || cid.asInt() >= 4096u) {
				Log::error("Invalid VoxelMax chunk identifier");
				return false;
			}
			latest.put((uint32_t)cid.asInt(), i);
		}
		for (size_t i = 0; i < snapshots.size(); ++i) {
			if (stopExecution()) {
				return false;
			}
			const util::BinaryPList &storage = snapshots.asArray()[i].getDictEntry("s");
			const uint32_t cid = (uint32_t)storage.getDictEntry("id").getDictEntry("c").asInt();
			if (latest.find(cid)->value != i) {
				continue;
			}
			const util::BinaryPList &stats = storage.getDictEntry("st");
			const util::BinaryPList &mins = stats.getDictEntry("min");
			const util::BinaryPList &chunkOrder = stats.getDictEntry("extent").getDictEntry("o");
			if (!mins.isArray() || mins.size() != 4 || !mins.asArray()[3].isInt() ||
				mins.asArray()[3].asInt() >= 32768u || !chunkOrder.isInt() || chunkOrder.asInt() != 5) {
				Log::error("Invalid VoxelMax snapshot stats");
				return false;
			}
			uint8_t x, y, z;
			voxel::mortonIndexToCoord(cid, x, y, z);
			if (!decode(storage.getDictEntry("ds"), glm::ivec3(x, y, z) * 32, (int)mins.asArray()[3].asInt(), 2)) {
				return false;
			}
		}
	}
	if (voxels.empty()) {
		return true;
	}

	// Each used (layer,color) pair needs its own palette entry. Split only when
	// the 256-entry vengi palette fills up, preserving all 8 * 255 combinations.
	int entries[8][256];
	core_memset(entries, 0xFF, sizeof(entries));
	int combinations = 0;
	for (const VoxelData &v : voxels) {
		if (entries[v.material][v.color] == -1) {
			entries[v.material][v.color] = combinations++;
		}
	}
	int parent = sceneGraph.root().id();
	if (obj.pid.isValid()) {
		if (scenegraph::SceneGraphNode *parentNode = sceneGraph.findNodeByUUID(obj.pid)) {
			parent = parentNode->id();
		}
	}
	for (int part = 0; part < (combinations + 255) / 256; ++part) {
		palette::Palette partPalette;
		partPalette.setName(pal.name());
		glm::ivec3 mins(512), maxs(-1);
		for (int layer = 0; layer < 8; ++layer) {
			for (int color = 0; color < 255; ++color) {
				const int entry = entries[layer][color];
				if (entry < 0 || entry / 256 != part) {
					continue;
				}
				const uint8_t index = (uint8_t)(entry % 256);
				partPalette.setColor(index, pal.color((uint8_t)color));
				partPalette.setMaterial(index, pal.material((uint8_t)color));
				if (objectLayers.present[layer]) {
					applyVmaxLayerMaterial(partPalette, index, objectLayers.layers[layer]);
				}
			}
		}
		for (const VoxelData &v : voxels) {
			if (entries[v.material][v.color] / 256 == part) {
				mins = glm::min(mins, v.position);
				maxs = glm::max(maxs, v.position);
			}
		}
		partPalette.setSize(core_min(256, combinations - part * 256));
		voxel::RawVolume *volume = new voxel::RawVolume(voxel::Region(glm::ivec3(0), maxs - mins));
		for (const VoxelData &v : voxels) {
			const int entry = entries[v.material][v.color];
			if (entry / 256 == part) {
				volume->setVoxel(v.position - mins, voxel::createVoxel(partPalette, (uint8_t)(entry % 256)));
			}
		}
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model, part == 0 ? obj.id : core::UUID());
		node.setName(part == 0 ? obj.n : core::String::format("%s-%i", obj.n.c_str(), part + 1));
		applySceneTransform(node, obj.t_p, obj.t_r, obj.t_s, glm::vec3(mins));
		if (obj.pid.isValid()) {
			node.setProperty(scenegraph::PropParentUUID, obj.pid.str());
		}
		node.setVisible(!obj.h);
		node.setVolume(volume);
		node.setPalette(partPalette);
		if (sceneGraph.emplace(core::move(node), parent) == InvalidNodeId) {
			return false;
		}
	}
	return true;
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
	if (vmaxPlistNumber(node.getDictEntry("tc"), value)) {
		material.transmission = value;
		material.hasTransmission = true;
	}
	const util::BinaryPList &dispersion = node.getDictEntry("md");
	const util::BinaryPList &medium = dispersion.isDict() ? dispersion : node.getDictEntry("medium");
	if (medium.isDict()) {
		if (vmaxPlistNumber(medium.getDictEntry("t"), value)) {
			material.transmission = value;
			material.hasTransmission = true;
		}
		if (vmaxPlistNumber(medium.getDictEntry("i"), value)) {
			material.ior = value;
			material.hasIor = true;
		}

	}
	return layer >= 0 && layer < 8;
}

void VMaxFormat::applyVmaxLayerMaterial(palette::Palette &palette, uint8_t palIdx, const VmaxMaterial &material) const {
	if (material.hasMetalness) {
		palette.setMetal(palIdx, (float)glm::clamp((material.metalness - 0.1) / 0.8, 0.0, 1.0));
	}
	if (material.hasRoughness) {
		palette.setRoughness(palIdx, (float)glm::clamp((material.roughness - 0.1) / 0.8, 0.0, 1.0));
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
			applyVmaxLayerMaterial(palette, (uint8_t)(i - 1), layers.layers[applyLayer]);
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
		const int palIdx = (int)idNode.asInt() - 1;
		if (palIdx < 0 || palIdx >= 255) {
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
	const util::BinaryPList plist = vmaxReadPlist(stream);
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
		const core::String settingsName = core::string::stripExtension(paletteName) + ".settings.vmaxpsb";
		core::ScopedPtr<io::SeekableReadStream> settingsStream(archive->readStream(settingsName));
		if (settingsStream) {
			const util::BinaryPList settings = vmaxReadPlist(*settingsStream);
			const util::BinaryPList &colors = settings.getDictEntry("colors");
			image::ImagePtr image = image::createEmptyImage(settingsName);
			if (colors.isData() && !colors.empty() && colors.size() % 4 == 0 && colors.size() <= 255 * 4 &&
				image->loadRGBA(colors.asData().data(), (int)(colors.size() / 4), 1) && palette.load(image)) {
				applyVmaxPaletteSettings(settings, palette, layers);
				return true;
			}
		}
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
	core::String paletteName = core::string::extractExtension(filename) == "vmaxb" ? vmaxPaletteFileForContents(filename) : "palette.png";
	VmaxLayerMaterials layers;
	core::String packageDir;
	if (vmaxIsPackage(archive, filename, packageDir)) {
		VMaxScene scene;
		if (loadSceneJson(archive, scene, vmaxJoin(packageDir, "scene.json")) && !scene.objects.empty() &&
			!scene.objects[0].pal.empty()) {
			paletteName = scene.objects[0].pal;
		}
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
		VMaxScene scene;
		if (loadSceneJson(zipArchive, scene, "scene.json") && !scene.objects.empty() && !scene.objects[0].pal.empty()) {
			paletteName = scene.objects[0].pal;
		}
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
