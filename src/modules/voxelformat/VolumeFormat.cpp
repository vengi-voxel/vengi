/**
 * @file
 */

#include "VolumeFormat.h"
#include "app/App.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/SharedPtr.h"
#include "core/StringUtil.h"
#include "core/Trace.h"
#include "core/collection/DynamicArray.h"
#include "io/Archive.h"
#include "io/FilesystemArchive.h"
#include "io/File.h"
#include "io/FormatDescription.h"
#include "io/Stream.h"
#include "metric/MetricFacade.h"
#include "scenegraph/SceneGraphNode.h"
#include "video/Texture.h"
#include "voxelformat/Format.h"
#include "voxelformat/private/rooms/ThingFormat.h"
#include "voxelformat/private/aceofspades/AoSVXLFormat.h"
#include "voxelformat/private/animatoon/AnimaToonFormat.h"
#include "voxelformat/private/binvox/BinVoxFormat.h"
#include "voxelformat/private/chronovox/CSMFormat.h"
#include "voxelformat/private/commandconquer/VXLFormat.h"
#include "voxelformat/private/cubeworld/CubFormat.h"
#include "voxelformat/private/cubzh/CubzhB64Format.h"
#include "voxelformat/private/cubzh/CubzhFormat.h"
#include "voxelformat/private/cubzh/PCubesFormat.h"
#include "voxelformat/private/goxel/GoxFormat.h"
#include "voxelformat/private/magicavoxel/VoxFormat.h"
#include "voxelformat/private/magicavoxel/XRawFormat.h"
#include "voxelformat/private/mesh/FBXFormat.h"
#include "voxelformat/private/mesh/GLTFFormat.h"
#include "voxelformat/private/mesh/OBJFormat.h"
#include "voxelformat/private/mesh/PLYFormat.h"
#include "voxelformat/private/mesh/STLFormat.h"
#include "voxelformat/private/mesh/quake/MD2Format.h"
#include "voxelformat/private/mesh/quake/QuakeBSPFormat.h"
#include "voxelformat/private/minecraft/DatFormat.h"
#include "voxelformat/private/minecraft/MCRFormat.h"
#include "voxelformat/private/minecraft/MTSFormat.h"
#include "voxelformat/private/minecraft/SchematicFormat.h"
#include "voxelformat/private/qubicle/QBCLFormat.h"
#include "voxelformat/private/qubicle/QBFormat.h"
#include "voxelformat/private/qubicle/QBTFormat.h"
#include "voxelformat/private/qubicle/QEFFormat.h"
#include "voxelformat/private/sandbox/VXCFormat.h"
#include "voxelformat/private/sandbox/VXMFormat.h"
#include "voxelformat/private/sandbox/VXRFormat.h"
#include "voxelformat/private/sandbox/VXTFormat.h"
#include "voxelformat/private/slab6/KV6Format.h"
#include "voxelformat/private/slab6/KVXFormat.h"
#include "voxelformat/private/slab6/SLAB6VoxFormat.h"
#include "voxelformat/private/sproxel/SproxelFormat.h"
#include "voxelformat/private/starmade/SMFormat.h"
#include "voxelformat/private/starmade/SMTPLFormat.h"
#include "voxelformat/private/vengi/VENGIFormat.h"
#include "voxelformat/private/voxel3d/V3AFormat.h"
#include "voxelformat/private/voxelbuilder/VBXFormat.h"
#include "voxelformat/private/voxelmax/VMaxFormat.h"

