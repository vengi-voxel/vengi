/**
 * @file
 */

#include "ScriptApi.h"
#include "app/App.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "http/Request.h"
#include "io/BufferedReadWriteStream.h"
#include "io/File.h"
#include "json/JSON.h"

namespace voxelui {

static const char *SCRIPT_DIRS[] = {"scripts", "brushes", "selectionmodes"};
static const char *SCRIPT_TYPES[] = {"generator", "brush", "selectionmode"};

void ScriptApi::query(const core::String &baseUrl, core::Function<void(ScriptInfoList)> callback) const {
	http::Request request(baseUrl + "/scripts", http::RequestType::GET);
	if (_client.execute(request, [callback](const http::Response &response) {
			callback(response.success && response.statusCode == 200 ? parseList(response.text()) : ScriptInfoList());
		}) == 0) {
		callback({});
	}
}

ScriptInfoList ScriptApi::parseList(const core::String &body) {
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

void ScriptApi::download(const io::FilesystemPtr &filesystem, const core::String &baseUrl, const ScriptInfo &info,
						 core::Function<void(bool)> callback) const {
	http::Request request(baseUrl + "/scripts/download/" + info.filename, http::RequestType::GET);
	if (_client.execute(request, [filesystem, info, callback](const http::Response &response) {
			const core::String dir = scriptTypeToDir(info.type);
			if (!response.success || response.statusCode != 200 || dir.empty()) {
				callback(false);
				return;
			}
			filesystem->sysCreateDir(filesystem->homeWritePath(dir));
			callback(filesystem->homeWrite(core::string::path(dir, info.filename), response.body.data(),
										   response.body.size()));
		}) == 0) {
		callback(false);
	}
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

static core::String loadFromUrl(const core::String &url) {
	http::Request request(url, http::RequestType::GET);
	request.setUserAgent(app::App::getInstance()->fullAppname());

	io::BufferedReadWriteStream stream;
	int statusCode = 0;
	if (!request.execute(stream, &statusCode)) {
		Log::error("Failed to download script from %s", url.c_str());
		return "";
	}
	if (statusCode != 200) {
		Log::error("Script download returned status %d for %s", statusCode, url.c_str());
		return "";
	}

	stream.seek(0);
	core::String body;
	stream.readString((int)stream.size(), body);
	return body;
}

static core::String loadFromFile(const core::String &path) {
	io::File f(path, io::FileMode::SysRead);
	if (!f.validHandle()) {
		Log::error("Failed to open script file: %s", path.c_str());
		return "";
	}
	return f.load();
}

static bool installSource(const io::FilesystemPtr &filesystem, const core::String &filename,
						  const core::String &luaSource) {
	if (luaSource.empty()) {
		Log::error("Failed to read script: %s", filename.c_str());
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

bool ScriptApi::install(const io::FilesystemPtr &filesystem, const core::String &source) const {
	core::String luaSource;
	core::String filename;

	if (core::string::startsWith(source, "http://") || core::string::startsWith(source, "https://")) {
		luaSource = loadFromUrl(source);
		filename = core::string::extractFilenameWithExtension(source);
	} else if (core::string::startsWith(source, "file://")) {
		const core::String path = source.substr(7);
		luaSource = loadFromFile(path);
		filename = core::string::extractFilenameWithExtension(path);
	} else {
		luaSource = loadFromFile(source);
		filename = core::string::extractFilenameWithExtension(source);
	}

	return installSource(filesystem, filename, luaSource);
}

void ScriptApi::install(const io::FilesystemPtr &filesystem, const core::String &source,
						core::Function<void(bool)> callback) const {
	if (!core::string::startsWith(source, "http://") && !core::string::startsWith(source, "https://")) {
		callback(install(filesystem, source));
		return;
	}
	http::Request request(source, http::RequestType::GET);
	const core::String filename = core::string::extractFilenameWithExtension(source);
	if (_client.execute(request, [filesystem, filename, callback](const http::Response &response) {
			callback(response.success && response.statusCode == 200 &&
					 installSource(filesystem, filename, response.text()));
		}) == 0) {
		callback(false);
	}
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
