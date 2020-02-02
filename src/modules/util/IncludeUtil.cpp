/**
 * @file
 */

#include "IncludeUtil.h"
#include "core/StringUtil.h"
#include "core/io/Filesystem.h"
#include "core/App.h"
#include "core/Log.h"

namespace util {

std::pair<core::String, bool> handleIncludes(const core::String& buffer, const std::vector<core::String>& includeDirs, std::vector<core::String>* includedFiles) {
	core::String src;
	const core::String include = "#include";
	int index = 0;
	bool success = true;
	for (std::string::const_iterator i = buffer.begin(); i != buffer.end(); ++i, ++index) {
		const char *c = &buffer[index];
		if (*c != '#') {
			const char buf[] = {*c, '\0'};
			src.append(buf);
			continue;
		}
		if (::strncmp(include.c_str(), c, include.size())) {
			const char buf[] = {*c, '\0'};
			src.append(buf);
			continue;
		}
		for (; i != buffer.end(); ++i, ++index) {
			const char *cStart = &buffer[index];
			if (*cStart != '"' && *cStart != '<') {
				continue;
			}

			++index;
			++i;
			const io::FilesystemPtr& fs = io::filesystem();
			for (; i != buffer.end(); ++i, ++index) {
				const char *cEnd = &buffer[index];
				if (*cEnd != '"' && *cEnd != '>') {
					continue;
				}

				bool found = false;
				const core::String includeFile(cStart + 1, (size_t)(cEnd - (cStart + 1)));
				for (const core::String& dir : includeDirs) {
					const core::String& fullPath = dir + "/" + includeFile;
					if (!fs->exists(fullPath)) {
						continue;
					}
					const core::String& includeBuffer = fs->load(fullPath);
					if (includeBuffer.empty()) {
						Log::error("could not load shader include %s from dir %s", includeFile.c_str(), dir.c_str());
						success = false;
					} else {
						if (includedFiles != nullptr) {
							includedFiles->push_back(fullPath);
						}
						src.append(includeBuffer);
						found = true;
					}
					break;
				}
				if (!found) {
					success = false;
					Log::error("Failed to resolve include '%s'", includeFile.c_str());
					for (const core::String& dir : includeDirs) {
						Log::error("Include paths: %s", dir.c_str());
					}
				}
				break;
			}
			break;
		}
		if (i == buffer.end()) {
			break;
		}
	}
	return std::make_pair(src, success);
}

}