namespace voxelformat {

const io::FormatDescription &aceOfSpades() {
	static io::FormatDescription f{"AceOfSpades", {"vxl"}, {}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED | FORMAT_FLAG_SAVE};
	return f;
}

const io::FormatDescription &tiberianSun() {
	static io::FormatDescription f{"Tiberian Sun",
								   {"vxl"},
								   {"Voxel Animation"},
								   VOX_FORMAT_FLAG_PALETTE_EMBEDDED | VOX_FORMAT_FLAG_ANIMATION | FORMAT_FLAG_SAVE};
	return f;
}

const io::FormatDescription &qubicleBinary() {
	static io::FormatDescription f{
		"Qubicle Binary", {"qb"}, {}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED | FORMAT_FLAG_SAVE};
	return f;
}

const io::FormatDescription &magicaVoxel() {
	static io::FormatDescription f{
		"MagicaVoxel", {"vox"}, {"VOX "}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED | FORMAT_FLAG_SAVE};
	return f;
}

const io::FormatDescription &qubicleBinaryTree() {
	static io::FormatDescription f{
		"Qubicle Binary Tree", {"qbt"}, {"QB 2"}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED | FORMAT_FLAG_SAVE};
	return f;
}

const io::FormatDescription &vengi() {
	static io::FormatDescription f{"Vengi",
								   {"vengi"},
								   {"VENG"},
								   VOX_FORMAT_FLAG_PALETTE_EMBEDDED | VOX_FORMAT_FLAG_ANIMATION | FORMAT_FLAG_SAVE};
	return f;
}

static const io::FormatDescription &roomsThing() {
	static io::FormatDescription f{"Rooms.xyz Thing",
								   {"thing"},
								   {},
								   VOX_FORMAT_FLAG_PALETTE_EMBEDDED};
	return f;
}

static const io::FormatDescription &qubicleProject() {
	static io::FormatDescription f{"Qubicle Project",
								   {"qbcl"},
								   {"QBCL"},
								   VOX_FORMAT_FLAG_SCREENSHOT_EMBEDDED | VOX_FORMAT_FLAG_PALETTE_EMBEDDED |
									   FORMAT_FLAG_SAVE};
	return f;
}

static const io::FormatDescription &qubicleExchange() {
	static io::FormatDescription f{"Qubicle Exchange", {"qef"}, {"Qubicle Exchange Format"}, FORMAT_FLAG_SAVE};
	return f;
}

static const io::FormatDescription &ufoaiBsp() {
	static io::FormatDescription f{"UFO:Alien Invasion", {"bsp"}, {"IBSP"}, VOX_FORMAT_FLAG_MESH};
	return f;
}

static const io::FormatDescription &quake1Bsp() {
	static io::FormatDescription f{"Quake 1", {"bsp"}, {"\x1d"}, VOX_FORMAT_FLAG_MESH};
	return f;
}

static const io::FormatDescription &quakeMd2() {
	static io::FormatDescription f{"Quake 2 Model", {"md2"}, {"IDP2"}, VOX_FORMAT_FLAG_MESH};
	return f;
}

static const io::FormatDescription &sandboxVXM() {
	static io::FormatDescription f{"Sandbox VoxEdit Model",
								   {"vxm"},
								   {"VXMA", "VXMB", "VXMC", "VXM9", "VXM8", "VXM7", "VXM6", "VXM5", "VXM4", "VXM3"},
								   VOX_FORMAT_FLAG_PALETTE_EMBEDDED | FORMAT_FLAG_SAVE};
	return f;
}

static const io::FormatDescription &sandboxVXR() {
	static io::FormatDescription f{"Sandbox VoxEdit Hierarchy",
								   {"vxr"},
								   {"VXR9", "VXR8", "VXR7", "VXR6", "VXR5", "VXR4", "VXR3", "VXR2", "VXR1"},
								   VOX_FORMAT_FLAG_ANIMATION | FORMAT_FLAG_SAVE};
	return f;
}

static const io::FormatDescription &sandboxTilemap() {
	static io::FormatDescription f{"Sandbox VoxEdit Tilemap", {"vxt"}, {"VXT1"}, 0u};
	return f;
}

static const io::FormatDescription &sandboxCollection() {
	static io::FormatDescription f{"Sandbox VoxEdit Collection", {"vxc"}, {}, VOX_FORMAT_FLAG_SCREENSHOT_EMBEDDED};
	return f;
}

static const io::FormatDescription &binvox() {
	static io::FormatDescription f{"BinVox", {"binvox"}, {"#binvox"}, FORMAT_FLAG_SAVE};
	return f;
}

static const io::FormatDescription &goxel() {
	static io::FormatDescription f{"Goxel",
								   {"gox"},
								   {"GOX "},
								   VOX_FORMAT_FLAG_SCREENSHOT_EMBEDDED | VOX_FORMAT_FLAG_PALETTE_EMBEDDED |
									   FORMAT_FLAG_SAVE};
	return f;
}

static const io::FormatDescription &cubeWorld() {
	static io::FormatDescription f{"CubeWorld", {"cub"}, {}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED | FORMAT_FLAG_SAVE};
	return f;
}

static const io::FormatDescription &minetest() {
	static io::FormatDescription f{"Minetest", {"mts"}, {}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED};
	return f;
}

static const io::FormatDescription &minecraftRegion() {
	static io::FormatDescription f{"Minecraft region", {"mca", "mcr"}, {}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED};
	return f;
}

static const io::FormatDescription &minecraftLevelDat() {
	static io::FormatDescription f{"Minecraft level dat", {"dat"}, {}, 0u};
	return f;
}

static const io::FormatDescription &minecraftSchematic() {
	static io::FormatDescription f{
		"Minecraft schematic", {"schematic", "schem", "nbt", "litematic"}, {}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED};
	return f;
}

static io::FormatDescription fbx() {
	static io::FormatDescription f{"FBX", {"fbx"}, {}, VOX_FORMAT_FLAG_MESH | FORMAT_FLAG_SAVE};
	return f;
}

static const io::FormatDescription &sproxelCSV() {
	static io::FormatDescription f{"Sproxel csv", {"csv"}, {}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED | FORMAT_FLAG_SAVE};
	return f;
}

static const io::FormatDescription &magicaVoxelXRAW() {
	static io::FormatDescription f{
		"Magicavoxel XRAW", {"xraw"}, {"XRAW"}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED | FORMAT_FLAG_SAVE};
	return f;
}

const io::FormatDescription &gltf() {
	static io::FormatDescription f{"GL Transmission Format",
								   {"gltf", "glb", "vrm"},
								   {},
								   VOX_FORMAT_FLAG_MESH | VOX_FORMAT_FLAG_ANIMATION | FORMAT_FLAG_SAVE};
	return f;
}

static const io::FormatDescription &particubes() {
	static io::FormatDescription f{"Particubes",
								   {"pcubes", "particubes"},
								   {"PARTICUBES!"},
								   VOX_FORMAT_FLAG_PALETTE_EMBEDDED | VOX_FORMAT_FLAG_SCREENSHOT_EMBEDDED | FORMAT_FLAG_SAVE};
	return f;
}

static const io::FormatDescription &cubzhB64() {
	static io::FormatDescription f{"Cubzh World",
								   {"b64"},
								   {},
								   0u};
	return f;
}

static const io::FormatDescription &cubzh() {
	static io::FormatDescription f{"Cubzh",
								   {"3zh"},
								   {"CUBZH!"},
								   VOX_FORMAT_FLAG_PALETTE_EMBEDDED | VOX_FORMAT_FLAG_SCREENSHOT_EMBEDDED |
									   FORMAT_FLAG_SAVE};
	return f;
}

static const io::FormatDescription &aceOfSpadesKV6() {
	static io::FormatDescription f{
		"AceOfSpades", {"kv6"}, {"Kvxl"}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED | FORMAT_FLAG_SAVE};
	return f;
}

static const io::FormatDescription &voxelMax() {
	static io::FormatDescription f{
		"VoxelMax", {"vmax.zip", "vmaxb"}, {}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED | VOX_FORMAT_FLAG_SCREENSHOT_EMBEDDED};
	return f;
}

static const io::FormatDescription &starMade() {
	static io::FormatDescription f{"StarMade Blueprint", {"sment", "smd2", "smd3"}, {}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED};
	return f;
}

static const io::FormatDescription &starMadeTemplate() {
	static io::FormatDescription f{
		"StarMade Template", {"smtpl"}, {}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED | FORMAT_FLAG_SAVE};
	return f;
}

static const io::FormatDescription &animaToon() {
	static io::FormatDescription f{
		"AnimaToon", {"scn"}, {}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED | VOX_FORMAT_FLAG_ANIMATION};
	return f;
}

static const io::FormatDescription &voxelBuilder() {
	static io::FormatDescription f{"VoxelBuilder", {"vbx"}, {"; Voxel Builder file format (VBX)"}, 0};
	return f;
}

static const io::FormatDescription &wavefrontObj() {
	static io::FormatDescription f{"Wavefront Object", {"obj"}, {}, VOX_FORMAT_FLAG_MESH | FORMAT_FLAG_SAVE};
	return f;
}

static const io::FormatDescription &standardTriangleLanguage() {
	static io::FormatDescription f{
		"Standard Triangle Language", {"stl"}, {}, VOX_FORMAT_FLAG_MESH | FORMAT_FLAG_SAVE};
	return f;
}

static const io::FormatDescription &polygonFileFormat() {
	static io::FormatDescription f{"Polygon File Format", {"ply"}, {}, VOX_FORMAT_FLAG_MESH | FORMAT_FLAG_SAVE};
	return f;
}

static const io::FormatDescription &buildKVX() {
	static io::FormatDescription f{
		"Build engine", {"kvx"}, {}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED | FORMAT_FLAG_SAVE};
	return f;
}

static const io::FormatDescription &voxel3D() {
	static io::FormatDescription f{"Voxel3D", {"v3a"}, {}, FORMAT_FLAG_SAVE};
	return f;
}

static const io::FormatDescription &chronoVox() {
	static io::FormatDescription f{"Chronovox", {"csm"}, {".CSM"}, 0u};
	return f;
}

static const io::FormatDescription &nicksVoxelModel() {
	static io::FormatDescription f{"Nicks Voxel Model", {"nvm"}, {".NVM"}, 0u};
	return f;
}

static const io::FormatDescription &slab6Vox() {
	static io::FormatDescription f{"SLAB6 vox", {"vox"}, {}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED | FORMAT_FLAG_SAVE};
	return f;
}

const io::FormatDescription *voxelLoad() {
	// this is the list of supported voxel volume formats that are
	// have importers implemented
	static const io::FormatDescription desc[] = {vengi(),
												 qubicleBinary(),
												 magicaVoxel(),
												 qubicleBinaryTree(),
												 qubicleProject(),
												 sandboxTilemap(),
												 sandboxCollection(),
												 sandboxVXM(),
												 sandboxVXR(),
												 binvox(),
												 goxel(),
												 cubeWorld(),
												 minetest(),
												 minecraftRegion(),
												 minecraftLevelDat(),
												 minecraftSchematic(),
												 ufoaiBsp(),
												 quake1Bsp(),
												 quakeMd2(),
												 fbx(),
												 sproxelCSV(),
												 magicaVoxelXRAW(),
												 starMade(),
												 starMadeTemplate(),
												 animaToon(),
												 voxelBuilder(),
												 wavefrontObj(),
												 gltf(),
												 standardTriangleLanguage(),
												 polygonFileFormat(),
												 buildKVX(),
												 particubes(),
												 cubzh(),
												 cubzhB64(),
												 voxel3D(),
												 roomsThing(),
												 aceOfSpadesKV6(),
												 tiberianSun(),
												 aceOfSpades(),
												 qubicleExchange(),
												 chronoVox(),
												 nicksVoxelModel(),
												 slab6Vox(),
												 voxelMax(),
												 {"", {}, {}, 0u}};
	return desc;
}

const io::FormatDescription *voxelSave() {
	static core::DynamicArray<io::FormatDescription> desc;
	if (desc.empty()) {
		for (const io::FormatDescription *d = voxelformat::voxelLoad(); d->valid(); ++d) {
			if (d->flags & FORMAT_FLAG_SAVE) {
				desc.push_back(*d);
			}
		}
		desc.push_back({"", {}, {}, 0u});
	}
	return desc.data();
}

static core::SharedPtr<Format> getFormat(const io::FormatDescription &desc, uint32_t magic) {
	for (const core::String &ext : desc.exts) {
		// you only have to check one of the supported extensions
		// here
		if (ext == vengi().mainExtension()) {
			return core::make_shared<VENGIFormat>();
		} else if (ext == qubicleBinary().mainExtension()) {
			return core::make_shared<QBFormat>();
		} else if (ext == magicaVoxel().mainExtension() && desc.name == magicaVoxel().name) {
			return core::make_shared<VoxFormat>();
		} else if (ext == slab6Vox().mainExtension()) {
			return core::make_shared<SLAB6VoxFormat>();
		} else if (ext == qubicleBinaryTree().mainExtension() || io::isA(qubicleBinaryTree(), magic)) {
			return core::make_shared<QBTFormat>();
		} else if (ext == buildKVX().mainExtension()) {
			return core::make_shared<KVXFormat>();
		} else if (ext == aceOfSpadesKV6().mainExtension()) {
			return core::make_shared<KV6Format>();
		} else if (ext == sproxelCSV().mainExtension()) {
			return core::make_shared<SproxelFormat>();
		} else if (ext == cubeWorld().mainExtension()) {
			return core::make_shared<CubFormat>();
		} else if (ext == goxel().mainExtension()) {
			return core::make_shared<GoxFormat>();
		} else if (ext == animaToon().mainExtension()) {
			return core::make_shared<AnimaToonFormat>();
		} else if (ext == minecraftRegion().mainExtension()) {
			return core::make_shared<MCRFormat>();
		} else if (ext == minetest().mainExtension()) {
			return core::make_shared<MTSFormat>();
		} else if (ext == minecraftLevelDat().mainExtension()) {
			return core::make_shared<DatFormat>();
		} else if (ext == starMade().mainExtension()) {
			return core::make_shared<SMFormat>();
		} else if (ext == starMadeTemplate().mainExtension()) {
			return core::make_shared<SMTPLFormat>();
		} else if (ext == sandboxVXM().mainExtension()) {
			return core::make_shared<VXMFormat>();
		} else if (ext == sandboxVXR().mainExtension()) {
			return core::make_shared<VXRFormat>();
		} else if (ext == voxelMax().mainExtension()) {
			return core::make_shared<VMaxFormat>();
		} else if (ext == sandboxCollection().mainExtension()) {
			return core::make_shared<VXCFormat>();
		} else if (ext == sandboxTilemap().mainExtension()) {
			return core::make_shared<VXTFormat>();
		} else if (ext == tiberianSun().mainExtension() && desc.name == tiberianSun().name) {
			return core::make_shared<VXLFormat>();
		} else if (ext == aceOfSpades().mainExtension() && desc.name == aceOfSpades().name) {
			return core::make_shared<AoSVXLFormat>();
		} else if (ext == nicksVoxelModel().mainExtension() || ext == chronoVox().mainExtension()) {
			return core::make_shared<CSMFormat>();
		} else if (ext == binvox().mainExtension()) {
			return core::make_shared<BinVoxFormat>();
		} else if (ext == qubicleExchange().mainExtension()) {
			return core::make_shared<QEFFormat>();
		} else if (ext == qubicleProject().mainExtension()) {
			return core::make_shared<QBCLFormat>();
		} else if (ext == wavefrontObj().mainExtension()) {
			return core::make_shared<OBJFormat>();
		} else if (ext == standardTriangleLanguage().mainExtension()) {
			return core::make_shared<STLFormat>();
		} else if (ext == ufoaiBsp().mainExtension() || ext == quake1Bsp().mainExtension()) {
			return core::make_shared<QuakeBSPFormat>();
		} else if (ext == polygonFileFormat().mainExtension()) {
			return core::make_shared<PLYFormat>();
		} else if (ext == fbx().mainExtension()) {
			return core::make_shared<FBXFormat>();
		} else if (ext == quakeMd2().mainExtension()) {
			return core::make_shared<MD2Format>();
		} else if (ext == minecraftSchematic().mainExtension()) {
			return core::make_shared<SchematicFormat>();
		} else if (ext == voxelBuilder().mainExtension()) {
			return core::make_shared<VBXFormat>();
		} else if (ext == magicaVoxelXRAW().mainExtension()) {
			return core::make_shared<XRawFormat>();
		} else if (ext == voxel3D().mainExtension()) {
			return core::make_shared<V3AFormat>();
		} else if (ext == particubes().mainExtension()) {
			return core::make_shared<PCubesFormat>();
		} else if (ext == particubes().mainExtension() || ext == cubzh().mainExtension()) {
			return core::make_shared<CubzhFormat>();
		} else if (ext == cubzhB64().mainExtension()) {
			return core::make_shared<CubzhB64Format>();
		} else if (ext == roomsThing().mainExtension()) {
			return core::make_shared<ThingFormat>();
		} else if (gltf().matchesExtension(ext)) {
			return core::make_shared<GLTFFormat>();
		} else {
			Log::warn("Unknown extension %s", ext.c_str());
		}
	}
	return {};
}

image::ImagePtr loadScreenshot(const core::String &filename, const io::ArchivePtr &archive, const LoadContext &ctx) {
	core_trace_scoped(LoadVolumeScreenshot);
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::warn("Failed to open file for %s", filename.c_str());
		return image::ImagePtr();
	}
	const uint32_t magic = loadMagic(*stream);

