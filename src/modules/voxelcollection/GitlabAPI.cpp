/**
 * @file
 */

#include "GitlabAPI.h"
#include "app/App.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/Trace.h"
#include "core/collection/StringMap.h"
#include "http/HttpCacheStream.h"
#include "http/Request.h"
#include "json/JSON.h"

namespace voxelcollection {
namespace gitlab {

static nlohmann::json cachedJson(const io::ArchivePtr &archive, const core::String &file, const core::String &url) {
	const core::String json = http::HttpCacheStream::string(archive, file, url);
	if (json.empty()) {
		return {};
	}
	return nlohmann::json::parse(json, nullptr, false, true);
}

core::String downloadUrl(const core::String &repository, const core::String &branch, const core::String &path) {
	return core::String::format("https://gitlab.com/%s/-/raw/%s/%s", repository.c_str(), branch.c_str(), path.c_str());
}

core::DynamicArray<TreeEntry> reposGitTrees(const io::ArchivePtr &archive, const core::String &repository,
											const core::String &branch, const core::String &path) {
	core_trace_scoped(ReposGitTrees);
	const core::String encoded = core::string::urlEncode(repository);
	const core::String urlPages = core::String::format(
		"https://gitlab.com/api/v4/projects/%s/repository/tree?ref=%s&recursive=1&per_page=100&page=1&path=%s",
		encoded.c_str(), branch.c_str(), path.c_str());
	http::Request request(urlPages, http::RequestType::GET);
	io::NOPWriteStream s;
	http::Headers headers;
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
		if (app::App::getInstance()->shouldQuit()) {
			break;
		}
		const core::String url = core::String::format(
			"https://gitlab.com/api/v4/projects/%s/repository/tree?ref=%s&recursive=1&per_page=100&page=%i&path=%s",
			encoded.c_str(), branch.c_str(), page, path.c_str());
		core::String file = core::String::format("gitlab-%s-%s-page%i.json", repository.c_str(), branch.c_str(), page);
		core::string::replaceAllChars(file, '/', '-');
		const auto &json = cachedJson(archive, file, url);
		if (!json.is_array()) {
			const core::String str = json.dump().c_str();
			Log::error("Unexpected json data for url: '%s': %s", url.c_str(), str.c_str());
			return entries;
		}
		Log::debug("Found json for repository %s with %i entries", repository.c_str(), (int)json.size());
		for (const auto &entry : json) {
			const auto type = entry.value("type", "");
			if (type != "blob") {
				Log::debug("No blob entry, but %s", type.c_str());
				continue;
			}
			TreeEntry treeEntry;
			treeEntry.path = entry.value("path", "").c_str();
			if (!path.empty() && !core::string::startsWith(treeEntry.path, path)) {
				Log::debug("Ignore entry %s - not in path %s", treeEntry.path.c_str(), path.c_str());
				continue;
			}
			treeEntry.url = downloadUrl(repository, branch, treeEntry.path);
			entries.push_back(treeEntry);
		}
	}

	return entries;
}

} // namespace gitlab
} // namespace voxelcollection
