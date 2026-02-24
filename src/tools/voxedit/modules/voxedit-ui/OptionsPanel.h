/**
 * @file
 */

#pragma once

#include "ui/Panel.h"
#include <stdint.h>

namespace voxedit {

enum class OptionCategory : uint8_t {
	UserInterface,
	Editor,
	Metrics,
	Layout,
	Display,
	Rendering,
	Renderer,
	MeshExport,
	VoxelImportExport,
	AllVariables,
	Max
};

/**
 * @brief Panel for viewing and editing application options/settings
 * @ingroup UI
 *
 * Replaces the old Options submenu with a dedicated dockable panel that supports
 * filtering and grouping of configuration variables via a tree view.
 */
class OptionsPanel : public ui::Panel {
private:
	using Super = ui::Panel;

	core::String _filter;
	core::String _lastFilter;
	bool _resetDockLayout = false;
	bool _visible = false;
	bool _requestFocus = false;
	OptionCategory _selectedCategory = OptionCategory::UserInterface;

	bool matchesFilter(const char *text) const;
	bool matchesVarFilter(const char *varName) const;
	bool hasFilter() const;
	/**
	 * @brief Check if a category has any items matching the current filter
	 */
	bool categoryHasMatch(OptionCategory category) const;

	void renderUserInterface();
	void renderEditor();
	void renderMetrics();
	void renderLayout();
	void renderDisplay();
	void renderRendering();
	void renderRenderer();
	void renderMeshExport();
	void renderVoxelImportExport();
	void renderAllVariables();

	void renderTree();
	void renderContent();

public:
	OptionsPanel(ui::IMGUIApp *app) : Super(app, "options") {
	}

	void update(const char *id);

	void toggleVisible() {
		_visible = !_visible;
		if (_visible) {
			_requestFocus = true;
		}
	}

	void showAllVariables() {
		_visible = true;
		_requestFocus = true;
		_selectedCategory = OptionCategory::AllVariables;
	}

	void setVisible(bool visible) {
		_visible = visible;
	}

	bool isVisible() const {
		return _visible;
	}

	/**
	 * @brief Check and reset the dock layout flag
	 * @return @c true if the dock layout should be reset
	 */
	bool shouldResetDockLayout();

#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *id) override;
#endif
};

} // namespace voxedit
