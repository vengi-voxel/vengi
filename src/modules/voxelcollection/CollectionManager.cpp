/**
 * @file
 */

#include "CollectionManager.h"
#include "app/Async.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/SharedPtr.h"
#include "core/Var.h"
#include "http/HttpCacheStream.h"
#include "image/Image.h"
#include "io/Archive.h"
#include "io/FilesystemArchive.h"
#include "io/Stream.h"
#include "voxelcollection/Downloader.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelrender/ImageGenerator.h"

namespace voxelcollection {

CollectionManager::CollectionManager(const io::FilesystemPtr &filesystem, const video::TexturePoolPtr &texturePool)
	: _texturePool(texturePool), _filesystem(filesystem) {
	_archive = io::openFilesystemArchive(filesystem, "", false);
	// make them shared pointers so that the async tasks can still access them
	// no matter if the CollectionManager is destroyed or not
	_newVoxelFiles = core::make_shared<QueuePtr::value_type>();
	_imageQueue = core::make_shared<ImageQueuePtr::value_type>();
	_voxelSourceQueue = core::make_shared<VoxelSourceQueuePtr::value_type>();
}

CollectionManager::~CollectionManager() {
}

bool CollectionManager::init() {
	VoxelSource local;
	local.name = LOCAL_SOURCE;
	_sources.push_back(local);
	core::String documents = _filesystem->sysSpecialDir(io::FilesystemDirectories::FS_Dir_Documents);
	if (documents.empty()) {
		documents = _filesystem->sysSpecialDir(io::FilesystemDirectories::FS_Dir_Public);
	}
	if (documents.empty()) {
		documents = _filesystem->sysSpecialDir(io::FilesystemDirectories::FS_Dir_Download);
	}
	if (documents.empty()) {
		documents = _filesystem->homePath();
	}
	core_assert(!documents.empty());
	const core::VarPtr &var = core::Var::get(cfg::AssetPanelLocalDirectory, documents);
	_localDir = var->strVal();
	if (_localDir.empty()) {
		var->setVal(documents);
	}
	return true;
}

core::String CollectionManager::absolutePath(const VoxelFile &voxelFile) const {
	// this has to match with the http cache stream
	if (voxelFile.isLocal()) {
		return voxelFile.targetFile();
	}
	return _filesystem->homeWritePath(voxelFile.targetFile());
}

bool CollectionManager::setLocalDir(const core::String &dir) {
	if (dir.empty()) {
		return false;
	}
	if (_localDir.empty() || dir != _localDir) {
		Log::debug("change local dir to %s", dir.c_str());
		_localDir = dir;
		_voxelFilesMap.remove(LOCAL_SOURCE);
		core::getVar(cfg::AssetPanelLocalDirectory)->setVal(_localDir);
		return true;
	}
	return false;
}

void CollectionManager::shutdown() {
}

bool CollectionManager::local() {
	const core::String localDir = _localDir;
	if (localDir.empty()) {
		Log::debug("No local dir set");
		return false;
	}
	_voxelFilesMap.remove(LOCAL_SOURCE);

	core::DynamicArray<io::FilesystemEntry> entities;
	Log::info("Local document scanning (%s)...", localDir.c_str());
	_archive->list(localDir, entities, "");
	Log::debug("Found %i entries in %s", (int)entities.size(), localDir.c_str());
	app::for_parallel(0, entities.size(), [entities, localDir, voxelFiles = _newVoxelFiles](int start, int end) {
		for (int i = start; i < end; ++i) {
			const io::FilesystemEntry &entry  = entities[i];
			if (!io::isA(entry.name, voxelformat::voxelLoad())) {
				continue;
			}
			VoxelFile voxelFile;
			voxelFile.name = entry.fullPath.substr(localDir.size());
			voxelFile.fullPath = entry.fullPath;
			voxelFile.url = "file://" + entry.fullPath;
			voxelFile.source = LOCAL_SOURCE;
			voxelFile.license = "unknown";
			// voxelFile.licenseUrl = "";
			// voxelFile.thumbnailUrl = "";
			voxelFile.downloaded = true;
			voxelFiles->push(voxelFile);
		}
	});
	VoxelCollection collection{{}, 0.0, true};
	_voxelFilesMap.put(LOCAL_SOURCE, collection);

	return true;
}

bool CollectionManager::online() {
	app::schedule([queue = _voxelSourceQueue]() {
		Downloader downloader;
		queue->pushAll(downloader.sources());
	});
	return true;
}

void CollectionManager::loadThumbnail(const VoxelFile &voxelFile) {
	if (_texturePool->has(voxelFile.name)) {
		return;
	}
	const core::String &targetImageFile = voxelFile.targetFile() + ".png";
	if (_archive->exists(targetImageFile)) {
		app::schedule([voxelFile, targetImageFile, archive = _archive, imageQueue = _imageQueue]() {
			core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(targetImageFile));
			image::ImagePtr image = image::loadImage(targetImageFile, *stream);
			if (image) {
				image->setName(voxelFile.name);
				imageQueue->push(image);
			}
		});
		return;
	}
	if (!voxelFile.thumbnailUrl.empty()) {
		app::schedule([imageQueue = _imageQueue, archive = _archive, targetImageFile, voxelFile]() {
			http::HttpCacheStream stream(archive, targetImageFile, voxelFile.thumbnailUrl);
			imageQueue->push(image::loadImage(voxelFile.name, stream));
		});
	} else {
		app::schedule([archive = _archive, voxelFile, targetImageFile, imageQueue = _imageQueue]() {
			http::HttpCacheStream stream(archive, voxelFile.targetFile(), voxelFile.url);
			stream.close();
			voxelformat::LoadContext loadCtx;
			image::ImagePtr thumbnailImage = voxelformat::loadScreenshot(voxelFile.targetFile(), archive, loadCtx);
			if (!thumbnailImage || !thumbnailImage->isLoaded()) {
				Log::debug("Failed to load given input file: %s", voxelFile.fullPath.c_str());
				return;
			}
			thumbnailImage->setName(voxelFile.name);
			core::ScopedPtr<io::SeekableWriteStream> imageStream(archive->writeStream(targetImageFile));
			if (!image::writePNG(thumbnailImage, *imageStream)) {
				Log::warn("Failed to save thumbnail for %s to %s", voxelFile.name.c_str(), targetImageFile.c_str());
			} else {
				Log::debug("Created thumbnail for %s at %s", voxelFile.name.c_str(), targetImageFile.c_str());
			}
			imageQueue->push(thumbnailImage);
		});
	}
}

