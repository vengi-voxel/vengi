/**
 * @file
 */

#include "CommandCompleter.h"
#include "core/App.h"
#include "io/Filesystem.h"

namespace core {

int complete(const std::string& dir, const std::string& str, std::vector<std::string>& matches) {
	std::vector<io::Filesystem::DirEntry> entries;
	const std::string filter = str + "*";
	core::App::getInstance()->filesystem()->list(dir, entries, filter);
	int i = 0;
	for (const io::Filesystem::DirEntry& entry : entries) {
		if (entry.type == io::Filesystem::DirEntry::Type::file) {
			matches.push_back(entry.name);
			++i;
		}
	}
	return i;
}

}
