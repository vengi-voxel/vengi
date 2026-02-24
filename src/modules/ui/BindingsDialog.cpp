/**
 * @file
 */

#include "BindingsDialog.h"
#include "IMGUIEx.h"
#include "IconsLucide.h"
#include "color/Color.h"
#include "command/Command.h"
#include "core/BindingContext.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "dearimgui/imgui.h"
#include "dearimgui/imgui_keyboard.h"
#include "util/CustomButtonNames.h"

#include <SDL_keyboard.h>
#include <SDL_keycode.h>

#if !SDL_VERSION_ATLEAST(3, 2, 0)
#define SDLK_A SDLK_a
#define SDLK_B SDLK_b
#define SDLK_C SDLK_c
#define SDLK_D SDLK_d
#define SDLK_E SDLK_e
#define SDLK_F SDLK_f
#define SDLK_G SDLK_g
#define SDLK_H SDLK_h
#define SDLK_I SDLK_i
#define SDLK_J SDLK_j
#define SDLK_K SDLK_k
#define SDLK_L SDLK_l
#define SDLK_M SDLK_m
#define SDLK_N SDLK_n
#define SDLK_O SDLK_o
#define SDLK_P SDLK_p
#define SDLK_Q SDLK_q
#define SDLK_R SDLK_r
#define SDLK_S SDLK_s
#define SDLK_T SDLK_t
#define SDLK_U SDLK_u
#define SDLK_V SDLK_v
#define SDLK_W SDLK_w
#define SDLK_X SDLK_x
#define SDLK_Y SDLK_y
#define SDLK_Z SDLK_z
#endif

