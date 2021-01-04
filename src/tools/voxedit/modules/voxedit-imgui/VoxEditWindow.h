/**
 * @file
 */

#include "core/Common.h"
#include "core/StringUtil.h"
#include "math/Axis.h"
#include "video/WindowedApp.h"
#include "voxedit-util/AbstractMainWindow.h"
#include "voxedit-util/SceneSettings.h"
#include "voxedit-util/layer/LayerSettings.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxelgenerator/TreeContext.h"

namespace voxedit {

class Viewport;

class VoxEditWindow : public AbstractMainWindow {
private:
	using Super = AbstractMainWindow;

	core::DynamicArray<core::String> _scripts;

	core::VarPtr _showAxisVar;
	core::VarPtr _showGridVar;
	core::VarPtr _modelSpaceVar;
	core::VarPtr _showLockedAxisVar;
	core::VarPtr _showAabbVar;
	core::VarPtr _renderShadowVar;
	core::VarPtr _animationSpeedVar;
	core::VarPtr _gridSizeVar;

	void menuBar();
	void palette();
	void tools();
	void layers();
	void statusBar();

	void leftWidget();
	void mainWidget();
	void rightWidget();

	void afterLoad(const core::String &file);

	void updateSettings();

	bool actionButton(const char *title, const char *command);
	bool actionMenuItem(const char *title, const char *command);
	bool modifierRadioButton(const char *title, ModifierType type);
	bool mirrorAxisRadioButton(const char *title, math::Axis type);

public:
	VoxEditWindow(video::WindowedApp *app);
	~VoxEditWindow();
	bool init();
	void shutdown();

	// commands
	void toggleViewport();
	void toggleAnimation();
	bool save(const core::String &file);
	bool load(const core::String &file);
	bool loadAnimationEntity(const core::String &file);
	bool createNew(bool force);

	bool isLayerWidgetDropTarget() const;
	bool isPaletteWidgetDropTarget() const;

	void update();
	bool isSceneHovered() const;
};

} // namespace voxedit
