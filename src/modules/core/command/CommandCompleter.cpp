/**
 * @file
 */

#include "CommandCompleter.h"
#include "core/App.h"
#include "core/String.h"
#include "core/io/Filesystem.h"

namespace core {

int complete(std::string dir, const std::string& match, std::vector<std::string>& matches, const char* pattern) {
	const std::string_view additionalDir = core::string::extractPath(match.c_str());
	dir += additionalDir;
	std::string currentMatch;
	if (dir.empty()) {
		dir = ".";
		currentMatch = match;
	} else {
		currentMatch = match.substr(additionalDir.length());
	}

	std::vector<io::Filesystem::DirEntry> entries;
	const std::string filter = match + pattern;
	const std::string_view& filterName = core::string::extractFilenameWithExtension(filter.c_str());
	const std::string_view& filterPath = core::string::extractPath(filter.c_str());
	const std::string filterPathStr(filterPath);
	const io::FilesystemPtr& fs = core::App::getInstance()->filesystem();
	fs->list(dir, entries, currentMatch + "*");
	int i = 0;
	for (const io::Filesystem::DirEntry& entry : entries) {
		if (entry.type == io::Filesystem::DirEntry::Type::dir) {
			std::string name = filterPathStr.empty() ? entry.name : filterPathStr + entry.name;
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
