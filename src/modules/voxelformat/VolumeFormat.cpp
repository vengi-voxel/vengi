/**
 * @file
 */

#include "VolumeFormat.h"
#include "app/App.h"
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
#include "voxelformat/MCRFormat.h"
#include "voxelformat/PLYFormat.h"
#include "voxelformat/FBXFormat.h"
#include "voxelformat/SproxelFormat.h"
#include "voxelformat/VXTFormat.h"
#include "voxelformat/VoxFormat.h"
#include "voxelformat/QBTFormat.h"
#include "voxelformat/QBFormat.h"
#include "voxelformat/QBCLFormat.h"
#include "voxelformat/QEFFormat.h"
#include "voxelformat/VXCFormat.h"
#include "voxelformat/VXMFormat.h"
#include "voxelformat/VXRFormat.h"
#include "voxelformat/VXLFormat.h"
#include "voxelformat/CubFormat.h"
#include "voxelformat/GoxFormat.h"
#include "voxelformat/STLFormat.h"
#include "voxelformat/BinVoxFormat.h"
#include "voxelformat/KVXFormat.h"
#include "voxelformat/KV6Format.h"
#include "voxelformat/AoSVXLFormat.h"
#include "voxelformat/CSMFormat.h"
#include "voxelformat/OBJFormat.h"
#include "voxelformat/VoxOldFormat.h"
#include "voxelformat/GLTFFormat.h"
#include "voxelformat/SchematicFormat.h"

