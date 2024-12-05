/**
 * @file
 */

#include "GithubAPI.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/Trace.h"
#include "http/HttpCacheStream.h"
#include "json/JSON.h"

namespace voxelcollection {
namespace github {

core::String downloadUrl(const io::ArchivePtr &archive, const core::String &repository, const core::String &branch,
						 const core::String &path) {
	core_assert(!path.empty());
	const core::String &branchEnc = core::string::urlPathEncode(branch);
	const core::String &url = "https://api.github.com/repos/" + repository + "/contents/" + path + "?ref=" + branchEnc;
	core::String file = "github-" + repository + "-" + branch + "-" + path + ".json";
	core::string::replaceAllChars(file, '/', '-');
	const core::String json = http::HttpCacheStream::string(archive, file, url);
	if (!json.empty()) {
		nlohmann::json jsonResponse = nlohmann::json::parse(json, nullptr, false, true);
		if (jsonResponse.contains("download_url")) {
			auto dlurl = jsonResponse.value("download_url", "");
			return dlurl.c_str();
		} else {
			const core::String str = jsonResponse.dump().c_str();
			Log::error("Unexpected json data for url: '%s': %s", url.c_str(), str.c_str());
		}
	}
	return "https://raw.githubusercontent.com/" + repository + "/" + branch + "/" + core::string::urlPathEncode(path);
}

core::DynamicArray<TreeEntry> reposGitTrees(const io::ArchivePtr &archive, const core::String &repository,
											const core::String &branch, const core::String &path) {
	core_trace_scoped(ReposGitTrees);
	const core::String url = "https://api.github.com/repos/" + repository + "/git/trees/" + branch + "?recursive=1";
	core::String file = "github-" + repository + "-" + branch + ".json";
	core::string::replaceAllChars(file, '/', '-');
	core::DynamicArray<TreeEntry> entries;
	const core::String json = http::HttpCacheStream::string(archive, file, url);
	if (json.empty()) {
		return entries;
	}
	nlohmann::json jsonResponse = nlohmann::json::parse(json, nullptr, false, true);
	if (!jsonResponse.contains("tree")) {
		const core::String str = jsonResponse.dump().c_str();
		Log::error("Unexpected json data for url: '%s': %s", url.c_str(), str.c_str());
		return entries;
	}
	nlohmann::json treeJson = jsonResponse["tree"];
	Log::debug("Found json for repository %s with %i entries", repository.c_str(), (int)treeJson.size());

	for (const auto &entry : treeJson) {
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
		treeEntry.url = downloadUrl(archive, repository, branch, treeEntry.path);
		entries.push_back(treeEntry);
	}

	if (entries.empty()) {
		Log::warn("No supported entries found for repository %s", repository.c_str());
	} else {
		Log::info("Found %i entries for repository %s", (int)entries.size(), repository.c_str());
	}

	return entries;
}

} // namespace github
} // namespace voxelcollection
