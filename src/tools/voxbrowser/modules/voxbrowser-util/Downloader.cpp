/**
 * @file
 */

#include "Downloader.h"
#include "JsonUtil.h"
#include "app/App.h"
#include "core/Log.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "engine-config.h"
#include "http/HttpCacheStream.h"
#include "http/Request.h"
#include "io/Archive.h"
#include "io/BufferedReadWriteStream.h"
#include "io/FileStream.h"
#include "io/FormatDescription.h"
#include "voxbrowser-util/GithubAPI.h"
#include "voxelformat/VolumeFormat.h"

namespace voxbrowser {

core::String VoxelFile::targetFile() const {
	return core::string::path(core::string::cleanPath(source), name);
}

core::String VoxelFile::targetDir() const {
	return core::string::path(core::string::cleanPath(source), core::string::extractPath(name));
}

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
	return io::isA(path, voxelformat::voxelLoad());
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

void Downloader::handleArchive(const VoxelFile &archiveFile, core::DynamicArray<VoxelFile> &files) const {
	http::HttpCacheStream stream(io::filesystem(), archiveFile.targetFile(), archiveFile.url);
	io::ArchivePtr archive = io::openArchive(io::filesystem(), archiveFile.fullPath, &stream);
	if (!archive) {
		Log::error("Failed to open archive %s", archiveFile.targetFile().c_str());
		return;
	}
	Log::info("Found %i files in archive %s", (int)archive->files().size(), archiveFile.name.c_str());
	for (const io::FilesystemEntry &file : archive->files()) {
		VoxelFile subFile;
		subFile.source = archiveFile.source;
		subFile.name = file.fullPath;
		subFile.license = archiveFile.license;
		subFile.licenseUrl = archiveFile.licenseUrl;
		subFile.thumbnailUrl = archiveFile.thumbnailUrl;
		subFile.fullPath = io::filesystem()->writePath(subFile.targetFile());
		const core::String &archiveFileName = core::string::path(archiveFile.targetDir(), file.fullPath);

		if (io::isSupportedArchive(file.name)) {
			// save file and call handleArchive again
			io::SeekableReadStreamPtr rs = archive->readStream(file.fullPath);
			if (!rs) {
				Log::error("Failed to read file %s", file.fullPath.c_str());
				continue;
			}
			if (io::filesystem()->write(archiveFileName, *rs.get())) {
				handleArchive(subFile, files);
			} else {
				Log::error("Failed to write file %s", file.fullPath.c_str());
			}
			continue;
		}
		if (!supportedFileExtension(file.name)) {
			continue;
		}

		Log::debug("Found %s in archive %s", file.name.c_str(), archiveFile.targetFile().c_str());
		if (io::filesystem()->exists(archiveFileName)) {
			files.push_back(subFile);
			continue;
		}
		io::SeekableReadStreamPtr rs = archive->readStream(file.fullPath);
		if (!rs) {
			Log::error("Failed to read file %s", file.fullPath.c_str());
			continue;
		}
		if (io::filesystem()->write(archiveFileName, *rs.get())) {
			files.push_back(subFile);
		} else {
			Log::error("Failed to write file %s", file.name.c_str());
		}
	}
}

core::DynamicArray<VoxelFile> Downloader::resolve(const VoxelSource &source) const {
	core::DynamicArray<VoxelFile> files;
	Log::info("... check source %s", source.name.c_str());
	if (source.provider == "github") {
		const core::DynamicArray<github::TreeEntry> &entries =
			github::reposGitTrees(source.github.repo, source.github.commit);
		const core::String &licenseDownloadUrl =
			github::downloadUrl(source.github.repo, source.github.commit, source.github.license);
		for (auto &entry : entries) {
			VoxelFile file;
			file.source = source.name;
			file.name = entry.path;
			file.license = source.license;
			if (!source.github.license.empty()) {
				file.licenseUrl = licenseDownloadUrl;
			}
			file.thumbnailUrl = findThumbnailUrl(entries, entry, source);
			file.url = entry.url;
			file.fullPath = io::filesystem()->writePath(file.targetFile());

			if (io::isSupportedArchive(file.name)) {
				handleArchive(file, files);
				continue;
			}

			if (!supportedFileExtension(entry.path)) {
				continue;
			}

			// TODO: allow to enable mesh formats?
			if (voxelformat::isMeshFormat(entry.path, false)) {
				continue;
			}
			files.push_back(file);
		}
	} else if (source.provider == "single") {
		VoxelFile file;
		file.source = source.name;
		file.name = core::string::extractFilenameWithExtension(source.single.url);
		file.license = source.license;
		file.thumbnailUrl = source.thumbnail;
		file.url = source.single.url;
		file.fullPath = io::filesystem()->writePath(file.targetFile());
		Log::info("Found single source with name %s and url %s", file.name.c_str(), file.url.c_str());
		if (io::isSupportedArchive(file.name)) {
			handleArchive(file, files);
		} else {
			files.push_back(file);
		}
	}

	return files;
}

}; // namespace voxbrowser
