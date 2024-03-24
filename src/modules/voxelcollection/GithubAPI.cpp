/**
 * @file
 */

#include "GithubAPI.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/Trace.h"
#include "http/HttpCacheStream.h"
#include "io/Filesystem.h"
#include <json.hpp>

namespace voxelcollection {
namespace github {

core::String downloadUrl(const core::String &repository, const core::String &branch, const core::String &path) {
	return "https://raw.githubusercontent.com/" + repository + "/" + branch + "/" + core::string::urlPathEncode(path);
}

core::DynamicArray<TreeEntry> reposGitTrees(const io::FilesystemPtr &filesystem, const core::String &repository,
											const core::String &branch, const core::String &path) {
	core_trace_scoped(ReposGitTrees);
	const core::String url = "https://api.github.com/repos/" + repository + "/git/trees/" + branch + "?recursive=1";
	core::String file = "github-" + repository + "-" + branch + ".json";
	core::string::replaceAllChars(file, '/', '-');
	http::HttpCacheStream stream(filesystem, file, url);
	core::DynamicArray<TreeEntry> entries;
	if (!stream.valid()) {
		return entries;
	}
	core::String json;
	stream.readString((int)stream.size(), json);
	nlohmann::json jsonResponse = nlohmann::json::parse(json, nullptr, false, true);
	if (!jsonResponse.contains("tree")) {
		const core::String str = jsonResponse.dump().c_str();
		Log::error("Unexpected json data for url: '%s': %s", url.c_str(), str.c_str());
		return entries;
	}
	jsonResponse = jsonResponse["tree"];
	Log::debug("Found json for repository %s with %i entries", repository.c_str(), (int)jsonResponse.size());
	for (const auto &entry : jsonResponse) {
		if (entry.value("type", "") != "blob") {
			continue;
		}
		TreeEntry treeEntry;
		treeEntry.path = entry.value("path", "").c_str();
		if (!path.empty() && !core::string::startsWith(treeEntry.path, path)) {
			continue;
		}
		treeEntry.url = downloadUrl(repository, branch, treeEntry.path);
		entries.push_back(treeEntry);
	}

	return entries;
}

} // namespace github
} // namespace voxelcollection
