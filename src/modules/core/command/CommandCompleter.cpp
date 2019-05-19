/**
 * @file
 */

#include "CommandCompleter.h"
#include "core/App.h"
#include "core/String.h"
#include "io/Filesystem.h"

namespace core {

int complete(const std::string& dir, const std::string& str, std::vector<std::string>& matches) {
	std::vector<io::Filesystem::DirEntry> entries;
	const std::string filter = str + "*";
	const std::string_view& filterName = core::string::extractFilename(filter.c_str());
	const std::string_view& filterPath = core::string::extractPath(filter.c_str());
	const std::string filterPathStr(filterPath);
	core::App::getInstance()->filesystem()->list(dir, entries, std::string(filterName));
	int i = 0;
	for (const io::Filesystem::DirEntry& entry : entries) {
		if (entry.type == io::Filesystem::DirEntry::Type::file) {
			matches.push_back(filterPathStr.empty() ? entry.name : filterPathStr + entry.name);
			++i;
		} else if (entry.type == io::Filesystem::DirEntry::Type::dir) {
			std::string name = filterPathStr.empty() ? entry.name : filterPathStr + entry.name;
			name.append("/");
			matches.push_back(name);
			++i;
		}
	}
	return i;
}

}
