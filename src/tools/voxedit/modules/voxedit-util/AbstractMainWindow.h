/**
 * @file
 */

#include "core/Var.h"
#include "voxedit-util/AbstractViewport.h"
#include "voxedit-util/layer/LayerSettings.h"
#include "voxedit-util/SceneSettings.h"
#include "video/WindowedApp.h"
#include "voxelgenerator/TreeContext.h"

namespace voxedit {

class AbstractMainWindow {
protected:
	video::WindowedApp* _app;
	core::VarPtr _lastOpenedFile;

	glm::ivec3 _lastCursorPos;

	LayerSettings _layerSettings;
	SceneSettings _settings;

	core::String _activeScript;
	core::String _loadFile;
	core::String _lastExecutedCommand;

	bool _fourViewAvailable = false;
	bool _animationViewAvailable = false;

	AbstractViewport* _scene = nullptr;
	AbstractViewport* _sceneTop = nullptr;
	AbstractViewport* _sceneLeft = nullptr;
	AbstractViewport* _sceneFront = nullptr;
	AbstractViewport* _sceneAnimation = nullptr;

	voxelgenerator::TreeContext _treeGeneratorContext;

	void afterLoad(const core::String& file);

public:
	AbstractMainWindow(video::WindowedApp* app);

	bool importHeightmap(const core::String& file);
	bool importAsPlane(const core::String& file);
	bool importPalette(const core::String& file);

	bool saveScreenshot(const core::String& file);
	bool prefab(const core::String& file);
	void resetCamera();
};

}
