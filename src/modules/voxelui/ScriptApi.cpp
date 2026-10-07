/**
 * @file
 */

#include "ScriptApi.h"
#include "app/App.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "http/Http.h"
#include "http/RequestAsync.h"
#include "io/File.h"
#include "json/JSON.h"

namespace voxelui {

static const char *SCRIPT_DIRS[] = {"scripts", "brushes", "selectionmodes"};
static const char *SCRIPT_TYPES[] = {"generator", "brush", "selectionmode"};

ScriptInfoList ScriptApi::parseScriptList(const core::String &body) {
	ScriptInfoList list;
	json::Json jsonResponse = json::Json::parse(body);
	if (!jsonResponse.isValid() || !jsonResponse.isArray()) {
		Log::error("Invalid JSON response from script API");
		return list;
	}

	for (const auto &entry : jsonResponse) {
		ScriptInfo info;
		info.type = entry.strVal("type", "");
		info.name = entry.strVal("name", "");
		info.description = entry.strVal("description", "");
		info.version = entry.strVal("version", "");
		info.author = entry.strVal("author", "");
		info.filename = entry.strVal("filename", "");
		if (!info.filename.empty()) {
			list.push_back(info);
		}
	}

	Log::info("Fetched %d scripts from API", (int)list.size());
	return list;
}

static http::Request makeScriptRequest(const core::String &url) {
	http::Request request(url, http::RequestType::GET);
	request.setUserAgent(app::App::getInstance()->fullAppname());
	return request;
}

static bool writeDownloadedScript(const io::FilesystemPtr &filesystem, const ScriptInfo &info,
								  const core::String &body) {
	core::String dir = ScriptApi::scriptTypeToDir(info.type);
	if (dir.empty()) {
		Log::error("Unknown script type: %s", info.type.c_str());
		return false;
	}

	const core::String targetPath = core::string::path(dir, info.filename);
	const core::String fullDir = filesystem->homeWritePath(dir);
	filesystem->sysCreateDir(fullDir);

	if (!filesystem->homeWrite(targetPath, body)) {
		Log::error("Failed to write script to %s", targetPath.c_str());
		return false;
	}

	Log::info("Installed script %s to %s", info.filename.c_str(), targetPath.c_str());
	return true;
}

void ScriptApi::queryAsync(const core::String &baseUrl, ScriptListCallback &&callback) const {
	const core::String url = baseUrl + "/scripts";
	http::requestAsync(makeScriptRequest(url), [callback = core::move(callback)](const http::Response &response) mutable {
		ScriptInfoList list;
		if (!response.success || !http::isValidStatusCode(response.statusCode)) {
			Log::error("Failed to query script API at %s (status %d)", response.url.c_str(), response.statusCode);
			callback(ScriptInfoList());
			return;
		}
		callback(parseScriptList(response.body));
	});
}

void ScriptApi::downloadAsync(const io::FilesystemPtr &filesystem, const core::String &baseUrl, const ScriptInfo &info,
							  ScriptBoolCallback &&callback) const {
	const core::String url = baseUrl + "/scripts/download/" + info.filename;
	const ScriptInfo infoCopy = info;
	http::requestAsync(makeScriptRequest(url),
					   [filesystem, infoCopy, callback = core::move(callback)](const http::Response &response) mutable {
						   if (!response.success || !http::isValidStatusCode(response.statusCode)) {
							   Log::error("Failed to download script %s (status %d)", infoCopy.filename.c_str(),
										  response.statusCode);
							   callback(false);
							   return;
						   }
						   callback(writeDownloadedScript(filesystem, infoCopy, response.body));
					   });
}

bool ScriptApi::uninstall(const io::FilesystemPtr &filesystem, const ScriptInfo &info) const {
	const core::String dir = scriptTypeToDir(info.type);
	if (dir.empty()) {
		Log::error("Unknown script type: %s", info.type.c_str());
		return false;
	}

	const core::String path = filesystem->homeWritePath(core::string::path(dir, info.filename));
	if (!io::Filesystem::sysRemoveFile(path)) {
		Log::error("Failed to remove script %s", path.c_str());
		return false;
	}

	Log::info("Uninstalled script %s", info.filename.c_str());
	return true;
}

core::String ScriptApi::scriptTypeToDir(const core::String &type) {
	for (int i = 0; i < lengthof(SCRIPT_DIRS); ++i) {
		if (type == SCRIPT_TYPES[i]) {
			return SCRIPT_DIRS[i];
		}
	}
	return "";
}

core::String ScriptApi::detectScriptType(const core::String &luaSource) {
	if (luaSource.empty()) {
		return "";
	}
	// look for: 'function select(' or 'function select ('
	if (luaSource.contains("function select")) {
		return "selectionmode";
	}
	// look for: 'function generate(' or 'function generate ('
	if (luaSource.contains("function generate")) {
		return "brush";
	}
	// look for: 'function main(' or 'function main ('
	if (luaSource.contains("function main")) {
		return "generator";
	}
	return "";
}

static core::String loadFromFile(const core::String &path) {
	io::File f(path, io::FileMode::SysRead);
	if (!f.validHandle()) {
		Log::error("Failed to open script file: %s", path.c_str());
		return "";
	}
	return f.load();
}

static bool installLuaSource(const io::FilesystemPtr &filesystem, const core::String &luaSource,
							 const core::String &filename) {
	if (luaSource.empty()) {
		return false;
	}

	if (core::string::extractExtension(filename) != "lua") {
		Log::error("Script file must have .lua extension: %s", filename.c_str());
		return false;
	}

	const core::String type = ScriptApi::detectScriptType(luaSource);
	if (type.empty()) {
		Log::error("Could not detect script type for %s - must contain function main(), generate() or select()",
				   filename.c_str());
		return false;
	}

	const core::String dir = ScriptApi::scriptTypeToDir(type);
	const core::String targetPath = core::string::path(dir, filename);
	const core::String fullDir = filesystem->homeWritePath(dir);
	filesystem->sysCreateDir(fullDir);

	if (!filesystem->homeWrite(targetPath, luaSource)) {
		Log::error("Failed to write script to %s", targetPath.c_str());
		return false;
	}

	Log::info("Installed %s script %s to %s", type.c_str(), filename.c_str(), dir.c_str());
	return true;
}

void ScriptApi::installAsync(const io::FilesystemPtr &filesystem, const core::String &source,
							 ScriptBoolCallback &&callback) const {
	if (core::string::startsWith(source, "http://") || core::string::startsWith(source, "https://")) {
		const core::String filename = core::string::extractFilenameWithExtension(source);
		http::requestAsync(makeScriptRequest(source),
						   [filesystem, filename, callback = core::move(callback)](const http::Response &response) mutable {
							   if (!response.success || !http::isValidStatusCode(response.statusCode)) {
								   Log::error("Failed to download script from %s (status %d)", response.url.c_str(),
											  response.statusCode);
								   callback(false);
								   return;
							   }
							   callback(installLuaSource(filesystem, response.body, filename));
						   });
		return;
	}

	core::String luaSource;
	core::String filename;
	if (core::string::startsWith(source, "file://")) {
		const core::String path = source.substr(7);
		luaSource = loadFromFile(path);
		filename = core::string::extractFilenameWithExtension(path);
	} else {
		luaSource = loadFromFile(source);
		filename = core::string::extractFilenameWithExtension(source);
	}
	if (luaSource.empty()) {
		Log::error("Failed to read script from: %s", source.c_str());
		callback(false);
		return;
	}
	callback(installLuaSource(filesystem, luaSource, filename));
}

bool ScriptApi::install(const io::FilesystemPtr &filesystem, const core::String &source) const {
	if (core::string::startsWith(source, "http://") || core::string::startsWith(source, "https://")) {
		Log::error("Use installAsync for http(s) script sources: %s", source.c_str());
		return false;
	}

	core::String luaSource;
	core::String filename;
	if (core::string::startsWith(source, "file://")) {
		const core::String path = source.substr(7);
		luaSource = loadFromFile(path);
		filename = core::string::extractFilenameWithExtension(path);
	} else {
		luaSource = loadFromFile(source);
		filename = core::string::extractFilenameWithExtension(source);
	}

	if (luaSource.empty()) {
		Log::error("Failed to read script from: %s", source.c_str());
		return false;
	}
	return installLuaSource(filesystem, luaSource, filename);
}

bool ScriptApi::uninstallByFilename(const io::FilesystemPtr &filesystem, const core::String &filename) const {
	for (int i = 0; i < lengthof(SCRIPT_DIRS); ++i) {
		const core::String path = filesystem->homeWritePath(core::string::path(SCRIPT_DIRS[i], filename));
		if (path.empty()) {
			continue;
		}
		if (io::Filesystem::sysExists(path)) {
			if (!io::Filesystem::sysRemoveFile(path)) {
				Log::error("Failed to remove script %s", path.c_str());
				return false;
			}
			Log::info("Uninstalled %s script %s", SCRIPT_TYPES[i], filename.c_str());
			return true;
		}
	}
	Log::error("Script %s not found in any script directory", filename.c_str());
	return false;
}

} // namespace voxelui
