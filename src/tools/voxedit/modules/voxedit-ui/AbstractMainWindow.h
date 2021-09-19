/**
 * @file
 */

#include "core/Var.h"
#include "AbstractViewport.h"
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

	voxelgenerator::TreeContext _treeGeneratorContext;

	void afterLoad(const core::String& file);
	virtual bool saveImage(const char *file) = 0;

public:
	AbstractMainWindow(video::WindowedApp* app);
	virtual ~AbstractMainWindow() {}

	bool importHeightmap(const core::String& file);
	bool importAsPlane(const core::String& file);
	bool importPalette(const core::String& file);

	bool saveScreenshot(const core::String& file);
	bool prefab(const core::String& file);
	virtual void resetCamera() = 0;
};

}
