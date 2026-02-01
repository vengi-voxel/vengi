/**
 * @file
 */

#include "VoxEditServer.h"
#include "app/App.h"
#include "core/Var.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/IModifierRenderer.h"

VoxEditServer::VoxEditServer(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider,
							 const voxedit::SceneManagerPtr &sceneMgr)
	: Super(filesystem, timeProvider, core::cpus()), _sceneMgr(sceneMgr) {
	init(ORGANISATION, "voxeditserver");
	_wantCrashLogs = true;
}

app::AppState VoxEditServer::onConstruct() {
	app::AppState appState = Super::onConstruct();
	core::Var::get(cfg::UILastDirectory, filesystem()->homePath().c_str());
	_sceneMgr->construct();
	core::Var::getSafe(cfg::AppVersion)->setVal(PROJECT_VERSION);
	return appState;
}

app::AppState VoxEditServer::onInit() {
	app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}
	if (!_sceneMgr->init()) {
		return app::AppState::InitFailure;
	}

	int port = core::Var::getSafe(cfg::VoxEditNetPort)->intVal();
	const core::String &iface = core::Var::getSafe(cfg::VoxEditNetServerInterface)->strVal();
	_sceneMgr->startLocalServer(port, iface);
	if (!_sceneMgr->server().isRunning()) {
		Log::error("Failed to start the voxedit server on %s:%i", iface.c_str(), port);
		return app::AppState::InitFailure;
	}
	// The client connect is needed here, too - as we need to track our own scene state to be able to send them out to
	// the clients.
	// See Server::shouldRequestClientState()
	if (!_sceneMgr->client().isConnected()) {
		Log::error("Failed to connect the local client to the server on %s:%i", iface.c_str(), port);
		return app::AppState::InitFailure;
	}

	Log::info("Server running on %s:%i", iface.c_str(), port);

	return state;
}

app::AppState VoxEditServer::onRunning() {
	Super::onRunning();
	_sceneMgr->update(_nowSeconds);
	return shouldQuit() ? app::AppState::Cleanup : app::AppState::Running;
}

app::AppState VoxEditServer::onCleanup() {
	_sceneMgr->shutdown();
	return Super::onCleanup();
}

int main(int argc, char *argv[]) {
	const io::FilesystemPtr &filesystem = core::make_shared<io::Filesystem>();
	const core::TimeProviderPtr &timeProvider = core::make_shared<core::TimeProvider>();
	const core::SharedPtr<voxedit::ISceneRenderer> &sceneRenderer = core::make_shared<voxedit::ISceneRenderer>();
	const core::SharedPtr<voxedit::IModifierRenderer> &modifierRenderer =
		core::make_shared<voxedit::IModifierRenderer>();
	const voxedit::SceneManagerPtr &sceneMgr = core::make_shared<voxedit::SceneManager>(
		timeProvider, filesystem, sceneRenderer, modifierRenderer);
	VoxEditServer app(filesystem, timeProvider, sceneMgr);
	return app.startMainLoop(argc, argv);
}
