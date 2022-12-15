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
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "io/FormatDescription.h"
#include "io/Stream.h"
#include "video/Texture.h"
#include "voxelformat/AoSVXLFormat.h"
#include "voxelformat/BinVoxFormat.h"
#include "voxelformat/CSMFormat.h"
#include "voxelformat/CubFormat.h"
#include "voxelformat/DatFormat.h"
#include "voxelformat/FBXFormat.h"
#include "voxelformat/Format.h"
#include "voxelformat/GLTFFormat.h"
#include "voxelformat/GoxFormat.h"
#include "voxelformat/KV6Format.h"
#include "voxelformat/KVXFormat.h"
#include "voxelformat/MCRFormat.h"
#include "voxelformat/OBJFormat.h"
#include "voxelformat/PLYFormat.h"
#include "voxelformat/QBCLFormat.h"
#include "voxelformat/QBFormat.h"
#include "voxelformat/QBTFormat.h"
#include "voxelformat/QEFFormat.h"
#include "voxelformat/QuakeBSPFormat.h"
#include "voxelformat/SLAB6VoxFormat.h"
#include "voxelformat/SMFormat.h"
#include "voxelformat/STLFormat.h"
#include "voxelformat/SchematicFormat.h"
#include "voxelformat/SproxelFormat.h"
#include "voxelformat/VXCFormat.h"
#include "voxelformat/VXLFormat.h"
#include "voxelformat/VXMFormat.h"
#include "voxelformat/VXRFormat.h"
#include "voxelformat/VXTFormat.h"
#include "voxelformat/VoxFormat.h"

