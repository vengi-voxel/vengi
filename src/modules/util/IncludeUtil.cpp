/**
 * @file
 */

#include "IncludeUtil.h"
#include "core/String.h"
#include "io/Filesystem.h"
#include "core/App.h"
#include "core/Log.h"

namespace util {

std::pair<std::string, bool> handleIncludes(const std::string& buffer, const std::vector<std::string>& includeDirs, std::vector<std::string>* includedFiles) {
	std::string src;
	const std::string_view include = "#include";
	int index = 0;
	bool success = true;
	for (std::string::const_iterator i = buffer.begin(); i != buffer.end(); ++i, ++index) {
		const char *c = &buffer[index];
		if (*c != '#') {
			src.append(c, 1);
			continue;
		}
		if (::strncmp(include.data(), c, include.length())) {
			src.append(c, 1);
			continue;
		}
		for (; i != buffer.end(); ++i, ++index) {
			const char *cStart = &buffer[index];
			if (*cStart != '"') {
				continue;
			}

			++index;
			++i;
			const io::FilesystemPtr& fs = core::App::getInstance()->filesystem();
			for (; i != buffer.end(); ++i, ++index) {
				const char *cEnd = &buffer[index];
				if (*cEnd != '"') {
					continue;
				}

				bool found = false;
				const std::string includeFile(cStart + 1, (size_t)(cEnd - (cStart + 1)));
				for (const std::string& dir : includeDirs) {
					const std::string& fullPath = core::string::concat(dir + "/", includeFile);
					if (!fs->exists(fullPath)) {
						continue;
					}
					const std::string& includeBuffer = fs->load(fullPath);
					if (includeBuffer.empty()) {
						Log::error("could not load shader include %s from dir %s", includeFile.c_str(), dir.data());
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
					for (const std::string& dir : includeDirs) {
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

