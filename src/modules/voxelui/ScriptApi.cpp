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

ScriptInfoList ScriptApi::query(const core::String &baseUrl) const {
	ScriptInfoList list;
	const core::String url = baseUrl + "/scripts";

	http::Request request(url, http::RequestType::GET);
	request.setUserAgent(app::App::getInstance()->fullAppname());

	io::BufferedReadWriteStream stream;
	int statusCode = 0;
	if (!request.execute(stream, &statusCode)) {
		Log::error("Failed to query script API at %s", url.c_str());
		return list;
	}
	if (statusCode != 200) {
		Log::error("Script API returned status %d", statusCode);
		return list;
	}

	stream.seek(0);
	core::String body;
	stream.readString((int)stream.size(), body);

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

bool ScriptApi::download(const io::FilesystemPtr &filesystem, const core::String &baseUrl, const ScriptInfo &info) const {
	const core::String url = baseUrl + "/scripts/download/" + info.filename;

	http::Request request(url, http::RequestType::GET);
	request.setUserAgent(app::App::getInstance()->fullAppname());

	io::BufferedReadWriteStream stream;
	int statusCode = 0;
	if (!request.execute(stream, &statusCode)) {
		Log::error("Failed to download script %s", info.filename.c_str());
		return false;
	}
	if (statusCode != 200) {
		Log::error("Script download returned status %d for %s", statusCode, info.filename.c_str());
		return false;
	}

	core::String dir = scriptTypeToDir(info.type);
	if (dir.empty()) {
		Log::error("Unknown script type: %s", info.type.c_str());
		return false;
	}

	const core::String targetPath = core::string::path(dir, info.filename);
	const core::String fullDir = filesystem->homeWritePath(dir);
	filesystem->sysCreateDir(fullDir);

	stream.seek(0);
	if (filesystem->homeWrite(targetPath, stream) <= 0) {
		Log::error("Failed to write script to %s", targetPath.c_str());
		return false;
	}

	Log::info("Installed script %s to %s", info.filename.c_str(), targetPath.c_str());
	return true;
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

	if (luaSource.empty()) {
		Log::error("Failed to read script from: %s", source.c_str());
		return false;
	}

	if (core::string::extractExtension(filename) != "lua") {
		Log::error("Script file must have .lua extension: %s", filename.c_str());
		return false;
	}

	const core::String type = detectScriptType(luaSource);
	if (type.empty()) {
		Log::error("Could not detect script type for %s - must contain function main(), generate() or select()",
				   filename.c_str());
		return false;
	}

	const core::String dir = scriptTypeToDir(type);
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
