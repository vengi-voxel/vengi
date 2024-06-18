/**
 * @file
 */

#include "CubZHAPI.h"
#include "app/App.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/Trace.h"
#include "http/Http.h"
#include "http/Request.h"
#include "io/BufferedReadWriteStream.h"
#include "io/Stream.h"
#include <json.hpp>

namespace voxelcollection {
namespace cubzh {

core::String downloadUrl(const core::String &repo, const core::String &name) {
	return "https://api.cu.bzh/items/" + repo + "/" + name + "/model";
}

static nlohmann::json request(const io::ArchivePtr &archive, const core::String &tk, const core::String &usrId, int page) {
	const core::String filename = core::string::format("itemdrafts_%i.json", page);
	if (archive->exists(filename)) {
		core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
		core::String json;
		if (stream->readString((int)stream->size(), json)) {
			auto j = nlohmann::json::parse(json, nullptr, false, true);
			if (j.contains("results")) {
				Log::debug("Use page %i from cache", page);
				return j;
			}
		}
	}
	const core::String url = core::string::format("https://api.cu.bzh/itemdrafts?perPage=250&page=%i", page);
	http::Request initialRequest(url, http::RequestType::GET);
	initialRequest.addHeader("Czh-Tk", tk);
	initialRequest.addHeader("Czh-Usr-Id", usrId);
	io::BufferedReadWriteStream stream;
	int status = 0;
	if (!initialRequest.execute(stream, &status)) {
		Log::error("Failed to download %s", url.c_str());
		return {};
	}
	if (!http::isValidStatusCode(status)) {
		Log::error("Failed to download %s with status %i", url.c_str(), status);
		return {};
	}
	stream.seek(0);
	core::String json;
	stream.readString((int)stream.size(), json);
	stream.seek(0);
	archive->write(filename, stream);
	auto j = nlohmann::json::parse(json, nullptr, false, true);
	if (!j.contains("results")) {
		Log::error("Unexpected json results data: '%s' with status %i", json.c_str(), status);
	}
	return j;
}

core::DynamicArray<TreeEntry> repoList(const io::ArchivePtr &archive, const core::String &tk,
									   const core::String &usrId) {
	core_trace_scoped(RepoList);
	const auto &firstPage = request(archive, tk, usrId, 1);
	if (!firstPage.contains("results")) {
		return {};
	}
	const int totalResults = firstPage.value("totalResults", 0);
	const int perPage = firstPage.value("perPage", 0);
	const int totalPages = (totalResults + perPage - 1) / perPage;
	core::DynamicArray<TreeEntry> entries;
	for (int page = 2; page <= totalPages; ++page) {
		if (app::App::getInstance()->shouldQuit()) {
			break;
		}
		Log::debug("Fetching page %i of %i", page, totalPages);
		const auto &jsonResponse = request(archive, tk, usrId, page);
		if (!jsonResponse.contains("results")) {
			return entries;
		}
		for (const auto &r : jsonResponse["results"]) {
			TreeEntry treeEntry;
			treeEntry.repo = r.value("repo", "").c_str();
			treeEntry.name = r.value("name", "").c_str();
			treeEntry.id = r.value("id", "").c_str();
			treeEntry.likes = r.value("likes", 0);
			treeEntry.url = downloadUrl(treeEntry.repo, treeEntry.name);
			entries.push_back(treeEntry);
		}
	}

	return entries;
}

} // namespace cubzh
} // namespace voxelcollection