namespace ui {

// Helper to convert SDL keycode to ImGuiKey
static ImGuiKey sdlKeyToImGuiKey(int32_t sdlKey) {
	switch (sdlKey) {
	case SDLK_TAB:
		return ImGuiKey_Tab;
	case SDLK_LEFT:
		return ImGuiKey_LeftArrow;
	case SDLK_RIGHT:
		return ImGuiKey_RightArrow;
	case SDLK_UP:
		return ImGuiKey_UpArrow;
	case SDLK_DOWN:
		return ImGuiKey_DownArrow;
	case SDLK_PAGEUP:
		return ImGuiKey_PageUp;
	case SDLK_PAGEDOWN:
		return ImGuiKey_PageDown;
	case SDLK_HOME:
		return ImGuiKey_Home;
	case SDLK_END:
		return ImGuiKey_End;
	case SDLK_INSERT:
		return ImGuiKey_Insert;
	case SDLK_DELETE:
		return ImGuiKey_Delete;
	case SDLK_BACKSPACE:
		return ImGuiKey_Backspace;
	case SDLK_SPACE:
		return ImGuiKey_Space;
	case SDLK_RETURN:
		return ImGuiKey_Enter;
	case SDLK_ESCAPE:
		return ImGuiKey_Escape;
	case SDLK_COMMA:
		return ImGuiKey_Comma;
	case SDLK_PERIOD:
		return ImGuiKey_Period;
	case SDLK_SEMICOLON:
		return ImGuiKey_Semicolon;
	case SDLK_CAPSLOCK:
		return ImGuiKey_CapsLock;
	case SDLK_SCROLLLOCK:
		return ImGuiKey_ScrollLock;
	case SDLK_NUMLOCKCLEAR:
		return ImGuiKey_NumLock;
	case SDLK_PRINTSCREEN:
		return ImGuiKey_PrintScreen;
	case SDLK_PAUSE:
		return ImGuiKey_Pause;
	case SDLK_LCTRL:
		return ImGuiKey_LeftCtrl;
	case SDLK_LSHIFT:
		return ImGuiKey_LeftShift;
	case SDLK_LALT:
		return ImGuiKey_LeftAlt;
	case SDLK_LGUI:
		return ImGuiKey_LeftSuper;
	case SDLK_RCTRL:
		return ImGuiKey_RightCtrl;
	case SDLK_RSHIFT:
		return ImGuiKey_RightShift;
	case SDLK_RALT:
		return ImGuiKey_RightAlt;
	case SDLK_RGUI:
		return ImGuiKey_RightSuper;
	case SDLK_APPLICATION:
		return ImGuiKey_Menu;
	case SDLK_0:
		return ImGuiKey_0;
	case SDLK_1:
		return ImGuiKey_1;
	case SDLK_2:
		return ImGuiKey_2;
	case SDLK_3:
		return ImGuiKey_3;
	case SDLK_4:
		return ImGuiKey_4;
	case SDLK_5:
		return ImGuiKey_5;
	case SDLK_6:
		return ImGuiKey_6;
	case SDLK_7:
		return ImGuiKey_7;
	case SDLK_8:
		return ImGuiKey_8;
	case SDLK_9:
		return ImGuiKey_9;
	case SDLK_A:
		return ImGuiKey_A;
	case SDLK_B:
		return ImGuiKey_B;
	case SDLK_C:
		return ImGuiKey_C;
	case SDLK_D:
		return ImGuiKey_D;
	case SDLK_E:
		return ImGuiKey_E;
	case SDLK_F:
		return ImGuiKey_F;
	case SDLK_G:
		return ImGuiKey_G;
	case SDLK_H:
		return ImGuiKey_H;
	case SDLK_I:
		return ImGuiKey_I;
	case SDLK_J:
		return ImGuiKey_J;
	case SDLK_K:
		return ImGuiKey_K;
	case SDLK_L:
		return ImGuiKey_L;
	case SDLK_M:
		return ImGuiKey_M;
	case SDLK_N:
		return ImGuiKey_N;
	case SDLK_O:
		return ImGuiKey_O;
	case SDLK_P:
		return ImGuiKey_P;
	case SDLK_Q:
		return ImGuiKey_Q;
	case SDLK_R:
		return ImGuiKey_R;
	case SDLK_S:
		return ImGuiKey_S;
	case SDLK_T:
		return ImGuiKey_T;
	case SDLK_U:
		return ImGuiKey_U;
	case SDLK_V:
		return ImGuiKey_V;
	case SDLK_W:
		return ImGuiKey_W;
	case SDLK_X:
		return ImGuiKey_X;
	case SDLK_Y:
		return ImGuiKey_Y;
	case SDLK_Z:
		return ImGuiKey_Z;
	case SDLK_F1:
		return ImGuiKey_F1;
	case SDLK_F2:
		return ImGuiKey_F2;
	case SDLK_F3:
		return ImGuiKey_F3;
	case SDLK_F4:
		return ImGuiKey_F4;
	case SDLK_F5:
		return ImGuiKey_F5;
	case SDLK_F6:
		return ImGuiKey_F6;
	case SDLK_F7:
		return ImGuiKey_F7;
	case SDLK_F8:
		return ImGuiKey_F8;
	case SDLK_F9:
		return ImGuiKey_F9;
	case SDLK_F10:
		return ImGuiKey_F10;
	case SDLK_F11:
		return ImGuiKey_F11;
	case SDLK_F12:
		return ImGuiKey_F12;
	case SDLK_KP_0:
		return ImGuiKey_Keypad0;
	case SDLK_KP_1:
		return ImGuiKey_Keypad1;
	case SDLK_KP_2:
		return ImGuiKey_Keypad2;
	case SDLK_KP_3:
		return ImGuiKey_Keypad3;
	case SDLK_KP_4:
		return ImGuiKey_Keypad4;
	case SDLK_KP_5:
		return ImGuiKey_Keypad5;
	case SDLK_KP_6:
		return ImGuiKey_Keypad6;
	case SDLK_KP_7:
		return ImGuiKey_Keypad7;
	case SDLK_KP_8:
		return ImGuiKey_Keypad8;
	case SDLK_KP_9:
		return ImGuiKey_Keypad9;
	case SDLK_KP_PERIOD:
		return ImGuiKey_KeypadDecimal;
	case SDLK_KP_DIVIDE:
		return ImGuiKey_KeypadDivide;
	case SDLK_KP_MULTIPLY:
		return ImGuiKey_KeypadMultiply;
	case SDLK_KP_MINUS:
		return ImGuiKey_KeypadSubtract;
	case SDLK_KP_PLUS:
		return ImGuiKey_KeypadAdd;
	case SDLK_KP_ENTER:
		return ImGuiKey_KeypadEnter;
	default:
		return ImGuiKey_None;
	}
}

static void highlightBindingKeys(int32_t sdlKey, int16_t modifier) {
	ImKeyboard::ClearHighlights();

	// Highlight main key
	ImGuiKey mainKey = sdlKeyToImGuiKey(sdlKey);
	if (mainKey != ImGuiKey_None) {
		ImKeyboard::Highlight(mainKey, true);
	}

	// Highlight modifier keys
	if (modifier & KMOD_LSHIFT) {
		ImKeyboard::Highlight(ImGuiKey_LeftShift, true);
	}
	if (modifier & KMOD_RSHIFT) {
		ImKeyboard::Highlight(ImGuiKey_RightShift, true);
	}
	if (modifier & KMOD_LCTRL) {
		ImKeyboard::Highlight(ImGuiKey_LeftCtrl, true);
	}
	if (modifier & KMOD_RCTRL) {
		ImKeyboard::Highlight(ImGuiKey_RightCtrl, true);
	}
	if (modifier & KMOD_LALT) {
		ImKeyboard::Highlight(ImGuiKey_LeftAlt, true);
	}
	if (modifier & KMOD_RALT) {
		ImKeyboard::Highlight(ImGuiKey_RightAlt, true);
	}
	if (modifier & KMOD_LGUI) {
		ImKeyboard::Highlight(ImGuiKey_LeftSuper, true);
	}
	if (modifier & KMOD_RGUI) {
		ImKeyboard::Highlight(ImGuiKey_RightSuper, true);
	}
	// Handle combined modifiers (e.g., KMOD_SHIFT = KMOD_LSHIFT | KMOD_RSHIFT)
	if ((modifier & KMOD_SHIFT) == KMOD_SHIFT) {
		ImKeyboard::Highlight(ImGuiKey_LeftShift, true);
	}
	if ((modifier & KMOD_CTRL) == KMOD_CTRL) {
		ImKeyboard::Highlight(ImGuiKey_LeftCtrl, true);
	}
	if ((modifier & KMOD_ALT) == KMOD_ALT) {
		ImKeyboard::Highlight(ImGuiKey_LeftAlt, true);
	}
	if ((modifier & KMOD_GUI) == KMOD_GUI) {
		ImKeyboard::Highlight(ImGuiKey_LeftSuper, true);
	}
}

static core::String buildBindingStringFromRecordedKeys(const ImVector<ImGuiKey> &keys) {
	core::String result;
	bool hasModifier = false;

	// First, extract modifiers
	for (int i = 0; i < keys.Size; ++i) {
		ImGuiKey key = keys[i];
		const char *modName = nullptr;
		switch (key) {
		case ImGuiKey_LeftShift:
		case ImGuiKey_RightShift:
			modName = "shift";
			break;
		case ImGuiKey_LeftCtrl:
		case ImGuiKey_RightCtrl:
			modName = "ctrl";
			break;
		case ImGuiKey_LeftAlt:
		case ImGuiKey_RightAlt:
			modName = "alt";
			break;
		case ImGuiKey_LeftSuper:
		case ImGuiKey_RightSuper:
			modName = "gui";
			break;
		default:
			break;
		}
		if (modName) {
			if (!result.empty()) {
				result.append("+");
			}
			result.append(modName);
			hasModifier = true;
		}
	}

	// Then, find the main key (non-modifier)
	for (int i = 0; i < keys.Size; ++i) {
		ImGuiKey key = keys[i];
		// Skip modifiers
		if (key == ImGuiKey_LeftShift || key == ImGuiKey_RightShift || key == ImGuiKey_LeftCtrl ||
			key == ImGuiKey_RightCtrl || key == ImGuiKey_LeftAlt || key == ImGuiKey_RightAlt ||
			key == ImGuiKey_LeftSuper || key == ImGuiKey_RightSuper) {
			continue;
		}

		// Get the key name from ImGui
		const char *keyName = ImGui::GetKeyName(key);
		if (keyName && keyName[0] != '\0') {
			if (hasModifier) {
				result.append("+");
			}
			core::String keyStr(keyName);
			result.append(keyStr.toLower());
			break; // Only take the first non-modifier key
		}
	}

	return result;
}

void BindingsDialog::reset() {
	if (_selectedBindingIndex >= 0 || _recordingBinding) {
		ImKeyboard::ClearHighlights();
		ImKeyboard::ClearRecorded();
		_selectedBindingIndex = -1;
		_recordingBinding = false;
		_recordingCommand.clear();
		_recordingContext.clear();
		_recordingOldBinding.clear();
	}
}

void BindingsDialog::render(bool &show, util::KeyBindingHandler &keybindingHandler,
							video::KeyboardLayout keyboardLayout, const core::DynamicArray<core::String> &uiKeyMaps,
							const core::VarPtr &uiKeyMap, bool &resetKeybindings,
							command::CommandExecutionListener &lastExecutedCommand) {
	// Clear highlights when dialog is closed
	if (!show) {
		reset();
		return;
	}

	if (ImGui::Begin(_("Bindings"), &show, ImGuiWindowFlags_AlwaysAutoResize)) {
		const util::BindMap &bindings = keybindingHandler.bindings();

		// Show recording status
		if (_recordingBinding) {
			ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), _("Recording new binding for: %s"),
							   _recordingCommand.c_str());
			ImGui::TextUnformatted(_("Press keys to set new binding, then click 'Apply' or press Escape to cancel"));

			const ImVector<ImGuiKey> &recordedKeys = ImKeyboard::GetRecordedKeys();
			if (!recordedKeys.empty()) {
				core::String newBinding = buildBindingStringFromRecordedKeys(recordedKeys);
				ImGui::Text(_("New binding: %s"), newBinding.c_str());
			}

			if (ImGui::Button(_("Apply"))) {
				const ImVector<ImGuiKey> &recorded = ImKeyboard::GetRecordedKeys();
				if (!recorded.empty()) {
					core::String newBinding = buildBindingStringFromRecordedKeys(recorded);
					if (!newBinding.empty()) {
						// Unbind old key if it exists
						if (!_recordingOldBinding.empty()) {
							command::executeCommands(core::String::format("unbind \"%s\" %s",
																		  _recordingOldBinding.c_str(),
																		  _recordingContext.c_str()),
													 &lastExecutedCommand);
						}
						// Bind new key
						command::executeCommands(core::String::format("bind \"%s\" \"%s\" %s", newBinding.c_str(),
																	  _recordingCommand.c_str(),
																	  _recordingContext.c_str()),
												 &lastExecutedCommand);
					}
				}
				_recordingBinding = false;
				_recordingCommand.clear();
				_recordingContext.clear();
				_recordingOldBinding.clear();
				ImKeyboard::ClearRecorded();
			}
			ImGui::SameLine();
			if (ImGui::Button(_("Cancel")) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
				_recordingBinding = false;
				_recordingCommand.clear();
				_recordingContext.clear();
				_recordingOldBinding.clear();
				ImKeyboard::ClearRecorded();
			}
			ImGui::Separator();
		}

