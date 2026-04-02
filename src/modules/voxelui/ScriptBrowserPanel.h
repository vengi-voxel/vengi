/**
 * @file
 */

#pragma once

#include "ui/Panel.h"
#include "voxelui/ScriptApi.h"

namespace voxelui {

class ScriptBrowserPanel : public ui::Panel {
private:
	using Super = ui::Panel;
	voxelui::ScriptInfoList _scripts;
	core::String _searchFilter;
	bool _requestPending = false;
	bool _open = false;
	bool _needsReload = false;
	bool _requestFocus = false;

	void fetchScripts();
	bool filtered(const voxelui::ScriptInfo &info) const;
	bool isInstalled(const voxelui::ScriptInfo &info) const;

public:
	ScriptBrowserPanel(ui::IMGUIApp *app) : Super(app, "scriptbrowser") {
	}
	void open();
	void update(const char *id);
	bool needsReload();
};

} // namespace voxelui
