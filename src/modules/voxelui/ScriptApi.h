/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "io/Filesystem.h"

namespace voxelui {

struct ScriptInfo {
	core::String type;
	core::String name;
	core::String description;
	core::String version;
	core::String author;
	core::String filename;
};

typedef core::DynamicArray<ScriptInfo> ScriptInfoList;

class ScriptApi {
public:
	/**
	 * @brief Fetch the list of available scripts from the API
	 */
	ScriptInfoList query(const core::String &baseUrl) const;

	/**
	 * @brief Download a script from the API and install it into the home directory.
	 * Generator scripts go to "scripts/", brush scripts go to "brushes/".
	 * @return @c true if the download and install succeeded
	 */
	bool download(const io::FilesystemPtr &filesystem, const core::String &baseUrl, const ScriptInfo &info) const;
};

} // namespace voxelui
