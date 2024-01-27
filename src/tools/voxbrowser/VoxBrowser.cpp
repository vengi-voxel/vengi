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
#include "io/Filesystem.h"

#include "voxbrowser-ui/MainWindow.h"
#include "voxbrowser-util/Downloader.h"
#include "voxelformat/FormatConfig.h"

VoxBrowser::VoxBrowser(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider)
	: Super(filesystem, timeProvider, core::halfcpus()) {
	init(ORGANISATION, "voxbrowser");
}

app::AppState VoxBrowser::onCleanup() {
	if (_mainWindow) {
		_mainWindow->shutdown();
		delete _mainWindow;
	}
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

	_mainWindow = new voxbrowser::MainWindow(this);
	if (!_mainWindow->init()) {
		Log::error("Failed to initialize the main window");
		return app::AppState::InitFailure;
	}

	return state;
}

void VoxBrowser::onRenderUI() {
	// TODO: support a local voxel file source, too
	_mainWindow->update(_voxelFilesMap);
}

app::AppState VoxBrowser::onRunning() {
	app::AppState state = Super::onRunning();
	if (state != app::AppState::Running) {
		return state;
	}

	voxbrowser::VoxelFiles voxelFiles;
	_newVoxelFiles.popAll(voxelFiles);
	for (voxbrowser::VoxelFile &voxelFile : voxelFiles) {
		auto iter = _voxelFilesMap.find(voxelFile.source);
		if (iter != _voxelFilesMap.end()) {
			iter->value.push_back(voxelFile);
			continue;
		}
		_voxelFilesMap.put(voxelFile.source, {voxelFile});
	}
	return state;
}

int main(int argc, char *argv[]) {
	const io::FilesystemPtr &filesystem = core::make_shared<io::Filesystem>();
	const core::TimeProviderPtr &timeProvider = core::make_shared<core::TimeProvider>();
	VoxBrowser app(filesystem, timeProvider);
	return app.startMainLoop(argc, argv);
}
