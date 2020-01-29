/**
 * @file
 */

#include "CommandCompleter.h"
#include "core/App.h"
#include "core/StringUtil.h"
#include "core/io/Filesystem.h"

namespace core {

int complete(core::String dir, const core::String& match, std::vector<std::string>& matches, const char* pattern) {
	const std::string_view additionalDir = core::string::extractPath(match.c_str());
	dir += additionalDir;
	core::String currentMatch;
	if (dir.empty()) {
		dir = ".";
		currentMatch = match;
	} else {
		currentMatch = match.substr(additionalDir.length());
	}

	std::vector<io::Filesystem::DirEntry> entries;
	const core::String filter = match + pattern;
	const std::string_view& filterName = core::string::extractFilenameWithExtension(filter.c_str());
	const std::string_view& filterPath = core::string::extractPath(filter.c_str());
	const core::String filterPathStr(filterPath);
	const io::FilesystemPtr& fs = io::filesystem();
	fs->list(dir, entries, currentMatch + "*");
	int i = 0;
	for (const io::Filesystem::DirEntry& entry : entries) {
		if (entry.type == io::Filesystem::DirEntry::Type::dir) {
			core::String name = filterPathStr.empty() ? entry.name : filterPathStr + entry.name;
			name.append("/");
			matches.push_back(name);
			++i;
		}
	}
	entries.clear();
	fs->list(dir, entries, std::string(filterName));
	for (const io::Filesystem::DirEntry& entry : entries) {
		if (entry.type == io::Filesystem::DirEntry::Type::file) {
			matches.push_back(filterPathStr.empty() ? entry.name : filterPathStr + entry.name);
			++i;
		}
	}
	return i;
}

}
