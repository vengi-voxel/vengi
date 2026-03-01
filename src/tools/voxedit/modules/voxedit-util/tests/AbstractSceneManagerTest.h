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
				   const SceneRendererPtr &sceneRenderer, const ModifierRendererPtr &modifierRenderer)
		: SceneManager(timeProvider, filesystem, sceneRenderer, modifierRenderer) {
	}
	bool loadForTest(scenegraph::SceneGraph &&sceneGraph) {
		return loadSceneGraph(core::move(sceneGraph));
	}

	int nodeColorToNewNode(int nodeId, const voxel::Voxel voxelColor) {
		return SceneManager::nodeColorToNewNode(nodeId, voxelColor);
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

	void testFillHollow() {
		nodeGroupFillHollow();
	}

	void testHollow() {
		nodeGroupHollow();
	}

	void testFill() {
		nodeGroupFill();
	}

	void testClear() {
		nodeGroupClear();
	}

	void testFlip(math::Axis axis) {
		nodeGroupFlip(axis);
	}

	bool testSaveNode(int nodeId, const core::String &file) {
		return nodeSave(nodeId, file);
	}

	bool testSaveModels(const core::String &dir) {
		return saveModels(dir);
	}

	void testSplitObjects(int nodeId) {
		nodeSplitObjects(nodeId);
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
		_sceneMgr =
			core::make_shared<SceneManagerEx>(timeProvider, _testApp->filesystem(), sceneRenderer, modifierRenderer);
		const core::VarDef uILastDirectory(cfg::UILastDirectory, "", "Last Directory",
										   "The last directory used in the UI", core::CV_NOPERSIST);
		core::Var::registerVar(uILastDirectory);
		const core::VarDef clientMouseRotationSpeed(cfg::ClientMouseRotationSpeed, 0.01f, "Mouse Rotation Speed",
													"The speed at which the camera rotates with the mouse",
													core::CV_NONE);
		core::Var::registerVar(clientMouseRotationSpeed);
		const core::VarDef clientCameraZoomSpeed(cfg::ClientCameraZoomSpeed, 0.1f, "Camera Zoom Speed",
												 "The speed at which the camera zooms", core::CV_NONE);
		core::Var::registerVar(clientCameraZoomSpeed);
		_sceneMgr->construct();
		ASSERT_TRUE(_sceneMgr->init());

		const voxel::Region region{0, 1};
		ASSERT_TRUE(_sceneMgr->newScene(true, "newscene", region));
	}
};

} // namespace voxedit
