/**
 * @file
 */

#include "VolumeFormat.h"
#include "app/App.h"
#include "core/ArrayLength.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/SharedPtr.h"
#include "core/StringUtil.h"
#include "core/Trace.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "io/FormatDescription.h"
#include "io/Stream.h"
#include "video/Texture.h"
#include "voxelformat/Format.h"
#include "voxelformat/private/aceofspades/AoSVXLFormat.h"
#include "voxelformat/private/animatoon/AnimaToonFormat.h"
#include "voxelformat/private/binvox/BinVoxFormat.h"
#include "voxelformat/private/chronovox/CSMFormat.h"
#include "voxelformat/private/commandconquer/VXLFormat.h"
#include "voxelformat/private/cubeworld/CubFormat.h"
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
#include "voxelformat/private/vengi/VENGIFormat.h"
#include "voxelformat/private/voxel3d/V3AFormat.h"
#include "voxelformat/private/voxelbuilder/VBXFormat.h"
#include "voxelformat/private/voxelmax/VMaxFormat.h"

namespace voxelformat {

io::FormatDescription aceOfSpades() {
	return {"AceOfSpades", {"vxl"}, nullptr, VOX_FORMAT_FLAG_PALETTE_EMBEDDED};
}

io::FormatDescription tiberianSun() {
	return {"Tiberian Sun",
			{"vxl"},
			[](uint32_t magic) { return magic == FourCC('V', 'o', 'x', 'e'); },
			VOX_FORMAT_FLAG_PALETTE_EMBEDDED};
}

io::FormatDescription qubicleBinary() {
	return {"Qubicle Binary", {"qb"}, nullptr, VOX_FORMAT_FLAG_PALETTE_EMBEDDED};
}

io::FormatDescription magicaVoxel() {
	return {"MagicaVoxel",
			{"vox"},
			[](uint32_t magic) { return magic == FourCC('V', 'O', 'X', ' '); },
			VOX_FORMAT_FLAG_PALETTE_EMBEDDED};
}

io::FormatDescription qubicleBinaryTree() {
	return {"Qubicle Binary Tree",
			{"qbt"},
			[](uint32_t magic) { return magic == FourCC('Q', 'B', ' ', '2'); },
			VOX_FORMAT_FLAG_PALETTE_EMBEDDED};
}

io::FormatDescription vengi() {
	return {"Vengi",
			{"vengi"},
			[](uint32_t magic) { return magic == FourCC('V', 'E', 'N', 'G'); },
			VOX_FORMAT_FLAG_PALETTE_EMBEDDED};
}

const io::FormatDescription *voxelLoad() {
	// this is the list of supported voxel volume formats that are have importers implemented
	static const io::FormatDescription desc[] = {
		vengi(),
		qubicleBinary(),
		magicaVoxel(),
		qubicleBinaryTree(),
		{"Qubicle Project",
		 {"qbcl"},
		 [](uint32_t magic) { return magic == FourCC('Q', 'B', 'C', 'L'); },
		 VOX_FORMAT_FLAG_SCREENSHOT_EMBEDDED | VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
		{"Sandbox VoxEdit Tilemap", {"vxt"}, [](uint32_t magic) { return magic == FourCC('V', 'X', 'T', '1'); }, 0u},
		{"Sandbox VoxEdit Collection", {"vxc"}, nullptr, 0u},
		{"Sandbox VoxEdit Model",
		 {"vxm"},
		 [](uint32_t magic) {
			 return magic == FourCC('V', 'X', 'M', 'A') || magic == FourCC('V', 'X', 'M', 'B') ||
					magic == FourCC('V', 'X', 'M', 'C') || magic == FourCC('V', 'X', 'M', '9') ||
					magic == FourCC('V', 'X', 'M', '8') || magic == FourCC('V', 'X', 'M', '7') ||
					magic == FourCC('V', 'X', 'M', '6') || magic == FourCC('V', 'X', 'M', '5') ||
					magic == FourCC('V', 'X', 'M', '4') || magic == FourCC('V', 'X', 'M', '3');
		 },
		 VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
		{"Sandbox VoxEdit Hierarchy",
		 {"vxr"},
		 [](uint32_t magic) {
			 return magic == FourCC('V', 'X', 'R', '9') || magic == FourCC('V', 'X', 'R', '8') ||
					magic == FourCC('V', 'X', 'R', '7') || magic == FourCC('V', 'X', 'R', '6') ||
					magic == FourCC('V', 'X', 'R', '5') || magic == FourCC('V', 'X', 'R', '4') ||
					magic == FourCC('V', 'X', 'R', '3') || magic == FourCC('V', 'X', 'R', '2') ||
					magic == FourCC('V', 'X', 'R', '1');
		 },
		 0u},
		{"BinVox", {"binvox"}, [](uint32_t magic) { return magic == FourCC('#', 'b', 'i', 'n'); }, 0u},
		{"Goxel",
		 {"gox"},
		 [](uint32_t magic) { return magic == FourCC('G', 'O', 'X', ' '); },
		 VOX_FORMAT_FLAG_SCREENSHOT_EMBEDDED | VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
		{"CubeWorld", {"cub"}, nullptr, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
		{"Minetest", {"mts"}, nullptr, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
		{"Minecraft region", {"mca", "mcr"}, nullptr, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
		{"Minecraft level dat", {"dat"}, nullptr, 0u},
		{"Minecraft schematic", {"schematic", "schem", "nbt"}, nullptr, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
		{"Quake BSP",
		 {"bsp"},
		 [](uint32_t magic) {
			 return magic == FourCC('I', 'B', 'S', 'P') || magic == FourCC('\x1d', '\0', '\0', '\0');
		 },
		 VOX_FORMAT_FLAG_MESH},
		{"Quake 2 Model",
		 {"md2"},
		 [](uint32_t magic) { return magic == FourCC('I', 'D', 'P', '2'); },
		 VOX_FORMAT_FLAG_MESH},
		{"FBX", {"fbx"}, nullptr, VOX_FORMAT_FLAG_MESH},
		{"Sproxel csv", {"csv"}, nullptr, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
		{"Magicavoxel XRAW", {"xraw"}, [](uint32_t magic) { return magic == FourCC('X', 'R', 'A', 'W'); }, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
		{"StarMade", {"sment"}, nullptr, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
		{"AnimaToon", {"scn"}, nullptr, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
		{"VoxelBuilder", {"vbx"}, [](uint32_t magic) { return magic == FourCC(';', ' ', 'V', 'o'); }, 0},
		{"Wavefront Object", {"obj"}, nullptr, VOX_FORMAT_FLAG_MESH},
		{"GL Transmission Format", {"gltf", "glb", "vrm"}, nullptr, VOX_FORMAT_FLAG_MESH},
		{"Standard Triangle Language", {"stl"}, nullptr, VOX_FORMAT_FLAG_MESH},
		{"Polygon File Format", {"ply"}, nullptr, VOX_FORMAT_FLAG_MESH},
		{"Build engine", {"kvx"}, nullptr, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
		{"Voxel3D", {"v3a"}, nullptr, 0u},
		{"AceOfSpades",
		 {"kv6"},
		 [](uint32_t magic) { return magic == FourCC('K', 'v', 'x', 'l'); },
		 VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
		tiberianSun(),
		aceOfSpades(),
		{"Qubicle Exchange", {"qef"}, [](uint32_t magic) { return magic == FourCC('Q', 'u', 'b', 'i'); }, 0u},
		{"Chronovox", {"csm"}, [](uint32_t magic) { return magic == FourCC('.', 'C', 'S', 'M'); }, 0u},
		{"Nicks Voxel Model", {"nvm"}, [](uint32_t magic) { return magic == FourCC('.', 'N', 'V', 'M'); }, 0u},
		{"SLAB6 vox", {"vox"}, nullptr, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
		{"VoxelMax",
		 {"vmax.zip", "vmaxb"},
		 nullptr,
		 VOX_FORMAT_FLAG_PALETTE_EMBEDDED | VOX_FORMAT_FLAG_SCREENSHOT_EMBEDDED},
		{"", {}, nullptr, 0u}};
	return desc;
}

const io::FormatDescription *voxelSave() {
	// this is the list of supported voxel or mesh formats that have exporters implemented
	static const io::FormatDescription desc[] = {{"Vengi", {"vengi"}, nullptr, 0u},
												 {"Qubicle Binary", {"qb"}, nullptr, 0u},
												 {"MagicaVoxel", {"vox"}, nullptr, 0u},
												 {"MagicaVoxel", {"xraw"}, nullptr, 0u},
												 {"AceOfSpades", {"kv6"}, nullptr, 0u},
												 {"Build engine", {"kvx"}, nullptr, 0u},
												 {"SLAB6 vox", {"vox"}, nullptr, 0u},
												 {"Voxel3D", {"v3a"}, nullptr, 0u},
												 {"Qubicle Binary Tree", {"qbt"}, nullptr, 0u},
												 {"Qubicle Project", {"qbcl"}, nullptr, 0u},
												 {"Sandbox VoxEdit Model", {"vxm"}, nullptr, 0u},
												 {"Sandbox VoxEdit Hierarchy", {"vxr"}, nullptr, 0u},
												 {"BinVox", {"binvox"}, nullptr, 0u},
												 {"Goxel", {"gox"}, nullptr, 0u},
												 {"Sproxel csv", {"csv"}, nullptr, 0u},
												 {"CubeWorld", {"cub"}, nullptr, 0u},
												 //{"Build engine", {"kvx"}, nullptr, 0u},
												 tiberianSun(),
												 {"Qubicle Exchange", {"qef"}, nullptr, 0u},
												 {"AceOfSpades", {"vxl"}, nullptr, 0u},
												 //{"Minecraft schematic", {"schematic", "schem", "nbt"}, nullptr, 0u},
												 {"Wavefront Object", {"obj"}, nullptr, VOX_FORMAT_FLAG_MESH},
												 {"Polygon File Format", {"ply"}, nullptr, VOX_FORMAT_FLAG_MESH},
												 {"FBX Ascii", {"fbx"}, nullptr, VOX_FORMAT_FLAG_MESH},
												 {"Standard Triangle Language", {"stl"}, nullptr, VOX_FORMAT_FLAG_MESH},
												 {"GLTF Binary", {"glb"}, nullptr, VOX_FORMAT_FLAG_MESH},
												 {"GLTF Text", {"gltf"}, nullptr, VOX_FORMAT_FLAG_MESH},
												 {"", {}, nullptr, 0u}};
	return desc;
}

static uint32_t loadMagic(io::SeekableReadStream &stream) {
	uint32_t magicWord = 0u;
	stream.peekUInt32(magicWord);
	return magicWord;
}

static const io::FormatDescription *getDescription(const core::String &filename, uint32_t magic) {
	const core::String ext = core::string::extractExtension(filename);
	const core::String extFull = core::string::extractAllExtensions(filename);
	for (const io::FormatDescription *desc = voxelLoad(); desc->valid(); ++desc) {
		if (!desc->matchesExtension(ext) && !desc->matchesExtension(extFull)) {
			continue;
		}
		if (magic > 0 && desc->isA && !desc->isA(magic)) {
			Log::debug("File doesn't have the expected magic number");
			continue;
		}
		return desc;
	}
	if (magic > 0) {
		// search again - but this time only the magic bytes...
		for (const io::FormatDescription *desc = voxelLoad(); desc->valid(); ++desc) {
			if (!desc->isA) {
				continue;
			}
			if (!desc->isA(magic)) {
				continue;
			}
			return desc;
		}
	}
	if (extFull.empty()) {
		Log::warn("Could not identify the format");
	} else {
		Log::warn("Could not find a supported format description for %s", extFull.c_str());
	}
	return nullptr;
}

static const io::FormatDescription *getDescription(const io::FileDescription &fileDesc, uint32_t magic) {
	if (fileDesc.desc.valid()) {
		return &fileDesc.desc;
	}
	const core::String filename = fileDesc.name;
	return getDescription(filename, magic);
}

static core::SharedPtr<Format> getFormat(const io::FormatDescription &desc, uint32_t magic, bool load) {
	core::SharedPtr<Format> format;
	for (const core::String &ext : desc.exts) {
		// you only have to check one of the supported extensions here
		if (ext == "vengi") {
			format = core::make_shared<VENGIFormat>();
		} else if (ext == "qb") {
			format = core::make_shared<QBFormat>();
		} else if (ext == "vox" && desc.name == magicaVoxel().name) {
			format = core::make_shared<VoxFormat>();
		} else if (ext == "vox") {
			format = core::make_shared<SLAB6VoxFormat>();
		} else if (ext == "qbt" || magic == FourCC('Q', 'B', ' ', '2')) {
			format = core::make_shared<QBTFormat>();
		} else if (ext == "kvx") {
			format = core::make_shared<KVXFormat>();
		} else if (ext == "kv6") {
			format = core::make_shared<KV6Format>();
		} else if (ext == "csv") {
			format = core::make_shared<SproxelFormat>();
		} else if (ext == "cub") {
			format = core::make_shared<CubFormat>();
		} else if (ext == "gox") {
			format = core::make_shared<GoxFormat>();
		} else if (ext == "scn") {
			format = core::make_shared<AnimaToonFormat>();
		} else if (ext == "mca") {
			format = core::make_shared<MCRFormat>();
		} else if (ext == "mts") {
			format = core::make_shared<MTSFormat>();
		} else if (ext == "dat") {
			format = core::make_shared<DatFormat>();
		} else if (ext == "sment") {
			format = core::make_shared<SMFormat>();
		} else if (ext == "vxm") {
			format = core::make_shared<VXMFormat>();
		} else if (ext == "vxr") {
			format = core::make_shared<VXRFormat>();
		} else if (ext == "vmax.zip") {
			format = core::make_shared<VMaxFormat>();
		} else if (ext == "vxc") {
			format = core::make_shared<VXCFormat>();
		} else if (ext == "vxt") {
			format = core::make_shared<VXTFormat>();
		} else if (ext == "vxl" && desc.name == tiberianSun().name) {
			format = core::make_shared<VXLFormat>();
		} else if (ext == "vxl" && desc.name == aceOfSpades().name) {
			format = core::make_shared<AoSVXLFormat>();
		} else if (ext == "csm" || ext == "nvm") {
			format = core::make_shared<CSMFormat>();
		} else if (ext == "binvox") {
			format = core::make_shared<BinVoxFormat>();
		} else if (ext == "qef") {
			format = core::make_shared<QEFFormat>();
		} else if (ext == "qbcl") {
			format = core::make_shared<QBCLFormat>();
		} else if (ext == "obj") {
			format = core::make_shared<OBJFormat>();
		} else if (ext == "stl") {
			format = core::make_shared<STLFormat>();
		} else if (ext == "bsp") {
			format = core::make_shared<QuakeBSPFormat>();
		} else if (ext == "ply") {
			format = core::make_shared<PLYFormat>();
		} else if (ext == "fbx") {
			format = core::make_shared<FBXFormat>();
		} else if (ext == "md2") {
			format = core::make_shared<MD2Format>();
		} else if (ext == "schematic") {
			format = core::make_shared<SchematicFormat>();
		} else if (ext == "vbx") {
			format = core::make_shared<VBXFormat>();
		} else if (ext == "xraw") {
			format = core::make_shared<XRawFormat>();
		} else if (ext == "v3a") {
			format = core::make_shared<V3AFormat>();
		} else if (ext == "gltf" || ext == "glb" || ext == "vrm") {
			format = core::make_shared<GLTFFormat>();
		} else {
			Log::warn("Unknown extension %s", ext.c_str());
		}
		if (format) {
			return format;
		}
	}
	return format;
}

image::ImagePtr loadScreenshot(const core::String &filename, io::SeekableReadStream &stream, const LoadContext &ctx) {
	core_trace_scoped(LoadVolumeScreenshot);
	const uint32_t magic = loadMagic(stream);

	const io::FormatDescription *desc = getDescription(filename, magic);
	if (desc == nullptr) {
		Log::warn("Format %s isn't supported", filename.c_str());
		return image::ImagePtr();
	}
	if (!(desc->flags & VOX_FORMAT_FLAG_SCREENSHOT_EMBEDDED)) {
		Log::warn("Format %s doesn't have a screenshot embedded", desc->name.c_str());
		return image::ImagePtr();
	}
	const core::SharedPtr<Format> &f = getFormat(*desc, magic, true);
	if (f) {
		stream.seek(0);
		return f->loadScreenshot(filename, stream, ctx);
	}
	Log::error("Failed to load model screenshot from file %s - unsupported file format", filename.c_str());
	return image::ImagePtr();
}

bool importPalette(const core::String &filename, voxel::Palette &palette) {
	if (io::isA(filename, io::format::palettes())) {
		return palette.load(filename.c_str());
	}
	if (io::isA(filename, voxelformat::voxelLoad())) {
		const io::FilesystemPtr &fs = io::filesystem();
		const io::FilePtr &palFile = fs->open(filename);
		if (!palFile->validHandle()) {
			Log::warn("Failed to open palette file at %s", filename.c_str());
			return false;
		}
		io::FileStream stream(palFile);
		LoadContext loadCtx;
		if (voxelformat::loadPalette(filename, stream, palette, loadCtx) <= 0) {
			Log::warn("Failed to load palette from %s", filename.c_str());
			return false;
		}
		return true;
	}
	Log::warn("Given file is not supported as palette source: %s", filename.c_str());
	return false;
}

size_t loadPalette(const core::String &filename, io::SeekableReadStream &stream, voxel::Palette &palette,
				   const LoadContext &ctx) {
	core_trace_scoped(LoadVolumePalette);
	const uint32_t magic = loadMagic(stream);
	const io::FormatDescription *desc = getDescription(filename, magic);
	if (desc == nullptr) {
		Log::warn("Format %s isn't supported", filename.c_str());
		return 0;
	}
	if (!(desc->flags & VOX_FORMAT_FLAG_PALETTE_EMBEDDED)) {
		Log::warn("Format %s doesn't have a palette embedded", desc->name.c_str());
		return 0;
	}
	const core::SharedPtr<Format> &f = getFormat(*desc, magic, true);
	if (f) {
		stream.seek(0);
		const size_t n = f->loadPalette(filename, stream, palette, ctx);
		palette.markDirty();
		return n;
	}
	Log::error("Failed to load model palette from file %s - unsupported file format", filename.c_str());
	return 0;
}

bool loadFormat(const io::FileDescription &fileDesc, io::SeekableReadStream &stream,
				scenegraph::SceneGraph &newSceneGraph, const LoadContext &ctx) {
	core_trace_scoped(LoadVolumeFormat);
	const uint32_t magic = loadMagic(stream);
	const io::FormatDescription *desc = getDescription(fileDesc, magic);
	if (desc == nullptr) {
		return false;
	}
	const core::String &filename = fileDesc.name;
	const core::SharedPtr<Format> &f = getFormat(*desc, magic, true);
	if (f) {
		if (!f->load(filename, stream, newSceneGraph, ctx)) {
			Log::error("Error while loading %s", filename.c_str());
			newSceneGraph.clear();
		}
	} else {
		Log::error("Failed to load model file %s - unsupported file format", filename.c_str());
		return false;
	}
	if (newSceneGraph.empty()) {
		Log::error("Failed to load model file %s. Scene graph doesn't contain models.", filename.c_str());
		return false;
	}
	// newSceneGraph.node(newSceneGraph.root().id()).setProperty("Type", desc->name);
	Log::info("Load file %s with %i model nodes", filename.c_str(), (int)newSceneGraph.size());
	return true;
}

bool isMeshFormat(const io::FormatDescription &desc) {
	return desc.flags & VOX_FORMAT_FLAG_MESH;
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
				io::SeekableWriteStream &stream, const SaveContext &ctx) {
	if (sceneGraph.empty()) {
		Log::error("Failed to save model file %s - no volumes given", filename.c_str());
		return false;
	}
	const core::String &type = sceneGraph.root().property("Type");
	if (!type.empty()) {
		Log::debug("Save '%s' file to '%s'", type.c_str(), filename.c_str());
	}
	const core::String &ext = core::string::extractExtension(filename);
	if (desc) {
		if (!desc->matchesExtension(ext)) {
			desc = nullptr;
		}
	}
	if (desc != nullptr) {
		core::SharedPtr<Format> f = getFormat(*desc, 0u, false);
		if (f) {
			if (f->save(sceneGraph, filename, stream, ctx)) {
				Log::debug("Saved file for format '%s' (ext: '%s')", desc->name.c_str(), ext.c_str());
				return true;
			}
			Log::error("Failed to save %s file", desc->name.c_str());
			return false;
		}
	}
	for (desc = voxelformat::voxelSave(); desc->valid(); ++desc) {
		if (desc->matchesExtension(ext) /*&& (type.empty() || type == desc->name)*/) {
			core::SharedPtr<Format> f = getFormat(*desc, 0u, false);
			if (f) {
				if (f->save(sceneGraph, filename, stream, ctx)) {
					Log::debug("Saved file for format '%s' (ext: '%s')", desc->name.c_str(), ext.c_str());
					return true;
				}
				Log::error("Failed to save %s file", desc->name.c_str());
				return false;
			}
		}
	}
	return false;
}

bool saveFormat(const io::FilePtr &filePtr, const io::FormatDescription *desc, scenegraph::SceneGraph &sceneGraph,
				const SaveContext &ctx) {
	if (!filePtr->validHandle()) {
		Log::error("Failed to save model - no valid file given");
		return false;
	}

	io::FileStream stream(filePtr);
	if (!saveFormat(sceneGraph, filePtr->name(), desc, stream, ctx)) {
		Log::warn("Failed to save file %s - saving as vengi instead", filePtr->name().c_str());
		VENGIFormat vengiFormat;
		const core::String &newName = core::string::replaceExtension(filePtr->name(), "vengi");
		io::FileStream newStream(io::filesystem()->open(newName, io::FileMode::SysWrite));
		return vengiFormat.save(sceneGraph, newName, newStream, ctx);
	}
	return true;
}

} // namespace voxelformat