bool CollectionManager::createThumbnail(const VoxelFile &voxelFile) {
	scenegraph::SceneGraph sceneGraph;
	io::FileDescription fileDesc;
	const core::String &fileName = absolutePath(voxelFile);
	fileDesc.set(fileName);
	voxelformat::LoadContext loadctx;
	if (!voxelformat::loadFormat(fileDesc, _archive, sceneGraph, loadctx)) {
		Log::error("Failed to load given input file: %s", fileName.c_str());
		return false;
	}
	voxelformat::ThumbnailContext ctx;
	image::ImagePtr image = voxelrender::volumeThumbnail(sceneGraph, ctx);
	if (!image || !image->isLoaded()) {
		Log::error("Failed to create thumbnail for %s", fileName.c_str());
		return false;
	}
	image->setName(voxelFile.id());
	_imageQueue->push(image);
	const core::String &targetImageFile = voxelFile.targetFile() + ".png";
	core::ScopedPtr<io::SeekableWriteStream> writeStream(_archive->writeStream(targetImageFile));
	if (!writeStream || !image::writePNG(image, *writeStream)) {
		Log::warn("Failed to write thumbnail to %s - no caching", targetImageFile.c_str());
	}
	Log::info("Created thumbnail for %s at %s", fileName.c_str(), targetImageFile.c_str());
	return true;
}

void CollectionManager::resolve(const VoxelSource &source, bool async) {
	if (source.isLocal()) {
		local();
		return;
	}
	if (!_onlineResolvedSources.insert(source.name)) {
		return;
	}
	auto func = [archive = _archive, voxelFiles = _newVoxelFiles, source]() {
		Downloader downloader;
		const VoxelFiles &files = downloader.resolve(archive, source);
		voxelFiles->push(files.begin(), files.end());
	};
	if (async) {
		app::schedule(func);
	} else {
		func();
	}
}

bool CollectionManager::resolved(const VoxelSource &source) const {
	if (source.isLocal()) {
		return _voxelFilesMap.hasKey(LOCAL_SOURCE);
	}
	return _onlineResolvedSources.has(source.name);
}

void CollectionManager::update(double nowSeconds, int n) {
	_voxelSourceQueue->popAll(_sources);

	image::ImagePtr image;
	if (_imageQueue->pop(image)) {
		if (image && image->isLoaded()) {
			_texturePool->addImage(image);
		}
	}

	VoxelFiles voxelFiles;
	_newVoxelFiles->pop(voxelFiles, n);

	for (const VoxelFile &voxelFile : voxelFiles) {
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
		app::sort_parallel(collection.files.begin(), collection.files.end(), core::Less<VoxelFile>());
		collection.sorted = true;
	}
	_count += voxelFiles.size();
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

int CollectionManager::allEntries() const {
	return _count;
}

} // namespace voxelcollection
