/**
 * @file
 */

#include "ScriptApi.h"
#include "app/App.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "http/Request.h"
#include "io/BufferedReadWriteStream.h"
#include "json/JSON.h"

namespace voxelui {

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

	core::String dir;
	if (info.type == "generator") {
		dir = "scripts";
	} else if (info.type == "brush") {
		dir = "brushes";
	} else if (info.type == "selectionmode") {
		dir = "selectionmodes";
	} else {
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

} // namespace voxelui
