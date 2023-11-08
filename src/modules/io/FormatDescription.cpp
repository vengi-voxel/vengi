/**
 * @file
 */

#include "FormatDescription.h"
#include "core/Algorithm.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/String.h"
#include "core/StringUtil.h"

namespace io {

namespace format {

const FormatDescription *images() {
	// clang-format: off
	static FormatDescription desc[] = {
		{"Portable Network Graphics", {"png"}, nullptr, 0u},
		{"JPEG", {"jpeg", "jpg"}, nullptr, 0u},
		{"Targa image file", {"tga"}, nullptr, 0u},
		{"DDS", {"dds"}, nullptr, 0u},
		{"PKM", {"pkm"}, nullptr, 0u},
		{"PVR", {"pvr"}, nullptr, 0u},
		{"Bitmap", {"bmp"}, nullptr, 0u},
		{"Photoshop", {"psd"}, nullptr, 0u},
		{"Graphics Interchange Format", {"gif"}, nullptr, 0u},
		{"Radiance rgbE", {"hdr"}, nullptr, 0u},
		{"Softimage PIC", {"pic"}, nullptr, 0u},
		{"Portable Anymap", {"pnm"}, nullptr, 0u},
		{"", {}, nullptr, 0u}
	};
	// clang-format: on
	return desc;
}

const FormatDescription *fonts() {
	// clang-format: off
	static FormatDescription desc[] = {
		{"TrueType Font", {"ttf"}, nullptr, 0u},
		{"", {}, nullptr, 0u}
	};
	// clang-format: on
	return desc;
}

FormatDescription jascPalette() {
	return {"JASC Palette", {"pal"}, [](uint32_t magic) { return magic == FourCC('J', 'A', 'S', 'C'); }, 0u};
}

const FormatDescription *palettes() {
	// clang-format: off
	static FormatDescription desc[] = {
		{"Gimp Palette", {"gpl"}, nullptr, 0u},
		{"Qubicle Palette", {"qsm"}, nullptr, 0u},
		jascPalette(),
		{"RGB Palette", {"pal"}, nullptr, 0u},
		{"CSV Palette", {"csv"}, nullptr, 0u},
		{"Portable Network Graphics", {"png"}, [](uint32_t magic) { return magic == FourCC('\x89', 'P', 'N', 'G'); }, 0u},
		{"", {}, nullptr, 0u}
	};
	// clang-format: on
	return desc;
}

const FormatDescription *lua() {
	// clang-format: off
	static FormatDescription desc[] = {
		{"LUA script", {"lua"}, nullptr, 0},
		{"", {}, nullptr, 0u}
	};
	// clang-format: on
	return desc;
}

} // namespace format

core::String FormatDescription::mainExtension(bool includeDot) const {
	if (exts.empty()) {
		return "";
	}
	if (!includeDot) {
		return exts[0];
	}
	return "." + exts[0];
}

bool FormatDescription::matchesExtension(const core::String &fileExt) const {
	const core::String &lowerExt = fileExt.toLower();
	for (const core::String &ext : exts) {
		if (lowerExt == ext) {
			return true;
		}
	}
	return false;
}

bool FormatDescription::operator<(const FormatDescription &rhs) const {
	return name < rhs.name;
}

core::String FormatDescription::wildCard() const {
	core::String pattern;
	for (size_t i = 0; i < exts.size(); ++i) {
		if (i > 0) {
			pattern.append(",");
		}
		pattern += core::string::format("*.%s", exts[i].c_str());
	}
	return pattern;
}

void FileDescription::set(const core::String &s, const io::FormatDescription *f) {
	if (f != nullptr) {
		desc = *f;
	} else {
		desc = {};
	}
	name = s;
}

void FileDescription::clear() {
	name = "";
	desc = {};
}

bool isA(const core::String &file, const io::FormatDescription *desc) {
	const core::String &ext = core::string::extractExtension(file);
	for (; desc->valid(); ++desc) {
		if (desc->matchesExtension(ext)) {
			return true;
		}
	}
	return false;
}

bool isImage(const core::String &file) {
	return isA(file, io::format::images());
}

void createGroupPatterns(const FormatDescription *inputDesc, core::DynamicArray<io::FormatDescription> &groups) {
	core::DynamicArray<io::FormatDescription> descs;
	for (; inputDesc->valid(); ++inputDesc) {
		descs.emplace_back(*inputDesc);
	}
	core::sort(descs.begin(), descs.end(), core::Less<io::FormatDescription>());
	core::DynamicArray<io::FormatDescription> temp;
	core::String lastName;
	for (const io::FormatDescription &desc : descs) {
		core::String firstWord = desc.name;
		auto iter = firstWord.find_first_of(" ");
		if (iter != core::String::npos) {
			firstWord = firstWord.substr(0, iter);
		}
		if (lastName != firstWord) {
			if (temp.size() >= 2) {
				io::FormatDescriptionExtensions exts{};
				uint32_t flags = FORMAT_FLAG_GROUP;
				for (const io::FormatDescription &tmpDesc : temp) {
					for (const core::String &ext : tmpDesc.exts) {
						exts.push_back(ext);
					}
					flags |= tmpDesc.flags;
				}
				const io::FormatDescription val{lastName, exts, nullptr, flags};
				groups.push_back(val);
			}
			lastName = firstWord;
			temp.clear();
		}
		temp.push_back(desc);
	}
	if (temp.size() >= 2) {
		io::FormatDescriptionExtensions exts{};
		uint32_t flags = FORMAT_FLAG_GROUP;
		for (const io::FormatDescription &tmpDesc : temp) {
			for (const core::String &ext : tmpDesc.exts) {
				exts.push_back(ext);
			}
			flags |= tmpDesc.flags;
		}
		const io::FormatDescription val{lastName, exts, nullptr, flags};
		groups.push_back(val);
	}
}

core::String convertToFilePattern(const FormatDescription &desc) {
	core::String pattern;
	bool showName = false;
	if (!desc.name.empty()) {
		pattern += desc.name;
		if (!desc.exts.empty()) {
			pattern.append(" (");
			showName = true;
		}
	}
	pattern += desc.wildCard();
	if (showName) {
		pattern.append(")");
	}
	return pattern;
}

core::String convertToAllFilePattern(const FormatDescription *desc) {
	core::String pattern;
	int j = 0;
	while (desc->valid()) {
		if (j > 0) {
			pattern.append(",");
		}
		pattern += desc->wildCard();
		++desc;
		++j;
	}
	return j > 1 ? pattern : "";
}

uint32_t loadMagic(io::SeekableReadStream &stream) {
	uint32_t magicWord = 0u;
	stream.peekUInt32(magicWord);
	return magicWord;
}

const io::FormatDescription *getDescription(const core::String &filename, uint32_t magic,
											const io::FormatDescription *descriptions) {
	const core::String ext = core::string::extractExtension(filename);
	const core::String extFull = core::string::extractAllExtensions(filename);
	for (const io::FormatDescription *desc = descriptions; desc->valid(); ++desc) {
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
		for (const io::FormatDescription *desc = descriptions; desc->valid(); ++desc) {
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

const io::FormatDescription *getDescription(const io::FileDescription &fileDesc, uint32_t magic,
											const io::FormatDescription *descriptions) {
	if (fileDesc.desc.valid()) {
		return &fileDesc.desc;
	}
	const core::String filename = fileDesc.name;
	return io::getDescription(filename, magic, descriptions);
}

} // namespace io
