/**
 * @file
 */

#pragma once

#include "app/App.h"
#include "app/CommandlineApp.h"

namespace voxedit {
class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;
} // namespace voxedit

/**
 * @brief This is the server to be used by the Voxedit application if you want to allow multiple users editing the same
 * scene
 *
 * @ingroup Tools
 */
class VoxEditServer : public app::CommandlineApp {
private:
	using Super = app::CommandlineApp;

protected:
	voxedit::SceneManagerPtr _sceneMgr;

public:
	VoxEditServer(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider,
				  const voxedit::SceneManagerPtr &sceneMgr);

	app::AppState onConstruct() override;
	app::AppState onInit() override;
	app::AppState onRunning() override;
	app::AppState onCleanup() override;
};