	const io::FormatDescription *desc = io::getDescription(filename, magic, voxelLoad());
	if (desc == nullptr) {
		Log::warn("Format %s isn't supported for loading screenshots", filename.c_str());
		return image::ImagePtr();
	}
	if (!(desc->flags & VOX_FORMAT_FLAG_SCREENSHOT_EMBEDDED)) {
		Log::debug("Format %s doesn't have a screenshot embedded", desc->name.c_str());
		return image::ImagePtr();
	}
	const core::SharedPtr<Format> &f = getFormat(*desc, magic);
	if (f) {
		return f->loadScreenshot(filename, archive, ctx);
	}
	Log::error("Failed to load model screenshot from file %s - "
			   "unsupported file format",
			   filename.c_str());
	return image::ImagePtr();
}

bool importPalette(const core::String &filename, palette::Palette &palette) {
	if (io::isA(filename, io::format::palettes())) {
		return palette.load(filename.c_str());
	}
	if (io::isA(filename, voxelformat::voxelLoad())) {
		const io::ArchivePtr &archive = io::openFilesystemArchive(io::filesystem());
		LoadContext loadCtx;
		if (voxelformat::loadPalette(filename, archive, palette, loadCtx) <= 0) {
			Log::warn("Failed to load palette from %s", filename.c_str());
			return false;
		}
		return true;
	}
	Log::warn("Given file is not supported as palette source: %s", filename.c_str());
	return false;
}

