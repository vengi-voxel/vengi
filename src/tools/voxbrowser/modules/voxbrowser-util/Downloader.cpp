/**
 * @file
 */

#include "Downloader.h"
#include "JsonUtil.h"
#include "core/Log.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "engine-config.h"
#include "http/HttpCacheStream.h"
#include "http/Request.h"
#include "io/BufferedReadWriteStream.h"
#include "io/FormatDescription.h"
#include "voxbrowser-util/GithubAPI.h"
#include "voxelformat/VolumeFormat.h"

namespace voxbrowser {

core::DynamicArray<VoxelSource> Downloader::sources() {
	core::DynamicArray<VoxelSource> sources;
	http::Request request("https://vengi-voxel.de/api/browser-data", http::RequestType::GET);
	request.setUserAgent("voxbrowser/" PROJECT_VERSION);
	request.setConnectTimeoutSecond(core::Var::get("vb_connect_timeout", 10));
	request.setTimeoutSecond(core::Var::get("vb_timeout", 10));
	core::String json;
	{
		io::BufferedReadWriteStream outStream;
		if (!request.execute(outStream)) {
			Log::error("Failed to download browser data");
			return sources;
		}
		outStream.seek(0);
		outStream.readString(outStream.size(), json);
	}

	nlohmann::json jsonResponse = nlohmann::json::parse(json);
	if (!jsonResponse.contains("sources")) {
		Log::error("Unexpected json data");
		return sources;
	}

	jsonResponse = jsonResponse["sources"];
	for (auto &entry : jsonResponse) {
		VoxelSource source;
		source.name = get(entry, "name");
		source.license = get(entry, "license");
		source.thumbnail = get(entry, "thumbnail");
		if (entry.contains("github")) {
			source.provider = "github";
			source.github.repo = get(entry["github"], "repo");
			source.github.commit = get(entry["github"], "commit");
			// the github license is a file in the repo, so we need to query the tree for it
			// and download it
			source.github.license = get(entry["github"], "license");
		} else if (entry.contains("single")) {
			source.provider = "single";
			source.single.url = get(entry["single"], "url");
		}
		sources.push_back(source);
	}
	return sources;
}

static bool supportedFileExtension(const core::String &path) {
	io::FileDescription fileDescription;
	fileDescription.set(path);
	const io::FormatDescription *desc = io::getDescription(fileDescription, 0, voxelformat::voxelLoad());
	return desc != nullptr && desc->valid();
}

static core::String findThumbnailUrl(const core::DynamicArray<github::TreeEntry> &entries,
									 const github::TreeEntry &current, const VoxelSource &source) {
	const core::String &pathNoExt = core::string::stripExtension(current.path);
	for (auto &entry : entries) {
		if (entry.path == current.path + ".png" || entry.path == pathNoExt + ".png") {
			return github::downloadUrl(source.github.repo, source.github.commit, entry.path);
		}
	}
	return "";
}

core::DynamicArray<VoxelFile> Downloader::resolve(const VoxelSource &source) const {
	core::DynamicArray<VoxelFile> files;
	if (source.provider == "github") {
		const core::DynamicArray<github::TreeEntry> &entries =
			github::reposGitTrees(source.github.repo, source.github.commit);
		const core::String &licenseDownloadUrl =
			github::downloadUrl(source.github.repo, source.github.commit, source.github.license);
		for (auto &entry : entries) {
			if (entry.type != "blob") {
				continue;
			}
			if (!supportedFileExtension(entry.path)) {
				continue;
			}

			// TODO: allow to enable mesh formats?
			if (voxelformat::isMeshFormat(entry.path, false)) {
				continue;
			}
			VoxelFile file;
			file.source = source.name;
			file.name = entry.path;
			file.license = source.license;
			if (!source.github.license.empty()) {
				file.licenseUrl = licenseDownloadUrl;
			}
			file.thumbnailUrl = findThumbnailUrl(entries, entry, source);
			file.url = entry.url;
			files.push_back(file);
		}
	} else if (source.provider == "single") {
		VoxelFile file;
		file.source = source.name;
		file.name = core::string::extractFilenameWithExtension(source.single.url);
		file.license = source.license;
		file.thumbnailUrl = source.thumbnail;
		file.url = source.single.url;
		files.push_back(file);
	}

	return files;
}

}; // namespace voxbrowser