namespace voxelformat {

// this is the list of internal formats that are supported engine-wide (the format we save our own models in)
const char *SUPPORTED_VOXEL_FORMATS_LOAD_LIST[] = {"qb", "vox", nullptr};

const io::FormatDescription* voxelLoad() {
	// this is the list of supported voxel volume formats that are have importers implemented
	static const io::FormatDescription desc[] = {
		{"Qubicle Binary", {"qb"}, nullptr, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
		{"MagicaVoxel", {"vox"}, [] (uint32_t magic) {return magic == FourCC('V','O','X',' ');}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
		{"Qubicle Binary Tree", {"qbt"}, [] (uint32_t magic) {return magic == FourCC('Q','B',' ','2');}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
		{"Qubicle Project", {"qbcl"}, [] (uint32_t magic) {return magic == FourCC('Q','B','C','L');}, VOX_FORMAT_FLAG_SCREENSHOT_EMBEDDED | VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
		{"Sandbox VoxEdit Tilemap", {"vxt"}, [] (uint32_t magic) {return magic == FourCC('V','X','T','1');}, 0u},
		{"Sandbox VoxEdit Collection", {"vxc"}, nullptr, 0u},
		{"Sandbox VoxEdit Model", {"vxm"}, [] (uint32_t magic) {return magic == FourCC('V','X','M','A')
				|| magic == FourCC('V','X','M','B') || magic == FourCC('V','X','M','C')
				|| magic == FourCC('V','X','M','9') || magic == FourCC('V','X','M','8')
				|| magic == FourCC('V','X','M','7') || magic == FourCC('V','X','M','6')
				|| magic == FourCC('V','X','M','5') || magic == FourCC('V','X','M','4');}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
		{"Sandbox VoxEdit Hierarchy", {"vxr"}, [] (uint32_t magic) {
			return magic == FourCC('V','X','R','9') || magic == FourCC('V','X','R','8')
				|| magic == FourCC('V','X','R','7') || magic == FourCC('V','X','R','6')
				|| magic == FourCC('V','X','R','5') || magic == FourCC('V','X','R','4')
				|| magic == FourCC('V','X','R','3') || magic == FourCC('V','X','R','2')
				|| magic == FourCC('V','X','R','1');}, 0u},
		{"BinVox", {"binvox"}, [] (uint32_t magic) {return magic == FourCC('#','b','i','n');}, 0u},
		{"Goxel", {"gox"}, [] (uint32_t magic) {return magic == FourCC('G','O','X',' ');}, VOX_FORMAT_FLAG_SCREENSHOT_EMBEDDED | VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
		{"CubeWorld", {"cub"}, nullptr, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
		{"Minecraft region", {"mca", "mcr"}, nullptr, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
		{"Minecraft level dat", {"dat"}, nullptr, 0u},
		{"Minecraft schematic", {"schematic", "schem", "nbt"}, nullptr, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
		{"Quake BSP", {"bsp"}, [](uint32_t magic) {
				return magic == FourCC('I', 'B', 'S', 'P') || magic == FourCC('\x1d', '\0', '\0', '\0');
		}, VOX_FORMAT_FLAG_MESH},
		{"Sproxel csv", {"csv"}, nullptr, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
		{"StarMade", {"sment"}, nullptr, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
		{"Wavefront Object", {"obj"}, nullptr, VOX_FORMAT_FLAG_MESH},
		{"GL Transmission Format", {"gltf", "glb"}, nullptr, VOX_FORMAT_FLAG_MESH},
		{"Standard Triangle Language", {"stl"}, nullptr, VOX_FORMAT_FLAG_MESH},
		{"Build engine", {"kvx"}, nullptr, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
		{"AceOfSpades", {"kv6"}, [] (uint32_t magic) {return magic == FourCC('K','v','x','l');}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
		{"Tiberian Sun", {"vxl"}, [] (uint32_t magic) {return magic == FourCC('V','o','x','e');}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
		{"AceOfSpades", {"vxl"}, nullptr, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
		{"Qubicle Exchange", {"qef"}, [](uint32_t magic) { return magic == FourCC('Q', 'u', 'b', 'i'); }, 0u},
		{"Chronovox", {"csm"}, [](uint32_t magic) { return magic == FourCC('.', 'C', 'S', 'M'); }, 0u},
		{"Nicks Voxel Model", {"nvm"}, [](uint32_t magic) { return magic == FourCC('.', 'N', 'V', 'M'); }, 0u},
		{"SLAB6 vox", {"vox"}, nullptr, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
		{"", {}, nullptr, 0u}
	};
	return desc;
}

const io::FormatDescription* voxelSave() {
	// this is the list of supported voxel or mesh formats that have exporters implemented
	static const io::FormatDescription desc[] = {
		{"Qubicle Binary", {"qb"}, nullptr, 0u},
		{"MagicaVoxel", {"vox"}, nullptr, 0u},
		{"AceOfSpades", {"kv6"}, nullptr, 0u},
		{"SLAB6 vox", {"vox"}, nullptr, 0u}, // TODO: handle duplicate extension
		{"Qubicle Binary Tree", {"qbt"}, nullptr, 0u},
		{"Qubicle Project", {"qbcl"}, nullptr, 0u},
		{"Sandbox VoxEdit Model", {"vxm"}, nullptr, 0u},
		{"Sandbox VoxEdit Hierarchy", {"vxr"}, nullptr, 0u},
		{"BinVox", {"binvox"}, nullptr, 0u},
		{"Goxel", {"gox"}, nullptr, 0u},
		{"Sproxel csv", {"csv"}, nullptr, 0u},
		{"CubeWorld", {"cub"}, nullptr, 0u},
		//{"Build engine", {"kvx"}, nullptr, 0u},
		{"Tiberian Sun", {"vxl"}, nullptr, 0u},
		{"Qubicle Exchange", {"qef"}, nullptr, 0u},
		{"AceOfSpades", {"vxl"}, nullptr, 0u}, // TODO: handle duplicate extension
		//{"Minecraft schematic", {"schematic", "schem", "nbt"}, nullptr, 0u},
		{"Wavefront Object", {"obj"}, nullptr, VOX_FORMAT_FLAG_MESH},
		{"Polygon File Format", {"ply"}, nullptr, VOX_FORMAT_FLAG_MESH},
		{"FBX Ascii", {"fbx"}, nullptr, VOX_FORMAT_FLAG_MESH},
		{"Standard Triangle Language", {"stl"}, nullptr, VOX_FORMAT_FLAG_MESH},
		{"GL Transmission Format", {"gltf", "glb"}, nullptr, VOX_FORMAT_FLAG_MESH},
		{"", {}, nullptr, 0u}
	};
	return desc;
}

static uint32_t loadMagic(io::SeekableReadStream &stream) {
	uint32_t magicWord = 0u;
	stream.peekUInt32(magicWord);
	return magicWord;
}

static const io::FormatDescription *getDescription(const core::String &ext, uint32_t magic) {
	for (const io::FormatDescription *desc = voxelLoad(); desc->valid(); ++desc) {
		if (!desc->matchesExtension(ext)) {
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
	if (ext.empty()) {
		Log::warn("Could not identify the format");
	} else {
		Log::warn("Could not find a supported format description for %s", ext.c_str());
	}
	return nullptr;
}

static core::SharedPtr<Format> getFormat(const io::FormatDescription *desc, uint32_t magic, bool load) {
	core::SharedPtr<Format> format;
	for (const core::String& ext : desc->exts) {
		// you only have to check one of the supported extensions here
		if (ext == "qb") {
			format = core::make_shared<QBFormat>();
		} else if (ext == "vox") {
			if (!load || magic == FourCC('V', 'O', 'X', ' ')) {
				format = core::make_shared<VoxFormat>();
			} else {
				format = core::make_shared<SLAB6VoxFormat>();
			}
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
		} else if (ext == "mca") {
			format = core::make_shared<MCRFormat>();
		} else if (ext == "dat") {
			format = core::make_shared<DatFormat>();
		} else if (ext == "sment") {
			format = core::make_shared<SMFormat>();
		} else if (ext == "vxm") {
			format = core::make_shared<VXMFormat>();
		} else if (ext == "vxr") {
			format = core::make_shared<VXRFormat>();
		} else if (ext == "vxc") {
			format = core::make_shared<VXCFormat>();
		} else if (ext == "vxt") {
			format = core::make_shared<VXTFormat>();
		} else if (ext == "vxl" && desc->name == "Tiberian Sun") {
			format = core::make_shared<VXLFormat>();
		} else if (ext == "vxl") {
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
		} else if (ext == "schematic") {
			format = core::make_shared<SchematicFormat>();
		} else if (ext == "gltf") {
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

image::ImagePtr loadScreenshot(const core::String &filename, io::SeekableReadStream &stream) {
	core_trace_scoped(LoadVolumeScreenshot);
	const uint32_t magic = loadMagic(stream);
	const core::String &fileext = core::string::extractExtension(filename);
	const io::FormatDescription *desc = getDescription(fileext, magic);
	if (desc == nullptr) {
		Log::warn("Format %s isn't supported", fileext.c_str());
		return image::ImagePtr();
	}
	if (!(desc->flags & VOX_FORMAT_FLAG_SCREENSHOT_EMBEDDED)) {
		Log::warn("Format %s doesn't have a screenshot embedded", desc->name.c_str());
		return image::ImagePtr();
	}
	const core::SharedPtr<Format> &f = getFormat(desc, magic, true);
	if (f) {
		stream.seek(0);
		return f->loadScreenshot(filename, stream);
	}
	Log::error("Failed to load model screenshot from file %s - unsupported file format for extension '%s'",
			   filename.c_str(), fileext.c_str());
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
		if (voxelformat::loadPalette(filename, stream, palette) <= 0) {
			Log::warn("Failed to load palette from %s", filename.c_str());
			return false;
		}
		return true;
	}
	Log::warn("Given file is not supported as palette source: %s", filename.c_str());
	return false;
}

size_t loadPalette(const core::String &filename, io::SeekableReadStream &stream, voxel::Palette &palette) {
	core_trace_scoped(LoadVolumePalette);
	const uint32_t magic = loadMagic(stream);
	const core::String &fileext = core::string::extractExtension(filename);
	const io::FormatDescription *desc = getDescription(fileext, magic);
	if (desc == nullptr) {
		Log::warn("Format %s isn't supported", fileext.c_str());
		return 0;
	}
	if (!(desc->flags & VOX_FORMAT_FLAG_PALETTE_EMBEDDED)) {
		Log::warn("Format %s doesn't have a palette embedded", desc->name.c_str());
		return 0;
	}
	const core::SharedPtr<Format> &f = getFormat(desc, magic, true);
	if (f) {
		stream.seek(0);
		const size_t n = f->loadPalette(filename, stream, palette);
		palette.markDirty();
		return n;
	}
	Log::error("Failed to load model palette from file %s - unsupported file format for extension '%s'",
			   filename.c_str(), fileext.c_str());
	return 0;
}

bool loadFormat(const core::String &filename, io::SeekableReadStream &stream, SceneGraph &newSceneGraph) {
	core_trace_scoped(LoadVolumeFormat);
	const uint32_t magic = loadMagic(stream);
	const core::String &fileext = core::string::extractExtension(filename);
	const io::FormatDescription *desc = getDescription(fileext, magic);
	if (desc == nullptr) {
		return false;
	}
	const core::SharedPtr<Format> &f = getFormat(desc, magic, true);
	if (f) {
		if (!f->load(filename, stream, newSceneGraph)) {
			Log::error("Error while loading %s", filename.c_str());
			newSceneGraph.clear();
		}
	} else {
		Log::error("Failed to load model file %s - unsupported file format for extension '%s'", filename.c_str(),
				  fileext.c_str());
		return false;
	}
	if (newSceneGraph.empty()) {
		Log::error("Failed to load model file %s. Scene graph doesn't contain models.", filename.c_str());
		return false;
	}
	// newSceneGraph.node(newSceneGraph.root().id()).setProperty("Type", desc->name);
	Log::info("Load model file %s with %i layers", filename.c_str(), (int)newSceneGraph.size());
	return true;
}

bool isMeshFormat(const io::FormatDescription &desc) {
	return desc.flags & VOX_FORMAT_FLAG_MESH;
}

bool isMeshFormat(const core::String &filename) {
	const core::String &ext = core::string::extractExtension(filename);
	for (const io::FormatDescription *desc = voxelformat::voxelSave(); desc->valid(); ++desc) {
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

bool saveFormat(SceneGraph &sceneGraph, const core::String &filename, io::SeekableWriteStream &stream, ThumbnailCreator thumbnailCreator) {
	if (sceneGraph.empty()) {
		Log::error("Failed to save model file %s - no volumes given", filename.c_str());
		return false;
	}
	const core::String &type = sceneGraph.root().property("Type");
	if (!type.empty()) {
		Log::debug("Save '%s' file to '%s'", type.c_str(), filename.c_str());
	}
	const core::String &ext = core::string::extractExtension(filename);
	for (const io::FormatDescription *desc = voxelformat::voxelSave(); desc->valid(); ++desc) {
		if (desc->matchesExtension(ext) /*&& (type.empty() || type == desc->name)*/) {
			core::SharedPtr<Format> f = getFormat(desc, 0u, false);
			if (f) {
				if (f->save(sceneGraph, filename, stream, thumbnailCreator)) {
					Log::debug("Saved file for format '%s' (ext: '%s')", desc->name.c_str(), ext.c_str());
					return true;
				}
				Log::error("Failed to save %s file", desc->name.c_str());
				return false;
			}
		}
	}
	Log::warn("Failed to save file with unknown type: %s - saving as qb instead", ext.c_str());
	QBFormat qbFormat;
	stream.seek(0);
	return qbFormat.save(sceneGraph, filename, stream, thumbnailCreator);
}

bool saveFormat(const io::FilePtr &filePtr, SceneGraph &sceneGraph, ThumbnailCreator thumbnailCreator) {
	if (!filePtr->validHandle()) {
		Log::error("Failed to save model - no valid file given");
		return false;
	}

	io::FileStream stream(filePtr);
	return saveFormat(sceneGraph, filePtr->name(), stream, thumbnailCreator);
}

}
