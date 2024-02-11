/**
 * @file
 */

#include "VoxBrowser.h"
#include "app/App.h"
#include "core/BindingContext.h"
#include "core/Log.h"
#include "core/TimeProvider.h"
#include "core/collection/DynamicArray.h"
#include "core/concurrent/Concurrency.h"
#include "core/concurrent/ThreadPool.h"
#include "http/HttpCacheStream.h"
#include "image/Image.h"
#include "io/Archive.h"
#include "io/File.h"
#include "io/Filesystem.h"

#include "io/FilesystemEntry.h"
#include "voxbrowser-ui/MainWindow.h"
#include "voxbrowser-util/Downloader.h"
#include "voxelformat/Format.h"
#include "voxelformat/FormatConfig.h"
#include "voxelformat/VolumeFormat.h"

VoxBrowser::VoxBrowser(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider)
	: Super(filesystem, timeProvider, core::halfcpus()) {
	init(ORGANISATION, "voxbrowser");
}

app::AppState VoxBrowser::onCleanup() {
	if (_mainWindow) {
		_mainWindow->shutdown();
		delete _mainWindow;
	}
	_texturePool.shutdown();
	return Super::onCleanup();
}

app::AppState VoxBrowser::onConstruct() {
	const app::AppState state = Super::onConstruct();
	_framesPerSecondsCap->setVal(60.0f);

	voxelformat::FormatConfig::init();

	return state;
}

app::AppState VoxBrowser::onInit() {
	const app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}

	if (!_texturePool.init()) {
		Log::error("Failed to initialize the texture pool");
		return app::AppState::InitFailure;
	}

	_mainWindow = new voxbrowser::MainWindow(this, _texturePool);
	if (!_mainWindow->init()) {
		Log::error("Failed to initialize the main window");
		return app::AppState::InitFailure;
	}

	_threadPool->enqueue([&]() {
		core::DynamicArray<io::FilesystemEntry> entities;
		const core::String docs = io::filesystem()->specialDir(io::FilesystemDirectories::FS_Dir_Documents);
		Log::info("Local document scanning (%s)...", docs.c_str());
		io::filesystem()->list(docs, entities, "", 2);

		for (const io::FilesystemEntry &entry : entities) {
			if (!io::isA(entry.name, voxelformat::voxelLoad())) {
				continue;
			}
			voxbrowser::VoxelFile voxelFile;
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

	_threadPool->enqueue([&]() {
		voxbrowser::Downloader downloader;
		core::DynamicArray<voxbrowser::VoxelFile> files;
		auto sources = downloader.sources();
		Log::info("Found %d online sources", (int)sources.size());
		for (const voxbrowser::VoxelSource &source : sources) {
			const core::DynamicArray<voxbrowser::VoxelFile> &files = downloader.resolve(source);
			_newVoxelFiles.push(files.begin(), files.end());
		}
		return files;
	});

	return state;
}

void VoxBrowser::onRenderUI() {
	// TODO: support a local voxel file source, too
	_mainWindow->update(_voxelFilesMap);
}

void VoxBrowser::loadThumbnail(const voxbrowser::VoxelFile &voxelFile) {
	if (_texturePool.has(voxelFile.name)) {
		return;
	}
	const core::String &targetImageFile = io::filesystem()->writePath(voxelFile.targetFile() + ".png");
	if (io::filesystem()->exists(targetImageFile)) {
		_threadPool->enqueue([this, voxelFile, targetImageFile]() {
			image::ImagePtr image = image::loadImage(targetImageFile);
			if (image) {
				image->setName(voxelFile.name);
				_imageQueue.push(image);
			}
		});
		return;
	}
	if (!io::filesystem()->createDir(core::string::extractPath(targetImageFile))) {
		Log::warn("Failed to create directory for thumbnails at: %s", voxelFile.targetDir().c_str());
		return;
	}
	if (!voxelFile.thumbnailUrl.empty()) {
		_threadPool->enqueue([=]() {
			http::HttpCacheStream stream(filesystem(), voxelFile.targetFile() + ".png", voxelFile.thumbnailUrl);
			this->_imageQueue.push(image::loadImage(voxelFile.name, stream));
		});
	} else {
		_threadPool->enqueue([=]() {
			http::HttpCacheStream stream(filesystem(), voxelFile.fullPath, voxelFile.url);
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

app::AppState VoxBrowser::onRunning() {
	app::AppState state = Super::onRunning();
	if (state != app::AppState::Running) {
		return state;
	}

	voxbrowser::VoxelFiles voxelFiles;
	_newVoxelFiles.pop(voxelFiles, 100);

	image::ImagePtr image;
	if (_imageQueue.pop(image)) {
		if (image && image->isLoaded()) {
			_texturePool.addImage(image);
		}
	}

	for (voxbrowser::VoxelFile &voxelFile : voxelFiles) {
		loadThumbnail(voxelFile);
		const core::String &relTargetFile = voxelFile.targetFile();

		auto iter = _voxelFilesMap.find(voxelFile.source);
		if (iter != _voxelFilesMap.end()) {
			iter->value.push_back(voxelFile);
		} else {
			_voxelFilesMap.put(voxelFile.source, {voxelFile});
		}
	}
	if (!voxelFiles.empty()) {
		for (auto e : _voxelFilesMap) {
			e->value.sort([] (const voxbrowser::VoxelFile &a, const voxbrowser::VoxelFile &b) {
				return a.name < b.name;
			});
		}
	}
	return state;
}

int main(int argc, char *argv[]) {
	const io::FilesystemPtr &filesystem = core::make_shared<io::Filesystem>();
	const core::TimeProviderPtr &timeProvider = core::make_shared<core::TimeProvider>();
	VoxBrowser app(filesystem, timeProvider);
	return app.startMainLoop(argc, argv);
}