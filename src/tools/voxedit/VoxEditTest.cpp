#include "VoxEdit.h"
#include "voxedit-util/SceneRenderer.h"
#include "voxedit-util/modifier/ModifierRenderer.h"
#include "voxedit-ui/MainWindow.h"

class VoxEditTest : public VoxEdit {
private:
	using Super = VoxEdit;

public:
	VoxEditTest(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider,
				const voxedit::SceneManagerPtr &sceneMgr, const voxelcollection::CollectionManagerPtr &collectionMgr,
				const video::TexturePoolPtr &texturePool)
		: VoxEdit(filesystem, timeProvider, sceneMgr, collectionMgr, texturePool) {
		_showWindow = false;
	}

	app::AppState onInit() override {
		app::AppState state = Super::onInit();
		if (state != app::AppState::Running) {
			return state;
		}

		// register the ui tests late - as we need the main window
		_mainWindow->registerUITests(_imguiTestEngine, "###app");

		return state;
	}
};

int main(int argc, char *argv[]) {
	const io::FilesystemPtr &filesystem = core::make_shared<io::Filesystem>();
	const core::TimeProviderPtr &timeProvider = core::make_shared<core::TimeProvider>();
	const core::SharedPtr<voxedit::SceneRenderer> &sceneRenderer = core::make_shared<voxedit::SceneRenderer>();
	const core::SharedPtr<voxedit::ModifierRenderer> &modifierRenderer = core::make_shared<voxedit::ModifierRenderer>();
	const video::TexturePoolPtr &texturePool = core::make_shared<video::TexturePool>();
	const voxedit::SceneManagerPtr &sceneMgr =
		core::make_shared<voxedit::SceneManager>(timeProvider, filesystem, sceneRenderer, modifierRenderer);
	const voxelcollection::CollectionManagerPtr &collectionMgr =
		core::make_shared<voxelcollection::CollectionManager>(filesystem, texturePool);
	VoxEditTest app(filesystem, timeProvider, sceneMgr, collectionMgr, texturePool);
	return app.startMainLoop(argc, argv);
}
