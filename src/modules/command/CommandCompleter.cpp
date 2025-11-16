/**
 * @file
 */

#include "CommandCompleter.h"
#include "core/StringUtil.h"

namespace command {

int completeDir(const io::FilesystemPtr& filesystem, core::String dir, const core::String& match, core::DynamicArray<core::String>& matches) {
	const core::String additionalDir = core::string::extractDir(match.c_str());
	dir += additionalDir;
	core::String currentMatch;
	if (dir.empty()) {
		dir = ".";
		currentMatch = match;
	} else {
		currentMatch = match.substr(additionalDir.size());
	}

	core::DynamicArray<io::FilesystemEntry> entries;
	const core::String& filterPath = core::string::extractDir(match.c_str());
	filesystem->list(dir, entries, currentMatch + "*");
	int i = 0;
	for (const io::FilesystemEntry& entry : entries) {
		if (entry.type != io::FilesystemEntry::Type::dir) {
			continue;
		}
		core::String name = filterPath.empty() ? entry.name : filterPath + entry.name;
		name.append("/");
		matches.push_back(name);
		++i;
	}
	return i;
}

int complete(const io::FilesystemPtr& filesystem, core::String dir, const core::String& match, core::DynamicArray<core::String>& matches, const char* pattern) {
	const core::String additionalDir = core::string::extractDir(match.c_str());
	dir += additionalDir;
	core::String currentMatch;
	if (dir.empty()) {
		dir = ".";
		currentMatch = match;
	} else {
		currentMatch = match.substr(additionalDir.size());
	}

	core::DynamicArray<io::FilesystemEntry> entries;
	const core::String filter = match + pattern;
	const core::String& filterName = core::string::extractFilenameWithExtension(filter.c_str());
	const core::String& filterPath = core::string::extractDir(filter.c_str());
	filesystem->list(dir, entries, currentMatch + "*");
	int i = 0;
	for (const io::FilesystemEntry& entry : entries) {
		if (entry.type == io::FilesystemEntry::Type::dir) {
			core::String name = filterPath.empty() ? entry.name : filterPath + entry.name;
			name.append("/");
			matches.push_back(name);
			++i;
		}
	}
	entries.clear();
	filesystem->list(dir, entries, core::String(filterName));
	for (const io::FilesystemEntry& entry : entries) {
		if (entry.type == io::FilesystemEntry::Type::file) {
			matches.push_back(filterPath.empty() ? entry.name : filterPath + entry.name);
			++i;
		}
	}
	return i;
}

int complete(const core::String& match, core::DynamicArray<core::String>& matches, const char* const* values, size_t valueCount) {
	int i = 0;
	for (size_t n = 0; n < valueCount; ++n) {
		if (core::string::startsWith(values[n], match.c_str())) {
			matches.push_back(values[n]);
			++i;
		}
	}
	return i;
}

}
