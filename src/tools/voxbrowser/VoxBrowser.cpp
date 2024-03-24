/**
 * @file
 */

#include "VoxBrowser.h"
#include "app/App.h"
#include "app/Async.h"
#include "command/Command.h"
#include "core/BindingContext.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/TimeProvider.h"
#include "core/collection/DynamicArray.h"
#include "core/concurrent/Atomic.h"
#include "core/concurrent/Concurrency.h"
#include "http/HttpCacheStream.h"
#include "image/Image.h"
#include "io/Archive.h"
#include "io/File.h"
#include "io/Filesystem.h"

#include "io/FilesystemEntry.h"
#include "voxbrowser-ui/MainWindow.h"
#include "voxelcollection/Downloader.h"
#include "voxelformat/Format.h"
#include "voxelformat/FormatConfig.h"
#include "voxelformat/VolumeFormat.h"
#include "engine-git.h"

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

	command::Command::registerCommand("downloadall", [this](const command::CmdArgs &args) {
		downloadAll();
	}).setHelp("Download all missing files");

	command::Command::registerCommand("thumbnaildownloadall", [this](const command::CmdArgs &args) {
		thumbnailAll();
	}).setHelp("Download missing thumbnails");

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

	app::async([&]() {
		core::DynamicArray<io::FilesystemEntry> entities;
		const core::String docs = io::filesystem()->specialDir(io::FilesystemDirectories::FS_Dir_Documents);
		Log::info("Local document scanning (%s)...", docs.c_str());
		io::filesystem()->list(docs, entities, "", 2);

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

	app::async([&]() {
		voxelcollection::Downloader downloader;
		core::DynamicArray<voxelcollection::VoxelFile> files;
		auto sources = downloader.sources();
		Log::info("Found %d online sources", (int)sources.size());
		for (const voxelcollection::VoxelSource &source : sources) {
			const core::DynamicArray<voxelcollection::VoxelFile> &files = downloader.resolve(_filesystem, source);
			_newVoxelFiles.push(files.begin(), files.end());
		}
		return files;
	});

	return state;
}

void VoxBrowser::onRenderUI() {
	_mainWindow->update(_voxelFilesMap, _downloadProgress, _count);
}

void VoxBrowser::printUsageHeader() const {
	Super::printUsageHeader();
	Log::info("Git commit " GIT_COMMIT " - " GIT_COMMIT_DATE);
}

void VoxBrowser::downloadAll() {
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

void VoxBrowser::thumbnailAll() {
	for (const auto &e : _voxelFilesMap) {
		for (const voxelcollection::VoxelFile &voxelFile : e->value.files) {
			loadThumbnail(voxelFile);
		}
	}
}

void VoxBrowser::loadThumbnail(const voxelcollection::VoxelFile &voxelFile) {
	if (_texturePool.has(voxelFile.name)) {
		return;
	}
	const core::String &targetImageFile = io::filesystem()->writePath(voxelFile.targetFile() + ".png");
	if (io::filesystem()->exists(targetImageFile)) {
		app::async([this, voxelFile, targetImageFile]() {
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
		app::async([=]() {
			http::HttpCacheStream stream(filesystem(), voxelFile.targetFile() + ".png", voxelFile.thumbnailUrl);
			this->_imageQueue.push(image::loadImage(voxelFile.name, stream));
		});
	} else {
		app::async([=]() {
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

	voxelcollection::VoxelFiles voxelFiles;
	_newVoxelFiles.pop(voxelFiles, 100);

	image::ImagePtr image;
	if (_imageQueue.pop(image)) {
		if (image && image->isLoaded()) {
			_texturePool.addImage(image);
		}
	}

	for (voxelcollection::VoxelFile &voxelFile : voxelFiles) {
		loadThumbnail(voxelFile);
		auto iter = _voxelFilesMap.find(voxelFile.source);
		if (iter != _voxelFilesMap.end()) {
			voxelcollection::VoxelCollection &collection = iter->value;
			collection.files.push_back(voxelFile);
			collection.timestamp = _nowSeconds;
			collection.sorted = false;
		} else {
			voxelcollection::VoxelCollection collection{{voxelFile}, _nowSeconds, false};
			_voxelFilesMap.put(voxelFile.source, collection);
		}
	}
	for (auto e : _voxelFilesMap) {
		voxelcollection::VoxelCollection &collection = e->value;
		if (collection.sorted) {
			continue;
		}
		if (collection.timestamp + 5.0 > _nowSeconds) {
			continue;
		}
		core::sort(collection.files.begin(), collection.files.end(), core::Less<voxelcollection::VoxelFile>());
		collection.sorted = true;
	}
	_count += voxelFiles.size();
	return state;
}

int main(int argc, char *argv[]) {
	const io::FilesystemPtr &filesystem = core::make_shared<io::Filesystem>();
	const core::TimeProviderPtr &timeProvider = core::make_shared<core::TimeProvider>();
	VoxBrowser app(filesystem, timeProvider);
	return app.startMainLoop(argc, argv);
}