size_t loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
				   const LoadContext &ctx) {
	core_trace_scoped(LoadVolumePalette);
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::warn("Failed to open palette file at %s", filename.c_str());
		return 0;
	}
	const uint32_t magic = loadMagic(*stream);
	const io::FormatDescription *desc = io::getDescription(filename, magic, voxelLoad());
	if (desc == nullptr) {
		Log::warn("Format %s isn't supported", filename.c_str());
		return 0;
	}
	if (!(desc->flags & VOX_FORMAT_FLAG_PALETTE_EMBEDDED)) {
		Log::warn("Format %s doesn't have a palette embedded", desc->name.c_str());
		return 0;
	}
	if (const core::SharedPtr<Format> &f = getFormat(*desc, magic)) {
		const size_t n = f->loadPalette(filename, archive, palette, ctx);
		palette.markDirty();
		return n;
	}
	Log::error("Failed to load model palette from file %s - "
			   "unsupported file format",
			   filename.c_str());
	return 0;
}

bool loadFormat(const io::FileDescription &fileDesc, const io::ArchivePtr &archive,
				scenegraph::SceneGraph &newSceneGraph, const LoadContext &ctx) {
	core_trace_scoped(LoadVolumeFormat);
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(fileDesc.name));
	if (!stream) {
		Log::warn("Failed to open file at %s", fileDesc.name.c_str());
		return false;
	}
	const uint32_t magic = loadMagic(*stream);
	const io::FormatDescription *desc = io::getDescription(fileDesc, magic, voxelLoad());
	if (desc == nullptr) {
		return false;
	}
	const core::String &filename = fileDesc.name;
	const core::SharedPtr<Format> &f = getFormat(*desc, magic);
	if (f) {
		if (!f->load(filename, archive, newSceneGraph, ctx)) {
			Log::error("Error while loading %s", filename.c_str());
			newSceneGraph.clear();
		}
	} else {
		Log::error("Failed to load model file %s - unsupported "
				   "file format",
				   filename.c_str());
		return false;
	}
	const int models = (int)newSceneGraph.size(scenegraph::SceneGraphNodeType::Model);
	const int points = (int)newSceneGraph.size(scenegraph::SceneGraphNodeType::Point);
	if (models == 0 && points == 0) {
		Log::error("Failed to load model file %s. Scene graph "
				"doesn't contain models.",
				filename.c_str());
		return false;
	}
	Log::info("Load file %s with %i model nodes and %i point nodes", filename.c_str(), models, points);
	const core::String &ext = core::string::extractExtension(filename);
	if (!ext.empty()) {
		metric::count("load", 1, {{"type", ext}});
	}
	return true;
}

