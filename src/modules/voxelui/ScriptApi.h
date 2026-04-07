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

	/**
	 * @brief Uninstall a previously installed script by removing its file from the home directory.
	 * @return @c true if the file was removed successfully
	 */
	bool uninstall(const io::FilesystemPtr &filesystem, const ScriptInfo &info) const;

	/**
	 * @brief Detect the script type from its Lua source code by looking for known entry functions.
	 * @return "generator" if it has main(), "brush" if it has generate(), "selectionmode" if it has select(),
	 * or an empty string if the type cannot be determined.
	 */
	static core::String detectScriptType(const core::String &luaSource);

	/**
	 * @brief Returns the directory name for a given script type.
	 * @return "scripts" for "generator", "brushes" for "brush", "selectionmodes" for "selectionmode",
	 * or an empty string for unknown types.
	 */
	static core::String scriptTypeToDir(const core::String &type);

	/**
	 * @brief Install a script from a source that can be a local file path, a file:// URI, or an http(s):// URL.
	 * The script type is auto-detected from the Lua source code.
	 * @return @c true if the install succeeded
	 */
	bool install(const io::FilesystemPtr &filesystem, const core::String &source) const;

	/**
	 * @brief Uninstall a script by its filename. Searches across all script directories (scripts/, brushes/,
	 * selectionmodes/) to find and remove the file.
	 * @return @c true if the file was found and removed successfully
	 */
	bool uninstallByFilename(const io::FilesystemPtr &filesystem, const core::String &filename) const;
};

} // namespace voxelui
