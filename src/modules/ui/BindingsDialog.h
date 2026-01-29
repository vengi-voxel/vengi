/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/Var.h"
#include "core/collection/DynamicArray.h"
#include "video/KeyboardLayout.h"

namespace command {
class CommandExecutionListener;
}

namespace util {
class KeyBindingHandler;
}

namespace ui {

/**
 * @brief Dialog for viewing and editing key bindings
 * @ingroup UI
 */
class BindingsDialog {
private:
	/**
	 * @brief Currently selected binding index in the bindings dialog (-1 = none)
	 */
	int _selectedBindingIndex = -1;
	/**
	 * @brief Whether we're recording a new key binding
	 */
	bool _recordingBinding = false;
	/**
	 * @brief The command for which we're recording a new binding
	 */
	core::String _recordingCommand;
	/**
	 * @brief The context for which we're recording a new binding
	 */
	core::String _recordingContext;
	/**
	 * @brief The old key binding string (for unbinding when replacing)
	 */
	core::String _recordingOldBinding;
	/**
	 * @brief String to filter the bindings in the binding dialog
	 */
	core::String _bindingsFilter;

public:
	BindingsDialog() = default;
	~BindingsDialog() = default;

	/**
	 * @brief Render the bindings dialog
	 * @param show Reference to the show flag - will be set to false when dialog is closed
	 * @param keybindingHandler The keybinding handler to get/set bindings
	 * @param keyboardLayout The current keyboard layout
	 * @param uiKeyMaps Available keymaps (empty if none)
	 * @param uiKeyMap The keymap variable
	 * @param resetKeybindings Reference to reset flag
	 * @param lastExecutedCommand Command execution listener
	 */
	void render(bool &show,
				util::KeyBindingHandler &keybindingHandler,
				video::KeyboardLayout keyboardLayout,
				const core::DynamicArray<core::String> &uiKeyMaps,
				const core::VarPtr &uiKeyMap,
				bool &resetKeybindings,
				command::CommandExecutionListener &lastExecutedCommand);

	/**
	 * @brief Reset the dialog state (selection, recording, etc.)
	 */
	void reset();

	bool isRecording() const { return _recordingBinding; }
};

} // namespace ui
