/**
 * @file
 */

#include "GithubAPI.h"
#include "JsonUtil.h"
#include "app/App.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/Trace.h"
#include "http/HttpCacheStream.h"

namespace github {

core::String downloadUrl(const core::String &repository, const core::String &branch, const core::String &path) {
	return "https://raw.githubusercontent.com/" + repository + "/" + branch + "/" + core::string::urlPathEncode(path);
}

core::DynamicArray<TreeEntry> reposGitTrees(const core::String &repository, const core::String &branch) {
	core_trace_scoped(ReposGitTrees);
	const core::String url = "https://api.github.com/repos/" + repository + "/git/trees/" + branch + "?recursive=1";
	const core::String file = "github-" + repository + "-" + branch + ".json";
	http::HttpCacheStream stream(io::filesystem(), file, url);
	core::DynamicArray<TreeEntry> entries;
	if (!stream.valid()) {
		return entries;
	}
	core::String json;
	stream.readString(stream.size(), json);
	nlohmann::json jsonResponse = nlohmann::json::parse(json);
	if (!jsonResponse.contains("tree")) {
		const core::String str = jsonResponse.dump().c_str();
		Log::error("Unexpected json data for url: '%s': %s", url.c_str(), str.c_str());
		return entries;
	}
	jsonResponse = jsonResponse["tree"];
	Log::debug("Found json for repository %s with %i entries", repository.c_str(), (int)jsonResponse.size());
	for (auto &entry : jsonResponse) {
		if (get(entry, "type") != "blob") {
			continue;
		}
		TreeEntry treeEntry;
		treeEntry.path = get(entry, "path");
		treeEntry.url = downloadUrl(repository, branch, treeEntry.path);
		entries.push_back(treeEntry);
	}

	return entries;
}

} // namespace github
