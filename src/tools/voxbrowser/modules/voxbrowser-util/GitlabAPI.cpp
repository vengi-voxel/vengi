/**
 * @file
 */

#include "GitlabAPI.h"
#include "JsonUtil.h"
#include "app/App.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/Trace.h"
#include "core/collection/StringMap.h"
#include "http/HttpCacheStream.h"
#include "http/Request.h"

namespace gitlab {

core::String downloadUrl(const core::String &repository, const core::String &branch, const core::String &path) {
	return core::string::format("https://gitlab.com/veloren/veloren/-/raw/%s/%s", branch.c_str(), path.c_str());
}

core::DynamicArray<TreeEntry> reposGitTrees(const io::FilesystemPtr &filesystem, const core::String &repository,
											const core::String &branch, const core::String &path) {
	core_trace_scoped(ReposGitTrees);
	const core::String encoded = core::string::urlEncode(repository);
	const core::String urlPages = core::string::format(
		"https://gitlab.com/api/v4/projects/%s/repository/tree?ref=%s&recursive=1&per_page=100&page=1&path=%s",
		encoded.c_str(), branch.c_str(), path.c_str());
	http::Request request(urlPages, http::RequestType::GET);
	io::NOPWriteStream s;
	core::StringMap<core::String> headers;
	if (!request.execute(s, nullptr, &headers)) {
		Log::error("Failed to download tree data for url %s", urlPages.c_str());
		return {};
	}
	core::String totalPagesStr;
	int totalPages = 1;
	if (headers.get("x-total-pages", totalPagesStr)) {
		totalPages = core::string::toInt(totalPagesStr);
		Log::info("Fetch %i pages for repository %s", totalPages, repository.c_str());
	}
	core::DynamicArray<TreeEntry> entries;
	for (int page = 1; page <= totalPages; ++page) {
		const core::String url = core::string::format(
			"https://gitlab.com/api/v4/projects/%s/repository/tree?ref=%s&recursive=1&per_page=100&page=%i&path=%s",
			encoded.c_str(), branch.c_str(), page, path.c_str());
		core::String file = core::string::format("gitlab-%s-%s-page%i.json", repository.c_str(), branch.c_str(), page);
		core::string::replaceAllChars(file, '/', '-');
		http::HttpCacheStream stream(filesystem, file, url);
		if (!stream.valid()) {
			return entries;
		}
		core::String json;
		stream.readString(stream.size(), json);
		nlohmann::json jsonResponse = nlohmann::json::parse(json, nullptr, false, true);
		if (!jsonResponse.is_array()) {
			const core::String str = jsonResponse.dump().c_str();
			Log::error("Unexpected json data for url: '%s': %s", url.c_str(), str.c_str());
			return entries;
		}
		Log::debug("Found json for repository %s with %i entries", repository.c_str(), (int)jsonResponse.size());
		for (const auto &entry : jsonResponse) {
			if (get(entry, "type") != "blob") {
				continue;
			}
			TreeEntry treeEntry;
			treeEntry.path = get(entry, "path");
			if (!path.empty() && !core::string::startsWith(treeEntry.path, path)) {
				continue;
			}
			treeEntry.url = downloadUrl(repository, branch, treeEntry.path);
			entries.push_back(treeEntry);
		}
	}

	return entries;
}

} // namespace gitlab
