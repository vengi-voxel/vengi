/**
 * @file
 */

#include "IncludeUtil.h"
#include "app/App.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "io/Filesystem.h"

namespace util {

core::Pair<core::String, bool> handleIncludes(const core::String &filename, const core::String &buffer,
											  const core::List<core::String> &includeDirs,
											  core::List<core::String> *includedFiles) {
	core::String src;
	const core::String include = "#include";
	int index = 0;
	int line = 1;
	bool success = true;
	for (auto i = buffer.begin(); i != buffer.end(); ++i, ++index) {
		const char *c = &buffer[index];
		if (*c == '\n' || (*c == '\r' && *(c + 1) != '\n')) {
			++line;
		}
		if (*c != '#') {
			const char buf[] = {*c, '\0'};
			src.append(buf);
			continue;
		}
		if (SDL_strncmp(include.c_str(), c, include.size()) != 0) {
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
			const io::FilesystemPtr &fs = io::filesystem();
			for (; i != buffer.end(); ++i, ++index) {
				const char *cEnd = &buffer[index];
				if (*cEnd != '"' && *cEnd != '>') {
					continue;
				}

				bool found = false;
				const core::String includeFile(cStart + 1, (size_t)(cEnd - (cStart + 1)));
				for (const core::String &dir : includeDirs) {
					const core::String &fullPath = core::string::path(dir, includeFile);
					if (!fs->exists(fullPath)) {
						continue;
					}
					const core::String &includeBuffer = fs->load(fullPath);
					if (includeBuffer.empty()) {
						Log::error("could not load include file '%s' from dir '%s' at line %i from file '%s'",
								   includeFile.c_str(), dir.c_str(), line, filename.c_str());
						success = false;
					} else {
						if (includedFiles != nullptr) {
							includedFiles->insert(fullPath);
						}
						// src.append("#line 1 ").append(fullPath).append("\n");
						src.append(includeBuffer);
						found = true;
					}
					break;
				}
				// src.append("#line ").append(line).append(" ").append(filename).append("\n");
				if (!found) {
					success = false;
					Log::error("Failed to resolve include '%s' at line %i from file '%s'", includeFile.c_str(), line,
							   filename.c_str());
					for (const core::String &dir : includeDirs) {
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
	return {src, success};
}

} // namespace util
