/**
 * @file
 */

#pragma once

#include "core/Function.h"
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
using ScriptListCallback = core::Function<void(ScriptInfoList)>;
using ScriptBoolCallback = core::Function<void(bool)>;

class ScriptApi {
public:
	/**
	 * @brief Parse a script-list JSON body from the API.
	 */
	static ScriptInfoList parseScriptList(const core::String &body);

	/**
	 * @brief Fetch the script list asynchronously; @p callback runs on the main thread.
	 */
	void queryAsync(const core::String &baseUrl, ScriptListCallback &&callback) const;

	/**
	 * @brief Async download/install; @p callback runs on the main thread.
	 * Generator scripts go to "scripts/", brush scripts go to "brushes/".
	 */
	void downloadAsync(const io::FilesystemPtr &filesystem, const core::String &baseUrl, const ScriptInfo &info,
					   ScriptBoolCallback &&callback) const;

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
	 * @brief Install a script from a local file path or file:// URI.
	 * The script type is auto-detected from the Lua source code.
	 * For http(s) sources use @c installAsync.
	 * @return @c true if the install succeeded
	 */
	bool install(const io::FilesystemPtr &filesystem, const core::String &source) const;

	/**
	 * @brief Async install; http(s) sources use @c http::requestAsync. @p callback runs on the main thread.
	 * Local paths and file:// URIs are handled immediately on the calling thread.
	 */
	void installAsync(const io::FilesystemPtr &filesystem, const core::String &source, ScriptBoolCallback &&callback) const;

	/**
	 * @brief Uninstall a script by its filename. Searches across all script directories (scripts/, brushes/,
	 * selectionmodes/) to find and remove the file.
	 * @return @c true if the file was found and removed successfully
	 */
	bool uninstallByFilename(const io::FilesystemPtr &filesystem, const core::String &filename) const;
};

} // namespace voxelui
