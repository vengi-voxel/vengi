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

static nlohmann::json cachedJson(const io::ArchivePtr &archive, const core::String &file, const core::String &url) {
	const core::String json = http::HttpCacheStream::string(archive, file, url);
	if (json.empty()) {
		return {};
	}
	return nlohmann::json::parse(json, nullptr, false, true);
}

core::String downloadUrl(const io::ArchivePtr &archive, const core::String &repository, const core::String &branch,
						 const core::String &path, int size) {
	// the git lfs stuff is usually not larger than 150 bytes because it's just a reference
	if (size < 150) {
		core_assert(!path.empty());
		const core::String &branchEnc = core::string::urlPathEncode(branch);
		const core::String &pathEnc = core::string::urlPathEncode(path);
		const core::String &url = "https://api.github.com/repos/" + repository + "/contents/" + pathEnc + "?ref=" + branchEnc;
		core::String file = "github-" + repository + "-" + branch + "-" + path + ".json";
		core::string::replaceAllChars(file, '/', '-');
		const auto &json = cachedJson(archive, file, url);
		if (json.contains("download_url")) {
			const auto &dlurl = json.value("download_url", "");
			return dlurl.c_str();
		}
		const core::String &str = json.dump().c_str();
		Log::debug("Unexpected json data for url: '%s': %s", url.c_str(), str.c_str());
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
	const auto &json = cachedJson(archive, file, url);
	if (!json.contains("tree")) {
		const core::String str = json.dump().c_str();
		Log::error("Unexpected json data for url: '%s': %s", url.c_str(), str.c_str());
		return entries;
	}
	const nlohmann::json &treeJson = json["tree"];
	Log::debug("Found json for repository %s with %i entries", repository.c_str(), (int)treeJson.size());

	for (const auto &entry : treeJson) {
		const auto &type = entry.value("type", "");
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
		treeEntry.size = entry.value("size", -1);
		treeEntry.url = downloadUrl(archive, repository, branch, treeEntry.path, treeEntry.size);
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
