/**
 * @file
 */

#include "../Config.h"
#include "../SceneManager.h"
#include "app/tests/AbstractTest.h"
#include "core/TimeProvider.h"
#include "scenegraph/SceneGraph.h"
#include "voxedit-util/ISceneRenderer.h"
#include "voxedit-util/modifier/IModifierRenderer.h"

namespace voxedit {

class SceneManagerEx : public SceneManager {
public:
	SceneManagerEx(const core::TimeProviderPtr &timeProvider, const io::FilesystemPtr &filesystem,
				   const SceneRendererPtr &sceneRenderer, const ModifierRendererPtr &modifierRenderer,
				   const SelectionManagerPtr &selectionManager)
		: SceneManager(timeProvider, filesystem, sceneRenderer, modifierRenderer, selectionManager) {
	}
	bool loadForTest(scenegraph::SceneGraph &&sceneGraph) {
		return loadSceneGraph(core::move(sceneGraph));
	}

	int colorToNewNode(int nodeId, const voxel::Voxel voxelColor) {
		return SceneManager::colorToNewNode(nodeId, voxelColor);
	}

	void setLastFilename(const core::String &name, const io::FormatDescription *desc = nullptr) {
		_lastFilename.set(name, desc);
	}

	void clearLastFilename() {
		_lastFilename.clear();
	}

	const voxelutil::PickResult &getPickResult() const {
		return _result;
	}

	bool testMouseRayTrace(bool force, const glm::mat4 &invModel) {
		return mouseRayTrace(force, invModel);
	}
};

class AbstractSceneManagerTest : public app::AbstractTest {
private:
	using Super = app::AbstractTest;

protected:
	SceneManagerPtr _sceneMgr;
	SceneManagerEx *sceneMgr() {
		return ((SceneManagerEx *)_sceneMgr.get());
	}

	void TearDown() override {
		_sceneMgr->shutdown();
		_sceneMgr.release();
		Super::TearDown();
	}

	void SetUp() override {
		Super::SetUp();
		const auto timeProvider = core::make_shared<core::TimeProvider>();
		const auto sceneRenderer = core::make_shared<ISceneRenderer>();
		const auto modifierRenderer = core::make_shared<IModifierRenderer>();
		const auto selectionManager = core::make_shared<SelectionManager>();
		_sceneMgr = core::make_shared<SceneManagerEx>(timeProvider, _testApp->filesystem(), sceneRenderer,
													  modifierRenderer, selectionManager);
		core::Var::get(cfg::UILastDirectory, "", core::CV_NOPERSIST);
		_sceneMgr->construct();
		ASSERT_TRUE(_sceneMgr->init());

		const voxel::Region region{0, 1};
		ASSERT_TRUE(_sceneMgr->newScene(true, "newscene", region));
	}
};

} // namespace voxedit
