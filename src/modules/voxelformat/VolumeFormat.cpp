/**
 * @file
 */

#include "VolumeFormat.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/SharedPtr.h"
#include "core/StringUtil.h"
#include "core/Trace.h"
#include "io/FileStream.h"
#include "io/FormatDescription.h"
#include "io/Stream.h"
#include "video/Texture.h"
#include "voxelformat/PLYFormat.h"
#include "voxelformat/VoxFormat.h"
#include "voxelformat/QBTFormat.h"
#include "voxelformat/QBFormat.h"
#include "voxelformat/QBCLFormat.h"
#include "voxelformat/QEFFormat.h"
#include "voxelformat/VXMFormat.h"
#include "voxelformat/VXRFormat.h"
#include "voxelformat/VXLFormat.h"
#include "voxelformat/CubFormat.h"
#include "voxelformat/BinVoxFormat.h"
#include "voxelformat/KVXFormat.h"
#include "voxelformat/KV6Format.h"
#include "voxelformat/AoSVXLFormat.h"
#include "voxelformat/CSMFormat.h"
#include "voxelformat/OBJFormat.h"

namespace voxelformat {

#define VOX_FORMAT_FLAG_SCREENSHOT_EMBEDDED (1 << 0)
#define VOX_FORMAT_FLAG_PALETTE_EMBEDDED (1 << 1)

// this is the list of supported voxel volume formats that are have importers implemented
const io::FormatDescription SUPPORTED_VOXEL_FORMATS_LOAD[] = {
	{"Qubicle Binary", "qb", nullptr, 0u},
	{"MagicaVoxel", "vox", [] (uint32_t magic) {return magic == FourCC('V','O','X',' ');}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
	{"Qubicle Binary Tree", "qbt", [] (uint32_t magic) {return magic == FourCC('Q','B',' ','2');}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
	{"Qubicle Project", "qbcl", [] (uint32_t magic) {return magic == FourCC('Q','B','C','L');}, VOX_FORMAT_FLAG_SCREENSHOT_EMBEDDED},
	{"Sandbox VoxEdit", "vxm", [] (uint32_t magic) {return magic == FourCC('V','X','M','A')
			|| magic == FourCC('V','X','M','B') || magic == FourCC('V','X','M','C')
			|| magic == FourCC('V','X','M','9') || magic == FourCC('V','X','M','8')
			|| magic == FourCC('V','X','M','7') || magic == FourCC('V','X','M','6')
			|| magic == FourCC('V','X','M','5') || magic == FourCC('V','X','M','4');}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
	{"Sandbox VoxEdit", "vxr", [] (uint32_t magic) {return magic == FourCC('V','X','R','7') || magic == FourCC('V','X','R','6')
			|| magic == FourCC('V','X','R','5') || magic == FourCC('V','X','R','4')
			|| magic == FourCC('V','X','R','3') || magic == FourCC('V','X','R','2')
			|| magic == FourCC('V','X','R','1');}, 0u},
	{"BinVox", "binvox", [] (uint32_t magic) {return magic == FourCC('#','b','i','n');}, 0u},
	{"CubeWorld", "cub", nullptr, 0u},
	{"Build engine", "kvx", nullptr, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
	{"Ace of Spades", "kv6", [] (uint32_t magic) {return magic == FourCC('K','v','x','l');}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
	{"Tiberian Sun", "vxl", [] (uint32_t magic) {return magic == FourCC('V','o','x','e');}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
	{"AceOfSpades", "vxl", nullptr, 0u},
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
	//{"Qubicle Project", "qbcl", nullptr, 0u},
	{"Sandbox VoxEdit", "vxm", nullptr, 0u},
	{"BinVox", "binvox", nullptr, 0u},
	{"CubeWorld", "cub", nullptr, 0u},
	//{"Build engine", "kvx", nullptr, 0u},
	{"Tiberian Sun", "vxl", nullptr, 0u},
	{"Qubicle Exchange", "qef", nullptr, 0u},
	{"WaveFront OBJ", "obj", nullptr, 0u},
	{"Polygon File Format", "ply", nullptr, 0u},
	{nullptr, nullptr, nullptr, 0u}
};

static uint32_t loadMagic(io::ReadStream& stream) {
	uint32_t magicWord = 0u;
	stream.readInt(magicWord);
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
	Log::warn("Could not find a supported format description for %s", ext.c_str());
	return nullptr;
}

static core::SharedPtr<voxel::Format> getFormat(const io::FormatDescription *desc, uint32_t magic) {
	core::SharedPtr<voxel::Format> format;
	const core::String &ext = desc->ext;
	if (ext == "qb") {
		format = core::make_shared<voxel::QBFormat>();
	} else if (ext == "vox" && magic != FourCC('V','O','X',' ')) {
		// TODO: old vox format support
	} else if (ext == "vox" || magic == FourCC('V','O','X',' ')) {
		format = core::make_shared<voxel::VoxFormat>();
	} else if (ext == "qbt" || magic == FourCC('Q','B',' ','2')) {
		format = core::make_shared<voxel::QBTFormat>();
	} else if (ext == "kvx") {
		format = core::make_shared<voxel::KVXFormat>();
	} else if (ext == "kv6") {
		format = core::make_shared<voxel::KV6Format>();
	} else if (ext == "cub") {
		format = core::make_shared<voxel::CubFormat>();
	} else if (ext == "vxm") {
		format = core::make_shared<voxel::VXMFormat>();
	} else if (ext == "vxr") {
		format = core::make_shared<voxel::VXRFormat>();
	} else if (ext == "vxl" && !strcmp(desc->name, "Tiberian Sun")) {
		format = core::make_shared<voxel::VXLFormat>();
	} else if (ext == "vxl") {
		format = core::make_shared<voxel::AoSVXLFormat>();
	} else if (ext == "csm" || ext == "nvm") {
		format = core::make_shared<voxel::CSMFormat>();
	} else if (ext == "binvox") {
		format = core::make_shared<voxel::BinVoxFormat>();
	} else if (ext == "qef") {
		format = core::make_shared<voxel::QEFFormat>();
	} else if (ext == "qbcl") {
		format = core::make_shared<voxel::QBCLFormat>();
	}
	return format;
}

image::ImagePtr loadVolumeScreenshot(const core::String &fileName, io::SeekableReadStream& stream) {
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
	const core::SharedPtr<voxel::Format> &f = getFormat(desc, magic);
	if (f) {
		stream.seek(0);
		return f->loadScreenshot(fileName, stream);
	}
	Log::error("Failed to load model screenshot from file %s - unsupported file format for extension '%s'",
			fileName.c_str(), fileext.c_str());
	return image::ImagePtr();
}

size_t loadVolumePalette(const core::String &fileName, io::SeekableReadStream& stream, core::Array<uint32_t, 256> &palette) {
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
	const core::SharedPtr<voxel::Format> &f = getFormat(desc, magic);
	if (f) {
		stream.seek(0);
		return f->loadPalette(fileName, stream, palette);
	}
	Log::error("Failed to load model palette from file %s - unsupported file format for extension '%s'",
			fileName.c_str(), fileext.c_str());
	return 0;
}

bool loadVolumeFormat(const core::String &fileName, io::SeekableReadStream& stream, voxel::VoxelVolumes& newVolumes) {
	core_trace_scoped(LoadVolumeFormat);
	const uint32_t magic = loadMagic(stream);
	const core::String& fileext = core::string::extractExtension(fileName);
	const io::FormatDescription *desc = getDescription(fileext, magic);
	if (desc == nullptr) {
		Log::warn("Format %s isn't supported", fileext.c_str());
		return false;
	}
	const core::SharedPtr<voxel::Format> &f = getFormat(desc, magic);
	if (f) {
		stream.seek(0);
		if (!f->loadGroups(fileName, stream, newVolumes)) {
			voxelformat::clearVolumes(newVolumes);
		}
	} else {
		Log::error("Failed to load model file %s - unsupported file format for extension '%s'",
				fileName.c_str(), fileext.c_str());
		return false;
	}
	if (newVolumes.empty()) {
		Log::error("Failed to load model file %s. Broken file.", fileName.c_str());
		return false;
	}
	Log::info("Load model file %s with %i layers", fileName.c_str(), (int)newVolumes.size());
	return true;
}

bool saveFormat(const io::FilePtr& filePtr, voxel::VoxelVolumes& volumes) {
	if (isMeshFormat(filePtr->name())) {
		return saveMeshFormat(filePtr, volumes);
	}
	return saveVolumeFormat(filePtr, volumes);
}

bool saveVolumeFormat(const io::FilePtr& filePtr, voxel::VoxelVolumes& volumes) {
	if (volumes.empty()) {
		Log::error("Failed to save model file %s - no volumes given", filePtr->name().c_str());
		return false;
	}

	io::FileStream stream(filePtr.get());
	const core::String& ext = filePtr->extension();
	const io::FormatDescription *desc = getDescription(ext, 0);
	if (desc != nullptr) {
		core::SharedPtr<voxel::Format> f = getFormat(desc, 0u);
		if (f && f->saveGroups(volumes, filePtr->fileName(), stream)) {
			return true;
		}
	}
	Log::warn("Failed to save file with unknown type: %s - saving as qb instead", ext.c_str());
	voxel::QBFormat qbFormat;
	return qbFormat.saveGroups(volumes, filePtr->fileName(), stream);
}

bool saveMeshFormat(const io::FilePtr& filePtr, voxel::VoxelVolumes& volumes) {
	if (volumes.empty()) {
		Log::error("Failed to save model file %s - no volumes given", filePtr->name().c_str());
		return false;
	}

	io::FileStream stream(filePtr.get());
	const core::String& ext = filePtr->extension();
	if (ext == "obj") {
		voxel::OBJFormat f;
		return f.saveGroups(volumes, filePtr->fileName(), stream);
	} else if (ext == "ply") {
		voxel::PLYFormat f;
		return f.saveGroups(volumes, filePtr->fileName(), stream);
	}
	Log::error("Failed to save model file %s - unknown extension '%s' given", filePtr->name().c_str(), ext.c_str());
	return false;
}

bool isMeshFormat(const core::String& filename) {
	const core::String& ext = core::string::extractExtension(filename);
	if (ext == "obj" || ext == "ply") {
		return true;
	}
	return false;
}

void clearVolumes(voxel::VoxelVolumes& volumes) {
	for (auto& v : volumes) {
		delete v.volume;
	}
	volumes.volumes.clear();
}

}
