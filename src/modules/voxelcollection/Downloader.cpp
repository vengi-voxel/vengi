/**
 * @file
 */

#include "Downloader.h"
#include "GithubAPI.h"
#include "GitlabAPI.h"
#include "app/App.h"
#include "core/Log.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "core/concurrent/Atomic.h"
#include "engine-config.h"
#include "http/HttpCacheStream.h"
#include "http/Request.h"
#include "io/Archive.h"
#include "io/BufferedReadWriteStream.h"
#include "io/Filesystem.h"
#include "io/FormatDescription.h"
#include "voxelformat/VolumeFormat.h"
#include <json.hpp>

namespace voxelcollection {

core::String VoxelFile::targetFile() const {
	return core::string::path(core::string::cleanPath(source), name);
}

core::String VoxelFile::targetDir() const {
	return core::string::path(core::string::cleanPath(source), core::string::extractPath(name));
}

core::DynamicArray<VoxelSource> Downloader::sources() {
	http::Request request("https://vengi-voxel.de/api/browser-data", http::RequestType::GET);
	const core::String userAgent = app::App::getInstance()->fullAppname() + "/" PROJECT_VERSION;
	request.setUserAgent(userAgent);
	request.setConnectTimeoutSecond(core::Var::get("vb_connect_timeout", 10));
	request.setTimeoutSecond(core::Var::get("vb_timeout", 10));
	core::String json;
	{
		io::BufferedReadWriteStream outStream;
		if (!request.execute(outStream)) {
			Log::error("Failed to download browser data");
			return {};
		}
		outStream.seek(0);
		outStream.readString(outStream.size(), json);
	}
	return sources(json);
}

core::DynamicArray<VoxelSource> Downloader::sources(const core::String &json) {
	core::DynamicArray<VoxelSource> voxelSources;
	nlohmann::json jsonResponse = nlohmann::json::parse(json, nullptr, false, true);
	if (!jsonResponse.contains("sources")) {
		Log::error("Unexpected json data");
		return voxelSources;
	}

	jsonResponse = jsonResponse["sources"];
	for (auto &entry : jsonResponse) {
		VoxelSource source;
		source.name = entry.value("name", "").c_str();
		source.license = entry.value("license", "").c_str();
		source.thumbnail = entry.value("thumbnail", "").c_str();
		if (entry.contains("github")) {
			source.provider = "github";
			const auto &githubNode = entry["github"];
			source.github.repo = githubNode.value("repo", "").c_str();
			source.github.commit = githubNode.value("commit", "").c_str();
			source.github.path = githubNode.value("path", "").c_str();
			// the github license is a file in the repo, so we need to query the tree for it
			// and download it
			source.github.license = githubNode.value("license", "").c_str();
		} else if (entry.contains("gitlab")) {
			source.provider = "gitlab";
			const auto &gitlabNode = entry["gitlab"];
			source.gitlab.repo = gitlabNode.value("repo", "").c_str();
			source.gitlab.commit = gitlabNode.value("commit", "").c_str();
			source.gitlab.path = gitlabNode.value("path", "").c_str();
			// the gitlab license is a file in the repo, so we need to query the tree for it
			// and download it
			source.gitlab.license = gitlabNode.value("license", "").c_str();
		} else if (entry.contains("single")) {
			source.provider = "single";
			source.single.url = entry["single"].value("url", "").c_str();
		}
		voxelSources.push_back(source);
	}
	return voxelSources;
}

static bool supportedFileExtension(const core::String &path) {
	return io::isA(path, voxelformat::voxelLoad());
}

template<class Entry>
static core::String findThumbnailUrl(const core::DynamicArray<Entry> &entries, const Entry &current,
									 const VoxelSource &source, core::AtomicBool &shouldQuit) {
	const core::String &pathNoExt = core::string::stripExtension(current.path);
	for (auto &entry : entries) {
		for (const io::FormatDescription *desc = io::format::images(); desc->valid(); ++desc) {
			for (const core::String &ext : desc->exts) {
				if (shouldQuit) {
					return "";
				}
				if (entry.path == current.path + "." + ext || entry.path == pathNoExt + "." + ext) {
					return github::downloadUrl(source.github.repo, source.github.commit, entry.path);
				}
			}
		}
	}
	return "";
}

void Downloader::handleArchive(const io::FilesystemPtr &filesystem, const VoxelFile &archiveFile,
							   core::DynamicArray<VoxelFile> &files, core::AtomicBool &shouldQuit) const {
	http::HttpCacheStream stream(filesystem, archiveFile.targetFile(), archiveFile.url);
	io::ArchivePtr archive = io::openArchive(filesystem, archiveFile.fullPath, &stream);
	if (!archive) {
		Log::error("Failed to open archive %s", archiveFile.targetFile().c_str());
		return;
	}
	Log::info("Found %i files in archive %s", (int)archive->files().size(), archiveFile.name.c_str());
	for (const io::FilesystemEntry &file : archive->files()) {
		if (shouldQuit) {
			return;
		}
		VoxelFile subFile;
		subFile.source = archiveFile.source;
		subFile.name = file.fullPath;
		subFile.license = archiveFile.license;
		subFile.licenseUrl = archiveFile.licenseUrl;
		subFile.thumbnailUrl = archiveFile.thumbnailUrl;
		subFile.fullPath = filesystem->writePath(subFile.targetFile());
		const core::String &archiveFileName = core::string::path(archiveFile.targetDir(), file.fullPath);

		if (io::isSupportedArchive(file.name)) {
			// save file and call handleArchive again
			io::SeekableReadStreamPtr rs = archive->readStream(file.fullPath);
			if (!rs) {
				Log::error("Failed to read file %s", file.fullPath.c_str());
				continue;
			}
			if (filesystem->write(archiveFileName, *rs.get())) {
				handleArchive(filesystem, subFile, files, shouldQuit);
			} else {
				Log::error("Failed to write file %s", file.fullPath.c_str());
			}
			continue;
		}
		if (!supportedFileExtension(file.name)) {
			continue;
		}

		Log::debug("Found %s in archive %s", file.name.c_str(), archiveFile.targetFile().c_str());
		if (filesystem->exists(archiveFileName)) {
			files.push_back(subFile);
			continue;
		}
		io::SeekableReadStreamPtr rs = archive->readStream(file.fullPath);
		if (!rs) {
			Log::error("Failed to read file %s", file.fullPath.c_str());
			continue;
		}
		if (filesystem->write(archiveFileName, *rs.get())) {
			files.push_back(subFile);
		} else {
			Log::error("Failed to write file %s", file.name.c_str());
		}
	}
}

bool Downloader::download(const io::FilesystemPtr &filesystem, const VoxelFile &file) const {
	http::HttpCacheStream stream(filesystem, file.targetFile(), file.url);
	if (stream.isNewInCache()) {
		Log::info("Downloaded %s", file.targetFile().c_str());
		return true;
	}
	return stream.valid();
}

VoxelFiles Downloader::resolve(const io::FilesystemPtr &filesystem, const VoxelSource &source,
							   core::AtomicBool &shouldQuit) const {
	VoxelFiles files;
	Log::info("... check source %s", source.name.c_str());
	if (source.provider == "github") {
		const core::DynamicArray<github::TreeEntry> &entries =
			github::reposGitTrees(filesystem, source.github.repo, source.github.commit, source.github.path);
		const core::String &licenseDownloadUrl =
			github::downloadUrl(source.github.repo, source.github.commit, source.github.license);
		for (const auto &entry : entries) {
			if (shouldQuit) {
				return {};
			}
			VoxelFile file;
			file.source = source.name;
			file.name = entry.path;
			file.license = source.license;
			if (!source.github.license.empty()) {
				file.licenseUrl = licenseDownloadUrl;
			}
			file.thumbnailUrl = findThumbnailUrl(entries, entry, source, shouldQuit);
			file.url = entry.url;
			file.fullPath = filesystem->writePath(file.targetFile());

			if (io::isSupportedArchive(file.name)) {
				handleArchive(filesystem, file, files, shouldQuit);
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
	} else if (source.provider == "gitlab") {
		const core::DynamicArray<gitlab::TreeEntry> &entries =
			gitlab::reposGitTrees(filesystem, source.gitlab.repo, source.gitlab.commit, source.gitlab.path);
		const core::String &licenseDownloadUrl =
			gitlab::downloadUrl(source.gitlab.repo, source.gitlab.commit, source.gitlab.license);
		for (const auto &entry : entries) {
			if (shouldQuit) {
				return {};
			}
			VoxelFile file;
			file.source = source.name;
			file.name = entry.path;
			file.license = source.license;
			if (!source.gitlab.license.empty()) {
				file.licenseUrl = licenseDownloadUrl;
			}
			file.thumbnailUrl = findThumbnailUrl(entries, entry, source, shouldQuit);
			file.url = entry.url;
			file.fullPath = filesystem->writePath(file.targetFile());

			if (io::isSupportedArchive(file.name)) {
				handleArchive(filesystem, file, files, shouldQuit);
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
		file.fullPath = filesystem->writePath(file.targetFile());
		Log::info("Found single source with name %s and url %s", file.name.c_str(), file.url.c_str());
		if (io::isSupportedArchive(file.name)) {
			handleArchive(filesystem, file, files, shouldQuit);
		} else {
			files.push_back(file);
		}
	}

	return files;
}

}; // namespace voxelcollection
