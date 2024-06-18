/**
 * @file
 */

#include "CollectionManager.h"
#include "app/Async.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "http/HttpCacheStream.h"
#include "io/Archive.h"
#include "io/FilesystemArchive.h"
#include "io/Stream.h"
#include "voxelcollection/Downloader.h"
#include "voxelformat/VolumeFormat.h"

namespace voxelcollection {

CollectionManager::CollectionManager(const io::FilesystemPtr &filesystem, const video::TexturePoolPtr &texturePool)
	: _texturePool(texturePool) {
	_archive = io::openFilesystemArchive(filesystem, "", false);
	_localDir = filesystem->specialDir(io::FilesystemDirectories::FS_Dir_Documents);
}

CollectionManager::~CollectionManager() {
}

bool CollectionManager::init() {
	VoxelSource local;
	local.name = "local";
	_sources.push_back(local);
	return true;
}

bool CollectionManager::setLocalDir(const core::String &dir) {
	if (dir.empty()) {
		return false;
	}
	if (_localDir.empty() || dir != _localDir) {
		_localDir = dir;
		// TODO: clear all local resolved files
		return true;
	}
	return false;
}

void CollectionManager::waitLocal() {
	if (_local.valid()) {
		Log::debug("Wait for local sources to finish");
		_local.wait();
	}
}

void CollectionManager::waitOnline() {
	if (_onlineSources.valid()) {
		Log::debug("Wait for online sources to finish");
		_onlineSources.wait();
	}
}

void CollectionManager::shutdown() {
	_shouldQuit = true;
	waitLocal();
	waitOnline();
	for (std::future<void> &f : _futures) {
		if (f.valid()) {
			f.wait();
		}
	}
	_futures.clear();
}

bool CollectionManager::local() {
	if (_local.valid()) {
		Log::debug("Local already queued");
		return false;
	}
	if (_localDir.empty()) {
		Log::debug("No local dir set");
		return false;
	}
	_local = app::async([&]() {
		if (_shouldQuit) {
			return;
		}
		core::DynamicArray<io::FilesystemEntry> entities;
		Log::info("Local document scanning (%s)...", _localDir.c_str());
		_archive->list(_localDir, entities, "");
		Log::debug("Found %i entries in %s", (int)entities.size(), _localDir.c_str());

		for (const io::FilesystemEntry &entry : entities) {
			if (_shouldQuit) {
				return;
			}
			if (!io::isA(entry.name, voxelformat::voxelLoad())) {
				continue;
			}
			VoxelFile voxelFile;
			voxelFile.name = entry.fullPath.substr(_localDir.size());
			voxelFile.fullPath = entry.fullPath;
			voxelFile.url = "file://" + entry.fullPath;
			voxelFile.source = "local";
			voxelFile.license = "unknown";
			// voxelFile.licenseUrl = "";
			// voxelFile.thumbnailUrl = "";
			voxelFile.downloaded = true;
			_newVoxelFiles.push(voxelFile);
		}
	});
	return true;
}

bool CollectionManager::online() {
	if (_onlineSources.valid()) {
		return false;
	}
	_onlineSources = app::async([&]() {
		Downloader downloader;
		return downloader.sources();
	});
	return true;
}

void CollectionManager::thumbnailAll() {
	for (const auto &e : _voxelFilesMap) {
		for (const VoxelFile &voxelFile : e->value.files) {
			loadThumbnail(voxelFile);
		}
	}
}

void CollectionManager::loadThumbnail(const VoxelFile &voxelFile) {
	if (_texturePool->has(voxelFile.name)) {
		return;
	}
	if (_shouldQuit) {
		return;
	}
	const core::String &targetImageFile = voxelFile.targetFile() + ".png";
	io::ArchivePtr archive = _archive;
	if (_archive->exists(targetImageFile)) {
		_futures.emplace_back(app::async([this, voxelFile, targetImageFile, archive]() {
			if (_shouldQuit) {
				return;
			}
			core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(targetImageFile));
			image::ImagePtr image = image::loadImage(targetImageFile, *stream);
			if (image) {
				image->setName(voxelFile.name);
				_imageQueue.push(image);
			}
		}));
		return;
	}
	if (!voxelFile.thumbnailUrl.empty()) {
		_futures.emplace_back(app::async([=]() {
			if (_shouldQuit) {
				return;
			}
			http::HttpCacheStream stream(_archive, targetImageFile, voxelFile.thumbnailUrl);
			this->_imageQueue.push(image::loadImage(voxelFile.name, stream));
		}));
	} else {
		_futures.emplace_back(app::async([this, archive, voxelFile, targetImageFile]() {
			if (_shouldQuit) {
				return;
			}
			http::HttpCacheStream stream(archive, voxelFile.fullPath, voxelFile.url);
			voxelformat::LoadContext loadCtx;
			image::ImagePtr thumbnailImage = voxelformat::loadScreenshot(voxelFile.fullPath, archive, loadCtx);
			if (!thumbnailImage || !thumbnailImage->isLoaded()) {
				Log::debug("Failed to load given input file: %s", voxelFile.fullPath.c_str());
				return;
			}
			thumbnailImage->setName(voxelFile.name);
			core::ScopedPtr<io::SeekableWriteStream> imageStream(archive->writeStream(targetImageFile));
			if (!image::writeImage(thumbnailImage, *imageStream)) {
				Log::warn("Failed to save thumbnail for %s to %s", voxelFile.name.c_str(), targetImageFile.c_str());
			} else {
				Log::debug("Created thumbnail for %s at %s", voxelFile.name.c_str(), targetImageFile.c_str());
			}
			this->_imageQueue.push(thumbnailImage);
		}));
	}
}

