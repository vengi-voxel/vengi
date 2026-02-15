#include "VoxEdit.h"
#include "voxedit-util/SceneRenderer.h"
#include "voxedit-util/modifier/ModifierRenderer.h"

class VoxEditTest : public VoxEdit {
private:
	using Super = VoxEdit;

public:
	VoxEditTest(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider,
				const voxedit::SceneManagerPtr &sceneMgr, const voxelcollection::CollectionManagerPtr &collectionMgr,
				const video::TexturePoolPtr &texturePool, const voxedit::SceneRendererPtr &sceneRenderer)
		: VoxEdit(filesystem, timeProvider, sceneMgr, collectionMgr, texturePool, sceneRenderer) {
		_showWindow = false;
		_wantCrashLogs = false;
	}

	bool allowedToQuit() override {
		return true;
	}
};

int main(int argc, char *argv[]) {
	const io::FilesystemPtr &filesystem = core::make_shared<io::Filesystem>();
	const core::TimeProviderPtr &timeProvider = core::make_shared<core::TimeProvider>();
	const voxedit::SceneRendererPtr &sceneRenderer = core::make_shared<voxedit::SceneRenderer>();
	const voxedit::ModifierRendererPtr &modifierRenderer = core::make_shared<voxedit::ModifierRenderer>();
	const video::TexturePoolPtr &texturePool = core::make_shared<video::TexturePool>();
	const voxedit::SceneManagerPtr &sceneMgr = core::make_shared<voxedit::SceneManager>(
		timeProvider, filesystem, sceneRenderer, modifierRenderer);
	const voxelcollection::CollectionManagerPtr &collectionMgr =
		core::make_shared<voxelcollection::CollectionManager>(filesystem, texturePool);
	VoxEditTest app(filesystem, timeProvider, sceneMgr, collectionMgr, texturePool, sceneRenderer);
	return app.startMainLoop(argc, argv);
}
