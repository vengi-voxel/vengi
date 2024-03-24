/**
 * @file
 */

#include "CollectionManager.h"
#include "app/Async.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "http/HttpCacheStream.h"
#include "voxelcollection/Downloader.h"
#include "voxelformat/VolumeFormat.h"

namespace voxelcollection {

CollectionManager::CollectionManager(const io::FilesystemPtr &filesystem, const video::TexturePoolPtr &texturePool)
	: _filesystem(filesystem), _texturePool(texturePool) {
}

CollectionManager::~CollectionManager() {
}

bool CollectionManager::init() {
	return true;
}

void CollectionManager::shutdown() {
}

void CollectionManager::local() {
	app::async([&]() {
		core::DynamicArray<io::FilesystemEntry> entities;
		const core::String docs = _filesystem->specialDir(io::FilesystemDirectories::FS_Dir_Documents);
		Log::info("Local document scanning (%s)...", docs.c_str());
		_filesystem->list(docs, entities, "", 2);

		for (const io::FilesystemEntry &entry : entities) {
			if (!io::isA(entry.name, voxelformat::voxelLoad())) {
				continue;
			}
			voxelcollection::VoxelFile voxelFile;
			voxelFile.name = entry.fullPath.substr(docs.size());
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
}

void CollectionManager::online() {
	app::async([&]() {
		voxelcollection::Downloader downloader;
		auto sources = downloader.sources();
		Log::info("Found %d online sources", (int)sources.size());
		for (const voxelcollection::VoxelSource &source : sources) {
			const core::DynamicArray<voxelcollection::VoxelFile> &files = downloader.resolve(_filesystem, source);
			_newVoxelFiles.push(files.begin(), files.end());
		}
	});
}

void CollectionManager::thumbnailAll() {
	for (const auto &e : _voxelFilesMap) {
		for (const voxelcollection::VoxelFile &voxelFile : e->value.files) {
			loadThumbnail(voxelFile);
		}
	}
}

void CollectionManager::loadThumbnail(const voxelcollection::VoxelFile &voxelFile) {
	if (_texturePool->has(voxelFile.name)) {
		return;
	}
	const core::String &targetImageFile = _filesystem->writePath(voxelFile.targetFile() + ".png");
	if (_filesystem->exists(targetImageFile)) {
		app::async([this, voxelFile, targetImageFile]() {
			image::ImagePtr image = image::loadImage(targetImageFile);
			if (image) {
				image->setName(voxelFile.name);
				_imageQueue.push(image);
			}
		});
		return;
	}
	if (!_filesystem->createDir(core::string::extractPath(targetImageFile))) {
		Log::warn("Failed to create directory for thumbnails at: %s", voxelFile.targetDir().c_str());
		return;
	}
	if (!voxelFile.thumbnailUrl.empty()) {
		app::async([=]() {
			http::HttpCacheStream stream(_filesystem, voxelFile.targetFile() + ".png", voxelFile.thumbnailUrl);
			this->_imageQueue.push(image::loadImage(voxelFile.name, stream));
		});
	} else {
		app::async([=]() {
			http::HttpCacheStream stream(_filesystem, voxelFile.fullPath, voxelFile.url);
			voxelformat::LoadContext loadCtx;
			image::ImagePtr thumbnailImage = voxelformat::loadScreenshot(voxelFile.fullPath, stream, loadCtx);
			if (!thumbnailImage || !thumbnailImage->isLoaded()) {
				Log::debug("Failed to load given input file: %s", voxelFile.fullPath.c_str());
				return;
			}
			thumbnailImage->setName(voxelFile.name);
			if (!image::writeImage(thumbnailImage, targetImageFile)) {
				Log::warn("Failed to save thumbnail for %s to %s", voxelFile.name.c_str(), targetImageFile.c_str());
			} else {
				Log::debug("Created thumbnail for %s at %s", voxelFile.name.c_str(), targetImageFile.c_str());
			}
			this->_imageQueue.push(thumbnailImage);
		});
	}
}

void CollectionManager::update(double nowSeconds, int n) {
	image::ImagePtr image;
	if (_imageQueue.pop(image)) {
		if (image && image->isLoaded()) {
			_texturePool->addImage(image);
		}
	}

	voxelcollection::VoxelFiles voxelFiles;
	_newVoxelFiles.pop(voxelFiles, n);
	if (voxelFiles.empty()) {
		return;
	}

	for (voxelcollection::VoxelFile &voxelFile : voxelFiles) {
		loadThumbnail(voxelFile);
		auto iter = _voxelFilesMap.find(voxelFile.source);
		if (iter != _voxelFilesMap.end()) {
			voxelcollection::VoxelCollection &collection = iter->value;
			collection.files.push_back(voxelFile);
			collection.timestamp = nowSeconds;
			collection.sorted = false;
		} else {
			voxelcollection::VoxelCollection collection{{voxelFile}, nowSeconds, false};
			_voxelFilesMap.put(voxelFile.source, collection);
		}
	}
	for (auto e : _voxelFilesMap) {
		voxelcollection::VoxelCollection &collection = e->value;
		if (collection.sorted) {
			continue;
		}
		if (collection.timestamp + 5.0 > nowSeconds) {
			continue;
		}
		core::sort(collection.files.begin(), collection.files.end(), core::Less<voxelcollection::VoxelFile>());
		collection.sorted = true;
	}
	_count += voxelFiles.size();
}

void CollectionManager::downloadAll() {
	app::async([this, voxelFilesMap = _voxelFilesMap]() {
		int all = 0;
		for (const auto &e : voxelFilesMap) {
			all += (int)e->value.files.size();
		}

		int current = 0;
		for (const auto &e : voxelFilesMap) {
			for (const voxelcollection::VoxelFile &voxelFile : e->value.files) {
				++current;
				if (voxelFile.downloaded) {
					continue;
				}
				voxelcollection::Downloader downloader;
				downloader.download(_filesystem, voxelFile);
				const float p = ((float)current / (float)all * 100.0f);
				_downloadProgress = (int)p;
			}
		}
		_downloadProgress = 0;
	});
}

const voxelcollection::VoxelFileMap &CollectionManager::voxelFilesMap() const {
	return _voxelFilesMap;
}

int CollectionManager::downloadProgress() const {
	return _downloadProgress;
}

int CollectionManager::allEntries() const {
	return _count;
}

} // namespace voxelcollection
