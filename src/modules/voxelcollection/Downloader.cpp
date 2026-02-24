/**
 * @file
 */

#include "Downloader.h"
#include "GithubAPI.h"
#include "GitlabAPI.h"
#include "app/App.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "core/concurrent/Atomic.h"
#include "engine-config.h"
#include "http/HttpCacheStream.h"
#include "http/Request.h"
#include "io/Archive.h"
#include "io/BufferedReadWriteStream.h"
#include "io/FormatDescription.h"
#include "io/ZipArchive.h"
#include "voxelformat/VolumeFormat.h"
#include "json/JSON.h"

namespace voxelcollection {

core::String VoxelFile::targetFile() const {
	if (isLocal()) {
		return fullPath;
	}
	const core::String cleanSource = core::string::cleanPath(source);
	return core::string::path(cleanSource, fullPath);
}

core::String VoxelFile::id() const {
	core::String copy = fullPath;
	core::string::replaceAllChars(copy, '/', '-');
	return copy;
}

core::String VoxelFile::targetDir() const {
	if (isLocal()) {
		return core::string::sanitizeDirPath(core::string::extractDir(fullPath));
	}
	const core::String cleanSource = core::string::cleanPath(source);
	const core::String path = core::string::path(cleanSource, core::string::extractDir(fullPath));
	return core::string::sanitizeDirPath(path);
}

VoxelSources Downloader::sources() {
	http::Request request("https://vengi-voxel.de/api/browser-data", http::RequestType::GET);
	const core::String userAgent = app::App::getInstance()->fullAppname() + "/" PROJECT_VERSION;
	request.setUserAgent(userAgent);
	request.setConnectTimeoutSecond(core::Var::registerVar(core::VarDef(cfg::HttpConnectTimeoutAssets, 10)));
	request.setTimeoutSecond(core::Var::registerVar(core::VarDef(cfg::HttpTimeoutAssets, 10)));
	core::String json;
	{
		io::BufferedReadWriteStream outStream;
		if (!request.execute(outStream)) {
			Log::error("Failed to download browser data");
			return {};
		}
		outStream.seek(0);
		outStream.readString((int)outStream.size(), json);
	}
	return sources(json);
}

VoxelSources Downloader::sources(const core::String &json) {
	VoxelSources voxelSources;
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
			source.github.enableMeshes = githubNode.value("enableMeshes", false);
			// the github license is a file in the repo, so we need to query the tree for it
			// and download it
			source.github.license = githubNode.value("license", "").c_str();
		} else if (entry.contains("gitlab")) {
			source.provider = "gitlab";
			const auto &gitlabNode = entry["gitlab"];
			source.gitlab.repo = gitlabNode.value("repo", "").c_str();
			source.gitlab.commit = gitlabNode.value("commit", "").c_str();
			source.gitlab.path = gitlabNode.value("path", "").c_str();
			source.gitlab.enableMeshes = gitlabNode.value("enableMeshes", false);
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

static core::String findThumbnailUrl(const io::ArchivePtr &archive, const core::DynamicArray<github::TreeEntry> &entries, const github::TreeEntry &current,
									 const VoxelSource &source) {
	const core::String &pathNoExt = core::string::stripExtension(current.path);
	for (const github::TreeEntry &entry : entries) {
		for (const io::FormatDescription *desc = io::format::images(); desc->valid(); ++desc) {
			for (const core::String &ext : desc->exts) {
				if (entry.path == current.path + "." + ext || entry.path == pathNoExt + "." + ext) {
					return github::downloadUrl(archive, source.github.repo, source.github.commit, entry.path, entry.size);
				}
			}
		}
	}
	return core::String::Empty;
}

static core::String findThumbnailUrl(const core::DynamicArray<gitlab::TreeEntry> &entries, const gitlab::TreeEntry &current,
									 const VoxelSource &source) {
	const core::String &pathNoExt = core::string::stripExtension(current.path);
	for (const gitlab::TreeEntry &entry : entries) {
		for (const io::FormatDescription *desc = io::format::images(); desc->valid(); ++desc) {
			for (const core::String &ext : desc->exts) {
				if (entry.path == current.path + "." + ext || entry.path == pathNoExt + "." + ext) {
					return gitlab::downloadUrl(source.gitlab.repo, source.gitlab.commit, entry.path);
				}
			}
		}
	}
	return core::String::Empty;
}

bool Downloader::handleArchive(const io::ArchivePtr &archive, const VoxelFile &archiveFile,
							   core::DynamicArray<VoxelFile> &files) const {
	http::HttpCacheStream stream(archive, archiveFile.targetFile(), archiveFile.url);
	io::ArchivePtr zipArchive = io::openZipArchive(&stream);
	if (!zipArchive) {
		Log::error("Failed to open zip archive %s", archiveFile.targetFile().c_str());
		return false;
	}
	Log::debug("Found %i files in zip archive %s", (int)zipArchive->files().size(), archiveFile.name.c_str());
	for (const io::FilesystemEntry &f : zipArchive->files()) {
		VoxelFile subFile;
		subFile.source = archiveFile.source;
		subFile.name = f.name;
		subFile.license = archiveFile.license;
		subFile.licenseUrl = archiveFile.licenseUrl;
		subFile.thumbnailUrl = archiveFile.thumbnailUrl;
		subFile.fullPath = core::string::path(core::string::extractDir(archiveFile.fullPath), core::string::extractFilename(archiveFile.fullPath), f.fullPath);
		subFile.downloaded = true;

		if (supportedFileExtension(subFile.name)) {
			Log::debug("Found %s in archive %s", subFile.name.c_str(), archiveFile.targetFile().c_str());
			if (archive->exists(subFile.fullPath)) {
				files.push_back(subFile);
				continue;
			}
			core::ScopedPtr<io::SeekableReadStream> rs(zipArchive->readStream(f.fullPath));
			if (!rs) {
				Log::error("Failed to read file %s from archive %s", f.fullPath.c_str(),
						   archiveFile.fullPath.c_str());
				continue;
			}
			if (archive->write(subFile.targetFile(), *rs)) {
				files.push_back(subFile);
			} else {
				Log::error("Failed to write file %s from archive %s", subFile.name.c_str(),
						   archiveFile.fullPath.c_str());
			}
		} else if (io::isZipArchive(subFile.name)) {
			// save file and call handleArchive again
			core::ScopedPtr<io::SeekableReadStream> rs(zipArchive->readStream(f.fullPath));
			if (!rs) {
				Log::error("Failed to read file %s from archive %s", f.fullPath.c_str(),
						   archiveFile.fullPath.c_str());
				continue;
			}
			if (archive->write(subFile.targetFile(), *rs)) {
				handleArchive(archive, subFile, files);
			} else {
				Log::error("Failed to write file %s from archive %s", subFile.fullPath.c_str(),
						   archiveFile.fullPath.c_str());
			}
		}
	}
	return true;
}

bool Downloader::download(const io::ArchivePtr &archive, const VoxelFile &file) const {
	http::HttpCacheStream stream(archive, file.targetFile(), file.url);
	if (stream.isNewInCache()) {
		Log::info("Downloaded %s", file.targetFile().c_str());
		return true;
	}
	return stream.valid();
}

bool Downloader::handleFile(const io::ArchivePtr &archive, VoxelFiles &files,
							VoxelFile &file, bool enableMeshes) const {
	if (supportedFileExtension(file.name)) {
		if (!enableMeshes && voxelformat::isMeshFormat(file.name, false)) {
			return false;
		}
		files.push_back(file);
		return true;
	} else if (io::isZipArchive(file.name)) {
		return handleArchive(archive, file, files);
	}
	return false;
}

core::DynamicArray<VoxelFile> Downloader::processEntries(const core::DynamicArray<gitlab::TreeEntry> &entries,
														 const VoxelSource &source, const io::ArchivePtr &archive) const {
	core::DynamicArray<VoxelFile> files;
	const core::String &licenseDownloadUrl =
		gitlab::downloadUrl(source.gitlab.repo, source.gitlab.commit, source.gitlab.license);
	for (const auto &entry : entries) {
		VoxelFile file;
		file.source = source.name;
		file.name = core::string::extractFilenameWithExtension(entry.path);
		file.license = source.license;
		if (!source.gitlab.license.empty()) {
			file.licenseUrl = licenseDownloadUrl;
		}
		file.thumbnailUrl = findThumbnailUrl(entries, entry, source);
		file.url = entry.url;
		file.fullPath = entry.path;
		handleFile(archive, files, file, false);
	}
	return files;
}

core::DynamicArray<VoxelFile> Downloader::processEntries(const core::DynamicArray<github::TreeEntry> &entries,
														 const VoxelSource &source, const io::ArchivePtr &archive) const {
	core::DynamicArray<VoxelFile> files;
	const core::String &licenseDownloadUrl = source.github.license.empty() ? core::String::Empty :
		github::downloadUrl(archive, source.github.repo, source.github.commit, source.github.license, 0);
	const core::String cleanSource = core::string::cleanPath(source.name);
	for (const auto &entry : entries) {
		VoxelFile file;
		file.source = source.name;
		file.name = core::string::extractFilenameWithExtension(entry.path);
		file.license = source.license;
		file.licenseUrl = licenseDownloadUrl;
		file.thumbnailUrl = findThumbnailUrl(archive, entries, entry, source);
		file.url = entry.url;
		file.fullPath = entry.path;
		handleFile(archive, files, file, source.github.enableMeshes);
	}
	return files;
}

VoxelFiles Downloader::resolve(const io::ArchivePtr &archive, const VoxelSource &source) const {
	Log::info("... check source %s", source.name.c_str());
	if (source.provider == "github") {
		const core::DynamicArray<github::TreeEntry> &entries =
			github::reposGitTrees(archive, source.github.repo, source.github.commit, source.github.path);
		return processEntries(entries, source, archive);
	} else if (source.provider == "gitlab") {
		const core::DynamicArray<gitlab::TreeEntry> &entries =
			gitlab::reposGitTrees(archive, source.gitlab.repo, source.gitlab.commit, source.gitlab.path);
		return processEntries(entries, source, archive);
	} else if (source.provider == "single") {
		VoxelFiles files;
		VoxelFile file;
		file.source = source.name;
		file.name = core::string::extractFilenameWithExtension(source.single.url);
		file.license = source.license;
		file.thumbnailUrl = source.thumbnail;
		file.url = source.single.url;
		file.fullPath = file.name;
		Log::info("Found single source with name %s and url %s", file.name.c_str(), file.url.c_str());
		if (!handleFile(archive, files, file, true)) {
			files.push_back(file);
		}
		return files;
	}
	Log::error("Unknown source provider %s", source.provider.c_str());
	return {};
}

}; // namespace voxelcollection
