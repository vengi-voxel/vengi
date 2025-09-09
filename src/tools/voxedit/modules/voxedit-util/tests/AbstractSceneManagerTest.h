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
		core::Var::get(cfg::VoxEditShowgrid, "true");
		core::Var::get(cfg::VoxEditShowlockedaxis, "true");
		core::Var::get(cfg::VoxEditRendershadow, "true");
		core::Var::get(cfg::VoxEditGridsize, "1");
		core::Var::get(cfg::VoxEditPlaneSize, "0");
		core::Var::get(cfg::VoxEditShowPlane, "false");
		core::Var::get(cfg::VoxEditMaxSuggestedVolumeSize, "128");
		core::Var::get(cfg::VoxEditMaxSuggestedVolumeSizePreview, "32");
		core::Var::get(cfg::VoxelMeshMode, core::string::toString((int)voxel::SurfaceExtractionType::Binary));
		core::Var::get(cfg::VoxelMeshSize, "16", core::CV_READONLY);
		core::Var::get(cfg::VoxEditShowaabb, "");
		core::Var::get(cfg::VoxEditShowBones, "");
		core::Var::get(cfg::VoxEditGrayInactive, "");
		core::Var::get(cfg::VoxEditHideInactive, "");
		core::Var::get(cfg::VoxEditLastPalette, "");
		core::Var::get(cfg::VoxEditModificationDismissMillis, "0");
		_sceneMgr->construct();
		ASSERT_TRUE(_sceneMgr->init());

		const voxel::Region region{0, 1};
		ASSERT_TRUE(_sceneMgr->newScene(true, "newscene", region));
	}
};

} // namespace voxedit
