/**
 * @file
 */

#include "VoxBrowser.h"
#include "app/App.h"
#include "command/Command.h"
#include "core/BindingContext.h"
#include "core/Log.h"
#include "core/TimeProvider.h"
#include "core/concurrent/Concurrency.h"
#include "http/HttpCacheStream.h"
#include "io/Archive.h"
#include "io/File.h"
#include "io/Filesystem.h"

#include "voxbrowser-ui/MainWindow.h"
#include "voxelformat/FormatConfig.h"
#include "engine-git.h"

VoxBrowser::VoxBrowser(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider, const video::TexturePoolPtr &texturePool)
	: Super(filesystem, timeProvider, core::halfcpus()), _collectionMgr(filesystem, texturePool), _texturePool(texturePool) {
	init(ORGANISATION, "voxbrowser");
}

app::AppState VoxBrowser::onCleanup() {
	if (_mainWindow) {
		_mainWindow->shutdown();
		delete _mainWindow;
	}
	_texturePool->shutdown();
	_collectionMgr.shutdown();
	return Super::onCleanup();
}

app::AppState VoxBrowser::onConstruct() {
	const app::AppState state = Super::onConstruct();
	_framesPerSecondsCap->setVal(60.0f);

	voxelformat::FormatConfig::init();

	_collectionMgr.construct();
	_texturePool->construct();

	command::Command::registerCommand("downloadall", [this](const command::CmdArgs &args) {
		_collectionMgr.downloadAll();
	}).setHelp("Download all missing files");

	command::Command::registerCommand("thumbnaildownloadall", [this](const command::CmdArgs &args) {
		_collectionMgr.thumbnailAll();
	}).setHelp("Download missing thumbnails");

	return state;
}

app::AppState VoxBrowser::onInit() {
	const app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}

	if (!_texturePool->init()) {
		Log::error("Failed to initialize the texture pool");
		return app::AppState::InitFailure;
	}

	if (!_collectionMgr.init()) {
		Log::error("Failed to initialize the collection manager");
		return app::AppState::InitFailure;
	}

	_mainWindow = new voxbrowser::MainWindow(this, _texturePool);
	if (!_mainWindow->init()) {
		Log::error("Failed to initialize the main window");
		return app::AppState::InitFailure;
	}

	_collectionMgr.local();
	_collectionMgr.online();

	return state;
}

void VoxBrowser::onRenderUI() {
	_mainWindow->update(_collectionMgr);
}

void VoxBrowser::printUsageHeader() const {
	Super::printUsageHeader();
	Log::info("Git commit " GIT_COMMIT " - " GIT_COMMIT_DATE);
}

app::AppState VoxBrowser::onRunning() {
	app::AppState state = Super::onRunning();
	if (state != app::AppState::Running) {
		return state;
	}
	_collectionMgr.update(_nowSeconds);
	return state;
}

int main(int argc, char *argv[]) {
	const io::FilesystemPtr &filesystem = core::make_shared<io::Filesystem>();
	const core::TimeProviderPtr &timeProvider = core::make_shared<core::TimeProvider>();
	const video::TexturePoolPtr &texturePool = core::make_shared<video::TexturePool>();
	VoxBrowser app(filesystem, timeProvider, texturePool);
	return app.startMainLoop(argc, argv);
}
