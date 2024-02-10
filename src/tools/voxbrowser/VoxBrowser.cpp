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
		voxbrowser::Downloader downloader;
		core::DynamicArray<voxbrowser::VoxelFile> files;
		auto sources = downloader.sources();
		Log::info("Found %d sources", (int)sources.size());
		for (const voxbrowser::VoxelSource &source : sources) {
			const auto &files = downloader.resolve(source);
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
	const core::String &targetImagePath = io::filesystem()->writePath(voxelFile.targetFile() + ".png");
	if (io::filesystem()->exists(targetImagePath)) {
		_threadPool->enqueue([this, voxelFile = voxelFile, targetImagePath = targetImagePath]() {
			image::ImagePtr image = image::loadImage(targetImagePath);
			if (image) {
				image->setName(voxelFile.name);
				_imageQueue.push(image);
			}
		});
		return;
	}
	if (!voxelFile.thumbnailUrl.empty()) {
		_threadPool->enqueue([this, voxelFile = voxelFile]() {
			http::HttpCacheStream stream(filesystem(), voxelFile.targetFile() + ".png", voxelFile.thumbnailUrl);
			_imageQueue.push(image::loadImage(voxelFile.name, stream));
		});
	} else {
		_threadPool->enqueue([this, voxelFile = voxelFile, targetImagePath = targetImagePath]() {
			http::HttpCacheStream stream(filesystem(), voxelFile.targetFile(), voxelFile.url);
			voxelformat::LoadContext loadCtx;
			image::ImagePtr thumbnailImage = voxelformat::loadScreenshot(voxelFile.targetFile(), stream, loadCtx);
			if (!thumbnailImage) {
				Log::debug("Failed to load given input file: %s", voxelFile.targetFile().c_str());
				return;
			}
			thumbnailImage->setName(voxelFile.name);
			if (!image::writeImage(thumbnailImage, targetImagePath)) {
				Log::warn("Failed to save thumbnail for %s to %s", voxelFile.name.c_str(), targetImagePath.c_str());
			} else {
				Log::info("Created thumbnail for %s at %s", voxelFile.name.c_str(), targetImagePath.c_str());
			}
			_imageQueue.push(thumbnailImage);
		});
	}
}

app::AppState VoxBrowser::onRunning() {
	app::AppState state = Super::onRunning();
	if (state != app::AppState::Running) {
		return state;
	}

	voxbrowser::VoxelFiles voxelFiles;
	_newVoxelFiles.popAll(voxelFiles);

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
	return state;
}

int main(int argc, char *argv[]) {
	const io::FilesystemPtr &filesystem = core::make_shared<io::Filesystem>();
	const core::TimeProviderPtr &timeProvider = core::make_shared<core::TimeProvider>();
	VoxBrowser app(filesystem, timeProvider);
	return app.startMainLoop(argc, argv);
}