bool isMeshFormat(const io::FormatDescription &desc) {
	return desc.flags & VOX_FORMAT_FLAG_MESH;
}

bool isAnimationSupported(const io::FormatDescription &desc) {
	return desc.flags & VOX_FORMAT_FLAG_ANIMATION;
}

bool isMeshFormat(const core::String &filename, bool save) {
	const core::String &ext = core::string::extractExtension(filename);
	for (const io::FormatDescription *desc = save ? voxelformat::voxelSave() : voxelformat::voxelLoad(); desc->valid();
		 ++desc) {
		if (desc->matchesExtension(ext) && isMeshFormat(*desc)) {
			return true;
		}
	}

	return false;
}

bool isModelFormat(const core::String &filename) {
	const core::String &ext = core::string::extractExtension(filename);
	for (const io::FormatDescription *desc = voxelformat::voxelLoad(); desc->valid(); ++desc) {
		if (desc->matchesExtension(ext)) {
			return true;
		}
	}

	return false;
}

bool saveFormat(scenegraph::SceneGraph &sceneGraph, const core::String &filename, const io::FormatDescription *desc,
				const io::ArchivePtr &archive, const SaveContext &ctx) {
	if (sceneGraph.empty()) {
		Log::error("Failed to save model file %s - no volumes given", filename.c_str());
		return false;
	}
	const core::String &ext = core::string::extractExtension(filename);
	if (desc) {
		if (!desc->matchesExtension(ext)) {
			desc = nullptr;
		}
	}
	if (desc != nullptr) {
		core::SharedPtr<Format> f = getFormat(*desc, 0u);
		if (f) {
			if (f->save(sceneGraph, filename, archive, ctx)) {
				Log::debug("Saved file for format '%s' (ext: '%s')", desc->name.c_str(), ext.c_str());
				metric::count("save", 1, {{"type", ext}});
				return true;
			}
			Log::error("Failed to save %s file", desc->name.c_str());
			return false;
		}
	}
	for (desc = voxelformat::voxelSave(); desc->valid(); ++desc) {
		if (desc->matchesExtension(ext)) {
			core::SharedPtr<Format> f = getFormat(*desc, 0u);
			if (f) {
				if (f->save(sceneGraph, filename, archive, ctx)) {
					Log::debug("Saved file for format '%s' (ext: '%s')", desc->name.c_str(), ext.c_str());
					metric::count("save", 1, {{"type", ext}});
					return true;
				}
				Log::error("Failed to save %s file", desc->name.c_str());
				return false;
			}
		}
	}
	return false;
}

} // namespace voxelformat