		static const uint32_t TableFlags = ImGuiTableFlags_Reorderable | ImGuiTableFlags_Resizable |
										   ImGuiTableFlags_Hideable | ImGuiTableFlags_BordersInner |
										   ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY;
		const ImVec2 outerSize(0.0f, ImGui::Height(20.0f));
		if (ImGui::BeginTable("##bindingslist", 5, TableFlags, outerSize)) {
			ImGui::TableSetupColumn(_("Keys"), ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn(_("Command"), ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn(_("Context"), ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn(_("Description"), ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("##actions", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableHeadersRow();

			int n = 0;
			int currentIndex = 0;
			int32_t selectedKey = 0;
			int16_t selectedModifier = 0;
			bool selectionChanged = false;

			for (util::BindMap::const_iterator i = bindings.begin(); i != bindings.end(); ++i, ++currentIndex) {
				const util::CommandModifierPair &pair = i->second;
				const core::String &command = pair.command;
				const core::String &keyBinding =
					util::KeyBindingHandler::toString(i->first, i->second.modifier, pair.count);
				const command::Command *cmd = nullptr;
				if (command.contains(" ")) {
					cmd = command::Command::getCommand(command.substr(0, command.find(" ")));
				} else {
					cmd = command::Command::getCommand(command);
				}
				if (_bindingsFilter.size() >= 2u) {
					const bool matchCmd = core::string::icontains(command, _bindingsFilter);
					const bool matchKey = core::string::icontains(keyBinding, _bindingsFilter);
					const bool matchHelp = cmd ? core::string::icontains(cmd->help(), _bindingsFilter) : true;
					if (!matchCmd && !matchKey && !matchHelp) {
						continue;
					}
				}

				ImGui::TableNextRow();

				// Make the row selectable
				ImGui::TableNextColumn();
				bool isSelected = (_selectedBindingIndex == currentIndex);
				const core::String selectableId = core::String::format("##row-%i", n);

				// Use Selectable spanning all columns for row selection
				if (ImGui::Selectable(selectableId.c_str(), isSelected,
									  ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick)) {
					if (_selectedBindingIndex != currentIndex) {
						_selectedBindingIndex = currentIndex;
						selectionChanged = true;
						selectedKey = i->first;
						selectedModifier = i->second.modifier;
					}
					// Double-click to start recording
					if (ImGui::IsMouseDoubleClicked(0) && !_recordingBinding) {
						_recordingBinding = true;
						_recordingCommand = command;
						_recordingContext = core::bindingContextString(pair.context);
						_recordingOldBinding = keyBinding;
						ImKeyboard::ClearRecorded();
					}
				}

				// Track selection for highlighting
				if (isSelected && !selectionChanged) {
					selectedKey = i->first;
					selectedModifier = i->second.modifier;
				}

				ImGui::SameLine();
				ImGui::TextUnformatted(keyBinding.c_str());

				ImGui::TableNextColumn();
				ImGui::TextUnformatted(command.c_str());

				ImGui::TableNextColumn();
				ImGui::TextUnformatted(core::bindingContextString(pair.context).c_str());

				ImGui::TableNextColumn();
				if (!cmd) {
					ImGui::TextColored(color::Red(), _("Failed to get command for %s"), command.c_str());
				} else {
					ImGui::TextUnformatted(cmd->help().c_str());
				}

				ImGui::TableNextColumn();
				const core::String &deleteButton = core::String::format(ICON_LC_TRASH "##del-key-%i", n);
				if (ImGui::Button(deleteButton.c_str())) {
					command::executeCommands(core::String::format("unbind \"%s\" %s", keyBinding.c_str(),
																  core::bindingContextString(pair.context).c_str()),
											 &lastExecutedCommand);
					if (_selectedBindingIndex == currentIndex) {
						_selectedBindingIndex = -1;
						ImKeyboard::ClearHighlights();
					}
				}
				ImGui::TooltipTextUnformatted(_("Delete binding"));
				++n;
			}

			ImGui::EndTable();

			// Update keyboard highlights based on selection
			if (_selectedBindingIndex >= 0 && !_recordingBinding) {
				highlightBindingKeys(selectedKey, selectedModifier);
			} else if (!_recordingBinding) {
				ImKeyboard::ClearHighlights();
			}
		}

		if (!uiKeyMaps.empty()) {
			ImGui::ComboVar(uiKeyMap, uiKeyMaps);
		} else {
			if (ImGui::Button(_("Reset to default"))) {
				resetKeybindings = true;
				_selectedBindingIndex = -1;
				ImKeyboard::ClearHighlights();
			}
		}
		ImGui::SameLine();
		ImGui::InputText(_("Filter"), &_bindingsFilter);

		ImKeyboard::ImGuiKeyboardLayout layout = ImKeyboard::ImGuiKeyboardLayout_Qwerty;
		switch (keyboardLayout) {
		case video::KeyboardLayout::QWERTY:
			layout = ImKeyboard::ImGuiKeyboardLayout_Qwerty;
			break;
		case video::KeyboardLayout::AZERTY:
			layout = ImKeyboard::ImGuiKeyboardLayout_Azerty;
			break;
		case video::KeyboardLayout::QWERTZ:
			layout = ImKeyboard::ImGuiKeyboardLayout_Qwertz;
			break;
		case video::KeyboardLayout::COLEMAK:
			layout = ImKeyboard::ImGuiKeyboardLayout_Colemak;
			break;
		case video::KeyboardLayout::DVORAK:
			layout = ImKeyboard::ImGuiKeyboardLayout_Dvorak;
			break;
		default:
			break; // keep default
		}

		ImKeyboard::ImGuiKeyboardFlags keyboardFlags = ImKeyboard::ImGuiKeyboardFlags_ShowPressed |
													   ImKeyboard::ImGuiKeyboardFlags_ShowBothLabels |
													   ImKeyboard::ImGuiKeyboardFlags_ShowIcons;
		if (_recordingBinding) {
			keyboardFlags |= ImKeyboard::ImGuiKeyboardFlags_Recordable;
		}
		ImKeyboard::Keyboard(layout, keyboardFlags);
	}
	ImGui::End();
}

} // namespace ui