void CollectionManager::resolve(const VoxelSource &source, bool async) {
	if (!_onlineResolvedSources.insert(source.name)) {
		return;
	}
	io::ArchivePtr archive = _archive;
	auto func = [&, archive]() {
		Downloader downloader;
		if (_shouldQuit) {
			return;
		}
		const VoxelFiles &files = downloader.resolve(archive, source, _shouldQuit);
		_newVoxelFiles.push(files.begin(), files.end());
	};
	if (async) {
		_futures.emplace_back(app::async(func));
	} else {
		func();
	}
}

bool CollectionManager::resolved(const VoxelSource &source) const {
	if (source.name == "local") {
		return _local.valid();
	}
	return _onlineResolvedSources.has(source.name);
}

void CollectionManager::update(double nowSeconds, int n) {
	if (app::App::getInstance()->shouldQuit()) {
		_shouldQuit = true;
	}

	if (_onlineSources.valid() && _onlineSources.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
		_sources.append(_onlineSources.get());
		_onlineSources = {};
	}

	image::ImagePtr image;
	if (_imageQueue.pop(image)) {
		if (image && image->isLoaded()) {
			_texturePool->addImage(image);
		}
	}

	VoxelFiles voxelFiles;
	_newVoxelFiles.pop(voxelFiles, n);

	for (VoxelFile &voxelFile : voxelFiles) {
		loadThumbnail(voxelFile);
		auto iter = _voxelFilesMap.find(voxelFile.source);
		if (iter != _voxelFilesMap.end()) {
			VoxelCollection &collection = iter->value;
			collection.files.push_back(voxelFile);
			collection.timestamp = nowSeconds;
			collection.sorted = false;
		} else {
			VoxelCollection collection{{voxelFile}, nowSeconds, true};
			_voxelFilesMap.put(voxelFile.source, collection);
		}
	}
	for (auto e : _voxelFilesMap) {
		VoxelCollection &collection = e->value;
		if (collection.sorted) {
			continue;
		}
		if (collection.timestamp + 5.0 > nowSeconds) {
			continue;
		}
		core::sort(collection.files.begin(), collection.files.end(), core::Less<VoxelFile>());
		collection.sorted = true;
	}
	_count += voxelFiles.size();
}

void CollectionManager::downloadAll() {
	const io::ArchivePtr archive = _archive;
	app::async([this, voxelFilesMap = _voxelFilesMap, archive]() {
		int all = 0;
		for (const auto &e : voxelFilesMap) {
			all += (int)e->value.files.size();
		}

		int current = 0;
		for (const auto &e : voxelFilesMap) {
			for (VoxelFile &voxelFile : e->value.files) {
				if (_shouldQuit) {
					return;
				}
				++current;
				if (voxelFile.downloaded) {
					continue;
				}
				download(archive, voxelFile);
				const float p = ((float)current / (float)all * 100.0f);
				_downloadProgress = (int)p;
			}
		}
		_downloadProgress = 0;
	});
}

bool CollectionManager::download(const io::ArchivePtr &archive, VoxelFile &voxelFile) {
	Downloader downloader;
	if (downloader.download(archive, voxelFile)) {
		voxelFile.downloaded = true;
		return true;
	}
	return false;
}

bool CollectionManager::download(VoxelFile &voxelFile) {
	return download(_archive, voxelFile);
}

const VoxelFileMap &CollectionManager::voxelFilesMap() const {
	return _voxelFilesMap;
}

int CollectionManager::downloadProgress() const {
	return _downloadProgress;
}

int CollectionManager::allEntries() const {
	return _count;
}

} // namespace voxelcollection