namespace voxelformat {

// this is the list of supported voxel volume formats that are have importers implemented
const io::FormatDescription SUPPORTED_VOXEL_FORMATS_LOAD[] = {
	{"Qubicle Binary", "qb", nullptr, 0u},
	{"MagicaVoxel", "vox", [] (uint32_t magic) {return magic == FourCC('V','O','X',' ');}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
	{"Qubicle Binary Tree", "qbt", [] (uint32_t magic) {return magic == FourCC('Q','B',' ','2');}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
	{"Qubicle Project", "qbcl", [] (uint32_t magic) {return magic == FourCC('Q','B','C','L');}, VOX_FORMAT_FLAG_SCREENSHOT_EMBEDDED},
	{"Sandbox VoxEdit Tilemap", "vxt", [] (uint32_t magic) {return magic == FourCC('V','X','T','1');}, 0u},
	{"Sandbox VoxEdit Collection", "vxc", nullptr, 0u},
	{"Sandbox VoxEdit Model", "vxm", [] (uint32_t magic) {return magic == FourCC('V','X','M','A')
			|| magic == FourCC('V','X','M','B') || magic == FourCC('V','X','M','C')
			|| magic == FourCC('V','X','M','9') || magic == FourCC('V','X','M','8')
			|| magic == FourCC('V','X','M','7') || magic == FourCC('V','X','M','6')
			|| magic == FourCC('V','X','M','5') || magic == FourCC('V','X','M','4');}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
	{"Sandbox VoxEdit Hierarchy", "vxr", [] (uint32_t magic) {
		return magic == FourCC('V','X','R','9') || magic == FourCC('V','X','R','8')
			|| magic == FourCC('V','X','R','7') || magic == FourCC('V','X','R','6')
			|| magic == FourCC('V','X','R','5') || magic == FourCC('V','X','R','4')
			|| magic == FourCC('V','X','R','3') || magic == FourCC('V','X','R','2')
			|| magic == FourCC('V','X','R','1');}, 0u},
	{"BinVox", "binvox", [] (uint32_t magic) {return magic == FourCC('#','b','i','n');}, 0u},
	{"Goxel", "gox", [] (uint32_t magic) {return magic == FourCC('G','O','X',' ');}, VOX_FORMAT_FLAG_SCREENSHOT_EMBEDDED},
	{"CubeWorld", "cub", nullptr, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
	{"Minecraft region", "mca", nullptr, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
	{"Minecraft schematic", "schematic", nullptr, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
	{"Sproxel csv", "csv", nullptr, 0u},
	{"Wavefront Object", "obj", nullptr, VOX_FORMAT_FLAG_MESH},
	{"GL Transmission Format", "gltf", nullptr, VOX_FORMAT_FLAG_MESH},
	{"GL Transmission Binary Format", "glb", nullptr, VOX_FORMAT_FLAG_MESH},
	{"Standard Triangle Language", "stl", nullptr, VOX_FORMAT_FLAG_MESH},
	{"Build engine", "kvx", nullptr, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
	{"Ace of Spades", "kv6", [] (uint32_t magic) {return magic == FourCC('K','v','x','l');}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
	{"Tiberian Sun", "vxl", [] (uint32_t magic) {return magic == FourCC('V','o','x','e');}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
	{"AceOfSpades", "vxl", nullptr, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
	{"Qubicle Exchange", "qef", [] (uint32_t magic) {return magic == FourCC('Q','u','b','i');}, 0u},
	{"Chronovox", "csm", [] (uint32_t magic) {return magic == FourCC('.','C','S','M');}, 0u},
	{"Nicks Voxel Model", "nvm", [] (uint32_t magic) {return magic == FourCC('.','N','V','M');}, 0u},
	{nullptr, nullptr, nullptr, 0u}
};
// this is the list of internal formats that are supported engine-wide (the format we save our own models in)
const char *SUPPORTED_VOXEL_FORMATS_LOAD_LIST[] = { "qb", "vox", nullptr };
// this is the list of supported voxel or mesh formats that have exporters implemented
const io::FormatDescription SUPPORTED_VOXEL_FORMATS_SAVE[] = {
	{"Qubicle Binary", "qb", nullptr, 0u},
	{"MagicaVoxel", "vox", nullptr, 0u},
	{"Qubicle Binary Tree", "qbt", nullptr, 0u},
	{"Qubicle Project", "qbcl", nullptr, 0u},
	{"Sandbox VoxEdit Model", "vxm", nullptr, 0u},
	{"Sandbox VoxEdit Hierarchy", "vxr", nullptr, 0u},
	{"BinVox", "binvox", nullptr, 0u},
	{"Goxel", "gox", nullptr, 0u},
	{"Sproxel csv", "csv", nullptr, 0u},
	{"CubeWorld", "cub", nullptr, 0u},
	//{"Build engine", "kvx", nullptr, 0u},
	{"Tiberian Sun", "vxl", nullptr, 0u},
	{"Qubicle Exchange", "qef", nullptr, 0u},
	{"AceOfSpades", "vxl", nullptr, 0u}, // TODO: handle duplicate extension
	{"Minecraft schematic", "schematic", nullptr, 0u},
	{"Wavefront Object", "obj", nullptr, VOX_FORMAT_FLAG_MESH},
	{"Polygon File Format", "ply", nullptr, VOX_FORMAT_FLAG_MESH},
	{"FBX Ascii", "fbx", nullptr, VOX_FORMAT_FLAG_MESH},
	{"Standard Triangle Language", "stl", nullptr, VOX_FORMAT_FLAG_MESH},
	{"GL Transmission Format", "gltf", nullptr, VOX_FORMAT_FLAG_MESH},
	{"GL Transmission Binary Format", "glb", nullptr, VOX_FORMAT_FLAG_MESH},
	{nullptr, nullptr, nullptr, 0u}
};

static uint32_t loadMagic(io::ReadStream& stream) {
	uint32_t magicWord = 0u;
	stream.readUInt32(magicWord);
	return magicWord;
}

static const io::FormatDescription *getDescription(const core::String &ext, uint32_t magic) {
	for (const io::FormatDescription *desc = SUPPORTED_VOXEL_FORMATS_LOAD; desc->ext != nullptr; ++desc) {
		if (ext != desc->ext) {
			continue;
		}
		if (magic > 0 && desc->isA && !desc->isA(magic)) {
			continue;
		}
		return desc;
	}
	if (magic > 0) {
		// search again - but this time only the magic bytes...
		for (const io::FormatDescription *desc = SUPPORTED_VOXEL_FORMATS_LOAD; desc->ext != nullptr; ++desc) {
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

static core::SharedPtr<Format> getFormat(const io::FormatDescription *desc, uint32_t magic) {
	core::SharedPtr<Format> format;
	const core::String &ext = desc->ext;
	if (ext == "qb") {
		format = core::make_shared<QBFormat>();
	} else if (ext == "vox" && (magic != 0u && magic != FourCC('V','O','X',' '))) {
		format = core::make_shared<VoxOldFormat>();
	} else if (ext == "vox" || magic == FourCC('V','O','X',' ')) {
		format = core::make_shared<VoxFormat>();
	} else if (ext == "qbt" || magic == FourCC('Q','B',' ','2')) {
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
	} else if (ext == "vxm") {
		format = core::make_shared<VXMFormat>();
	} else if (ext == "vxr") {
		format = core::make_shared<VXRFormat>();
	} else if (ext == "vxc") {
		format = core::make_shared<VXCFormat>();
	} else if (ext == "vxt") {
		format = core::make_shared<VXTFormat>();
	} else if (ext == "vxl" && !strcmp(desc->name, "Tiberian Sun")) {
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
	} else if (ext == "ply") {
		format = core::make_shared<PLYFormat>();
	} else if (ext == "fbx") {
		format = core::make_shared<FBXFormat>();
	} else if (ext == "schematic") {
		format = core::make_shared<SchematicFormat>();
	} else if (ext == "gltf" || ext == "glb") {
		format = core::make_shared<GLTFFormat>();
	}
	return format;
}

image::ImagePtr loadScreenshot(const core::String &fileName, io::SeekableReadStream& stream) {
	core_trace_scoped(LoadVolumeScreenshot);
	const uint32_t magic = loadMagic(stream);
	const core::String& fileext = core::string::extractExtension(fileName);
	const io::FormatDescription *desc = getDescription(fileext, magic);
	if (desc == nullptr) {
		Log::warn("Format %s isn't supported", fileext.c_str());
		return image::ImagePtr();
	}
	if (!(desc->flags & VOX_FORMAT_FLAG_SCREENSHOT_EMBEDDED)) {
		Log::warn("Format %s doesn't have a screenshot embedded", desc->name);
		return image::ImagePtr();
	}
	const core::SharedPtr<Format> &f = getFormat(desc, magic);
	if (f) {
		stream.seek(0);
		return f->loadScreenshot(fileName, stream);
	}
	Log::error("Failed to load model screenshot from file %s - unsupported file format for extension '%s'",
			fileName.c_str(), fileext.c_str());
	return image::ImagePtr();
}

bool importPalette(const core::String& file, voxel::Palette &palette) {
	const core::String& ext = core::string::extractExtension(file);
	core::String paletteName(core::string::extractFilename(file.c_str()));
	core::string::replaceAllChars(paletteName, ' ', '_');
	paletteName = paletteName.toLower();
	bool paletteLoaded = false;
	for (const io::FormatDescription* desc = io::format::images(); desc->name != nullptr; ++desc) {
		if (ext == desc->ext) {
			const image::ImagePtr &img = image::loadImage(file, false);
			if (!img->isLoaded()) {
				Log::warn("Failed to load image %s", file.c_str());
				break;
			}
			if (!voxel::Palette::createPalette(img, palette)) {
				Log::warn("Failed to create palette for image %s", file.c_str());
				return false;
			}
			paletteLoaded = true;
			break;
		}
	}
	const io::FilesystemPtr& fs = io::filesystem();
	if (!paletteLoaded) {
		const io::FilePtr& palFile = fs->open(file);
		if (!palFile->validHandle()) {
			Log::warn("Failed to load palette from %s", file.c_str());
			return false;
		}
		io::FileStream stream(palFile);
		if (voxelformat::loadPalette(file, stream, palette) <= 0) {
			Log::warn("Failed to load palette from %s", file.c_str());
			return false;
		}
		paletteLoaded = true;
	}
	return paletteLoaded;
}

size_t loadPalette(const core::String &fileName, io::SeekableReadStream& stream, voxel::Palette &palette) {
	core_trace_scoped(LoadVolumePalette);
	const uint32_t magic = loadMagic(stream);
	const core::String& fileext = core::string::extractExtension(fileName);
	const io::FormatDescription *desc = getDescription(fileext, magic);
	if (desc == nullptr) {
		Log::warn("Format %s isn't supported", fileext.c_str());
		return 0;
	}
	if (!(desc->flags & VOX_FORMAT_FLAG_PALETTE_EMBEDDED)) {
		Log::warn("Format %s doesn't have a palette embedded", desc->name);
		return 0;
	}
	const core::SharedPtr<Format> &f = getFormat(desc, magic);
	if (f) {
		stream.seek(0);
		const size_t n = f->loadPalette(fileName, stream, palette);
		palette.markDirty();
		return n;
	}
	Log::error("Failed to load model palette from file %s - unsupported file format for extension '%s'",
			fileName.c_str(), fileext.c_str());
	return 0;
}

bool loadFormat(const core::String &fileName, io::SeekableReadStream& stream, SceneGraph& newSceneGraph) {
	core_trace_scoped(LoadVolumeFormat);
	const uint32_t magic = loadMagic(stream);
	const core::String& fileext = core::string::extractExtension(fileName);
	const io::FormatDescription *desc = getDescription(fileext, magic);
	if (desc == nullptr) {
		return false;
	}
	const core::SharedPtr<Format> &f = getFormat(desc, magic);
	if (f) {
		stream.seek(0);
		if (!f->loadGroups(fileName, stream, newSceneGraph)) {
			newSceneGraph.clear();
		}
	} else {
		Log::warn("Failed to load model file %s - unsupported file format for extension '%s'",
				fileName.c_str(), fileext.c_str());
		return false;
	}
	if (newSceneGraph.empty()) {
		Log::error("Failed to load model file %s. Scene graph doesn't contain models.", fileName.c_str());
		return false;
	}
	//newSceneGraph.node(newSceneGraph.root().id()).setProperty("Type", desc->name);
	Log::info("Load model file %s with %i layers", fileName.c_str(), (int)newSceneGraph.size());
	return true;
}

bool isMeshFormat(const core::String& filename) {
	const core::String& ext = core::string::extractExtension(filename);
	for (const io::FormatDescription *desc = voxelformat::SUPPORTED_VOXEL_FORMATS_SAVE; desc->ext != nullptr; ++desc) {
		if (ext == desc->ext && (desc->flags & VOX_FORMAT_FLAG_MESH) != 0u) {
			return true;
		}
	}

	return false;
}

bool saveFormat(const io::FilePtr& filePtr, SceneGraph& sceneGraph) {
	if (sceneGraph.empty()) {
		Log::error("Failed to save model file %s - no volumes given", filePtr->name().c_str());
		return false;
	}

	if (!filePtr->validHandle()) {
		Log::error("Failed to save model - no valid file given");
		return false;
	}

	const core::String& type = sceneGraph.root().property("Type");
	if (!type.empty()) {
		Log::debug("Save '%s' file to '%s'", type.c_str(), filePtr->name().c_str());
	}
	io::FileStream stream(filePtr);
	const core::String& ext = filePtr->extension();
	for (const io::FormatDescription *desc = voxelformat::SUPPORTED_VOXEL_FORMATS_SAVE; desc->ext != nullptr; ++desc) {
		if (ext == desc->ext /*&& (type.empty() || type == desc->name)*/) {
			core::SharedPtr<Format> f = getFormat(desc, 0u);
			if (f && f->saveGroups(sceneGraph, filePtr->name(), stream)) {
				Log::debug("Saved file for format '%s' (ext: '%s')", desc->name, desc->ext);
				return true;
			}
		}
	}
	if (isMeshFormat(filePtr->name())) {
		Log::error("Failed to save model file %s - unknown extension '%s' given", filePtr->name().c_str(), ext.c_str());
		return false;
	}
	Log::warn("Failed to save file with unknown type: %s - saving as qb instead", ext.c_str());
	QBFormat qbFormat;
	return qbFormat.saveGroups(sceneGraph, filePtr->name(), stream);
}

}
