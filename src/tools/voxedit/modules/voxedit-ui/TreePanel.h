/**
 * @file
 */

#pragma once

#include "ui/Panel.h"
#include "voxelgenerator/TreeContext.h"

namespace voxedit {

/**
 * @brief Panel for the tree generator
 */
class TreePanel : public ui::Panel {
private:
	ui::IMGUIApp *_app;

	voxelgenerator::TreeContext _treeGeneratorContext;

	void switchTreeType(voxelgenerator::TreeType treeType);
public:
	PANEL_CLASS(TreePanel)
	bool init();
	void update(const char *title);
	void shutdown();
};

}
