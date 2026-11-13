/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/DynamicArray.h"

struct ImGuiInputTextCallbackData;
struct ImGuiTestEngine;

namespace command {
struct CommandExecutionListener;
}

namespace util {
class Console;
}

namespace ui {

#define POPUP_TITLE_COMMAND_PALETTE "###commandpalette"

/**
 * @brief VS Code style command palette with a focused filter input and a wide result list.
 * Shows console history and commands, truncates help with ellipsis, and supports Tab completion
 * for declared command arguments.
 * @ingroup UI
 */
class CommandPalette {
private:
	struct Entry {
		core::String name;
		core::String help;
		core::String shortcut;
		bool history = false;
	};

	bool _requestOpen = false;
	bool _focusSearch = false;
	int _selectedVisible = -1;
	int _completingArgIndex = -1;
	core::String _searchFilter;
	core::String _selectedCommand;
	core::String _freeArgs;
	core::DynamicArray<Entry> _entries;
	core::DynamicArray<int> _visible;
	core::DynamicArray<core::String> _paramValues;
	core::DynamicArray<core::String> _completionMatches;

	void rebuildEntries(const util::Console &console);
	void rebuildVisible();
	void selectVisible(int visibleIndex);
	void prepareParameters();
	void onFilterEdited();
	bool captureFilterInput(unsigned int filterId, bool filterFocused);
	bool executeSelected(command::CommandExecutionListener &listener, util::Console &console);
	void renderParameterFields();
	bool matchesFilter(const Entry &entry) const;
	static int argInputCallback(ImGuiInputTextCallbackData *data);

public:
	CommandPalette() = default;
	~CommandPalette() = default;

	void show();
	void reset();
	void render(command::CommandExecutionListener &listener, util::Console &console);

#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *id);
	static const char *testCategory() {
		return "commandpalette";
	}
#endif
};

} // namespace ui
