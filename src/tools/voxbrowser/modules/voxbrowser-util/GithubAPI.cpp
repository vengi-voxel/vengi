/**
 * @file
 */

#include "GithubAPI.h"
#include "JsonUtil.h"
#include "core/Log.h"
#include "http/Request.h"
#include "io/BufferedReadWriteStream.h"

namespace github {

core::DynamicArray<TreeEntry> reposGitTrees(const core::String &repository, const core::String &branch) {
	const core::String url = "https://api.github.com/repos/" + repository + "/git/trees/" + branch + "?recursive=1";
	http::Request request(url, http::RequestType::GET);
	io::BufferedReadWriteStream outStream;
	core::DynamicArray<TreeEntry> entries;
	int statusCode = 0;
	if (!request.execute(outStream, &statusCode)) {
		Log::error("Failed to query github api with %i", statusCode);
		return entries;
	}
	core::String json;
	outStream.seek(0);
	outStream.readString(outStream.size(), json);
	nlohmann::json jsonResponse = nlohmann::json::parse(json);
	if (!jsonResponse.contains("tree")) {
		const core::String str = jsonResponse.dump().c_str();
		Log::error("Unexpected json data for url: '%s': %s (status: %i)", url.c_str(), str.c_str(), statusCode);
		return entries;
	}
	jsonResponse = jsonResponse["tree"];
	for (auto &entry : jsonResponse) {
		TreeEntry treeEntry;
		treeEntry.path = get(entry, "path");
		treeEntry.mode = get(entry, "mode");
		treeEntry.type = get(entry, "type");
		treeEntry.sha = get(entry, "sha");
		treeEntry.url = "https://raw.githubusercontent.com/" + repository + "/" + branch + "/" + treeEntry.path;
		treeEntry.size = getInt(entry, "size");
		entries.push_back(treeEntry);
	}

	return entries;
}

} // namespace github
