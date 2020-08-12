/**
 * @file
 */

#include "CommandCompleter.h"
#include "core/App.h"
#include "core/StringUtil.h"
#include "core/io/Filesystem.h"

namespace core {

int complete(core::String dir, const core::String& match, core::DynamicArray<core::String>& matches, const char* pattern) {
	const core::String additionalDir = core::string::extractPath(match.c_str());
	dir += additionalDir;
	core::String currentMatch;
	if (dir.empty()) {
		dir = ".";
		currentMatch = match;
	} else {
		currentMatch = match.substr(additionalDir.size());
	}

	core::DynamicArray<io::Filesystem::DirEntry> entries;
	const core::String filter = match + pattern;
	const core::String& filterName = core::string::extractFilenameWithExtension(filter.c_str());
	const core::String& filterPath = core::string::extractPath(filter.c_str());
	const io::FilesystemPtr& fs = io::filesystem();
	fs->list(dir, entries, currentMatch + "*");
	int i = 0;
	for (const io::Filesystem::DirEntry& entry : entries) {
		if (entry.type == io::Filesystem::DirEntry::Type::dir) {
			core::String name = filterPath.empty() ? entry.name : filterPath + entry.name;
			name.append("/");
			matches.push_back(name);
			++i;
		}
	}
	entries.clear();
	fs->list(dir, entries, core::String(filterName));
	for (const io::Filesystem::DirEntry& entry : entries) {
		if (entry.type == io::Filesystem::DirEntry::Type::file) {
			matches.push_back(filterPath.empty() ? entry.name : filterPath + entry.name);
			++i;
		}
	}
	return i;
}

}
