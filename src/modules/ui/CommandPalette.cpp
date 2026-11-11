/**
 * @file
 */

#include "CommandPalette.h"
#include "IMGUIApp.h"
#include "IMGUIEx.h"
#include "IconsLucide.h"
#include "Panel.h"
#include "app/I18N.h"
#include "command/Command.h"
#include "command/CommandHandler.h"
#include "core/Common.h"
#include "core/StringUtil.h"
#include "core/Tokenizer.h"
#include "core/collection/StringSet.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "util/Console.h"

namespace ui {

void CommandPalette::show() {
	_requestOpen = true;
}

void CommandPalette::reset() {
	_requestOpen = false;
	_focusSearch = false;
	_selectedVisible = -1;
	_completingArgIndex = -1;
	_searchFilter.clear();
	_selectedCommand.clear();
	_freeArgs.clear();
	_entries.clear();
	_visible.clear();
	_paramValues.clear();
	_completionMatches.clear();
}

void CommandPalette::rebuildEntries(const util::Console &console) {
	_entries.clear();

	core::StringSet seenHistory;
	const core::DynamicArray<core::String> &history = console.history();
	for (int i = (int)history.size() - 1; i >= 0; --i) {
		const core::String &line = history[i];
		if (line.empty() || seenHistory.has(line)) {
			continue;
		}
		seenHistory.insert(line);
		Entry entry;
		entry.name = line;
		entry.help = _("Console history");
		entry.history = true;
		_entries.push_back(core::move(entry));
		if ((int)_entries.size() >= 20) {
			break;
		}
	}

	command::Command::visitSorted([this](const command::Command &cmd) {
		if (cmd.isInput()) {
			return;
		}
		Entry entry;
		entry.name = cmd.name();
		entry.help = cmd.help();
		entry.shortcut = imguiApp()->getKeyBindingsString(cmd.name().c_str());
		entry.history = false;
		_entries.push_back(core::move(entry));
	});

	_selectedVisible = -1;
	_selectedCommand.clear();
	_searchFilter.clear();
	_paramValues.clear();
	_freeArgs.clear();
	rebuildVisible();
}

bool CommandPalette::matchesFilter(const Entry &entry) const {
	if (_searchFilter.empty()) {
		return true;
	}
	if (core::string::icontains(entry.name, _searchFilter)) {
		return true;
	}
	return !entry.help.empty() && core::string::icontains(entry.help, _searchFilter);
}

void CommandPalette::rebuildVisible() {
	_visible.clear();
	for (int i = 0; i < (int)_entries.size(); ++i) {
		if (matchesFilter(_entries[i])) {
			_visible.push_back(i);
		}
	}
	if (_visible.empty()) {
		_selectedVisible = -1;
		_selectedCommand.clear();
		_paramValues.clear();
		return;
	}
	if (_selectedVisible < 0 || _selectedVisible >= (int)_visible.size()) {
		selectVisible(0);
	} else {
		selectVisible(_selectedVisible);
	}
}

void CommandPalette::selectVisible(int visibleIndex) {
	if (visibleIndex < 0 || visibleIndex >= (int)_visible.size()) {
		_selectedVisible = -1;
		_selectedCommand.clear();
		_paramValues.clear();
		_freeArgs.clear();
		return;
	}
	_selectedVisible = visibleIndex;
	const Entry &entry = _entries[_visible[visibleIndex]];
	if (entry.history) {
		_selectedCommand = entry.name;
		_paramValues.clear();
		_freeArgs.clear();
		_completionMatches.clear();
		return;
	}
	if (_selectedCommand == entry.name) {
		return;
	}
	_selectedCommand = entry.name;
	prepareParameters();
}

void CommandPalette::prepareParameters() {
	_paramValues.clear();
	_freeArgs.clear();
	_completionMatches.clear();
	_completingArgIndex = -1;
	const command::Command *cmd = command::Command::getCommand(_selectedCommand);
	if (cmd == nullptr) {
		return;
	}
	const core::DynamicArray<command::CommandArg> &args = cmd->args();
	_paramValues.reserve(args.size());
	for (const command::CommandArg &arg : args) {
		_paramValues.push_back(arg.defaultVal);
	}
}

void CommandPalette::onFilterEdited() {
	const int previousSelected =
		(_selectedVisible >= 0 && _selectedVisible < (int)_visible.size()) ? _visible[_selectedVisible] : -1;
	rebuildVisible();
	if (previousSelected < 0) {
		return;
	}
	for (int i = 0; i < (int)_visible.size(); ++i) {
		if (_visible[i] == previousSelected) {
			selectVisible(i);
			return;
		}
	}
	selectVisible(0);
}

bool CommandPalette::captureFilterInput(unsigned int filterId, bool filterFocused) {
	// Another widget (parameter field) is actively editing - leave keys alone.
	if (ImGui::IsAnyItemActive() && ImGui::GetActiveID() != (ImGuiID)filterId) {
		return false;
	}
	if (filterFocused) {
		return false;
	}

	ImGuiIO &io = ImGui::GetIO();
	bool stoleInput = false;
	if (io.InputQueueCharacters.Size > 0) {
		for (int n = 0; n < io.InputQueueCharacters.Size; n++) {
			const unsigned int c = (unsigned int)io.InputQueueCharacters[n];
			if (c == 0 || c < 32) {
				continue;
			}
			char utf8[5];
			ImTextCharToUtf8(utf8, c);
			_searchFilter.append(utf8);
			stoleInput = true;
		}
		io.InputQueueCharacters.resize(0);
	}

	if (ImGui::IsKeyPressed(ImGuiKey_Backspace) && !_searchFilter.empty()) {
		size_t size = _searchFilter.size();
		while (size > 0) {
			--size;
			const uint8_t byte = (uint8_t)_searchFilter[size];
			_searchFilter.erase(size, 1u);
			// Stop after removing a leading UTF-8 byte (not a continuation).
			if ((byte & 0xc0) != 0x80) {
				break;
			}
		}
		stoleInput = true;
	}

	if (!stoleInput) {
		return false;
	}

	onFilterEdited();
	ImGui::ActivateItemByID((ImGuiID)filterId);
	return true;
}

bool CommandPalette::executeSelected(command::CommandExecutionListener &listener, util::Console &console) {
	if (_selectedCommand.empty()) {
		return false;
	}

	if (_selectedVisible >= 0 && _selectedVisible < (int)_visible.size()) {
		const Entry &entry = _entries[_visible[_selectedVisible]];
		if (entry.history) {
			if (command::executeCommands(entry.name, &listener) > 0) {
				console.addToHistory(entry.name);
				return true;
			}
			return false;
		}
	}

	core::DynamicArray<core::String> tokens;
	const command::Command *cmd = command::Command::getCommand(_selectedCommand);
	if (cmd != nullptr && !cmd->args().empty()) {
		tokens.reserve(_paramValues.size());
		for (const core::String &value : _paramValues) {
			tokens.push_back(value);
		}
	} else if (!_freeArgs.empty()) {
		core::TokenizerConfig cfg;
		cfg.skipComments = false;
		const core::Tokenizer tok(cfg, _freeArgs, " ");
		tokens = tok.tokens();
	}

	if (!listener.allowed(_selectedCommand, tokens)) {
		return false;
	}
	if (!command::Command::execute(_selectedCommand, tokens)) {
		return false;
	}
	listener(_selectedCommand, tokens);

	core::String historyLine = _selectedCommand;
	for (const core::String &token : tokens) {
		historyLine.append(" ");
		if (token.contains(" ") || token.contains("\"")) {
			historyLine.append("\"");
			historyLine.append(token);
			historyLine.append("\"");
		} else {
			historyLine.append(token);
		}
	}
	console.addToHistory(historyLine);
	return true;
}

int CommandPalette::argInputCallback(ImGuiInputTextCallbackData *data) {
	CommandPalette *palette = (CommandPalette *)data->UserData;
	if (data->EventFlag != ImGuiInputTextFlags_CallbackCompletion || palette == nullptr) {
		return 0;
	}

	const int argIndex = palette->_completingArgIndex;
	const command::Command *cmd = command::Command::getCommand(palette->_selectedCommand);
	if (cmd == nullptr || argIndex < 0) {
		return 0;
	}

	core::Tokens tokens;
	tokens.push_back(palette->_selectedCommand);
	for (size_t i = 0; i < palette->_paramValues.size(); ++i) {
		tokens.push_back(palette->_paramValues[i]);
	}

	const core::String current(data->Buf, (size_t)data->BufTextLen);
	core::DynamicArray<core::String> matches;
	int found = cmd->completeArg(argIndex, current, tokens, matches);
	if (found == 0) {
		cmd->complete(current, matches);
	}
	if (matches.empty()) {
		palette->_completionMatches.clear();
		return 0;
	}

	core::StringSet uniqueMatches;
	uniqueMatches.insert(matches.begin(), matches.end());
	matches.clear();
	for (auto i = uniqueMatches.begin(); i != uniqueMatches.end(); ++i) {
		matches.push_back(i->key);
	}
	matches.sort(core::Greater<core::String>());
	palette->_completionMatches = matches;

	core::String replacement = matches.front();
	if (matches.size() > 1) {
		int pos = 0;
		for (char c : matches.front()) {
			bool allMatch = true;
			for (const core::String &match : matches) {
				if ((int)match.size() <= pos || match[pos] != c) {
					allMatch = false;
					break;
				}
			}
			if (!allMatch) {
				break;
			}
			++pos;
		}
		if (pos <= 0) {
			return 0;
		}
		replacement = matches.front().substr(0, pos);
	}

	data->DeleteChars(0, data->BufTextLen);
	data->InsertChars(0, replacement.c_str());
	return 0;
}

void CommandPalette::renderParameterFields() {
	if (_selectedVisible >= 0 && _selectedVisible < (int)_visible.size() &&
		_entries[_visible[_selectedVisible]].history) {
		return;
	}

	const command::Command *cmd = command::Command::getCommand(_selectedCommand);
	if (cmd == nullptr || cmd->args().empty()) {
		return;
	}

	ImGui::Separator();
	ImGui::TextUnformatted(_("Parameters"));
	const core::DynamicArray<command::CommandArg> &args = cmd->args();
	for (size_t i = 0; i < args.size() && i < _paramValues.size(); ++i) {
		const command::CommandArg &arg = args[i];
		core::String label = arg.name;
		if (arg.optional) {
			label.append(" ");
			label.append(_("(optional)"));
		}
		ImGui::PushID((int)i);
		if (arg.type == command::ArgType::Bool) {
			bool value = _paramValues[i] == "true" || _paramValues[i] == "1" || _paramValues[i] == "yes" ||
						 _paramValues[i] == "on";
			if (ImGui::Checkbox(label.c_str(), &value)) {
				_paramValues[i] = value ? "true" : "false";
			}
		} else if (arg.type == command::ArgType::Int) {
			int value = _paramValues[i].toInt();
			if (ImGui::InputInt(label.c_str(), &value)) {
				_paramValues[i] = core::string::toString(value);
			}
		} else if (arg.type == command::ArgType::Float) {
			float value = core::string::toFloat(_paramValues[i]);
			if (ImGui::InputFloat(label.c_str(), &value)) {
				_paramValues[i] = core::string::toString(value);
			}
		} else {
			_completingArgIndex = (int)i;
			ImGui::InputText(label.c_str(), &_paramValues[i], ImGuiInputTextFlags_CallbackCompletion, argInputCallback,
							 this);
			ImGui::TooltipTextUnformatted(_("Press Tab to autocomplete"));
		}
		if (!arg.description.empty()) {
			ImGui::TooltipTextUnformatted(arg.description.c_str());
		}
		ImGui::PopID();
	}

	if (!_completionMatches.empty()) {
		ImGui::BeginChild("##completions", ImVec2(0.0f, ImGui::Height(4.0f)), ImGuiChildFlags_Borders);
		for (const core::String &match : _completionMatches) {
			if (ImGui::Selectable(match.c_str())) {
				if (_completingArgIndex >= 0 && _completingArgIndex < (int)_paramValues.size()) {
					_paramValues[_completingArgIndex] = match;
				}
				_completionMatches.clear();
			}
		}
		ImGui::EndChild();
	}
}

void CommandPalette::render(command::CommandExecutionListener &listener, util::Console &console) {
	if (_requestOpen) {
		rebuildEntries(console);
		ImGui::OpenPopup(POPUP_TITLE_COMMAND_PALETTE);
		_focusSearch = true;
		_requestOpen = false;
	}

	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	const float popupWidth = core_max(ImGui::Size(50.0f), viewport->WorkSize.x * 0.65f);
	const float listHeight = ImGui::Height(16.0f);
	ImGui::SetNextWindowSize(ImVec2(popupWidth, 0.0f), ImGuiCond_Appearing);
	ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.35f));

	// NoNavInputs: Up/Down must change the filtered selection, not move focus between
	// filter / list / Ok / Cancel (VS Code command palette behavior).
	const ImGuiWindowFlags popupFlags =
		ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNavInputs;
	const core::String &title = Panel::makeTitle(ICON_LC_TERMINAL, _("Command Palette"), POPUP_TITLE_COMMAND_PALETTE);
	if (ImGui::BeginPopupModal(title.c_str(), nullptr, popupFlags)) {
		if (_focusSearch) {
			ImGui::SetKeyboardFocusHere();
			_focusSearch = false;
		}

		ImGui::SetNextItemWidth(-FLT_MIN);
		const core::String filterHint =
			core::String::format("%s %s", ICON_LC_SEARCH, _("Filter commands and history"));
		const bool filterChanged =
			ImGui::InputTextWithHint("##commandpalettefilter", filterHint.c_str(), &_searchFilter);
		const ImGuiID filterId = ImGui::GetItemID();
		bool filterFocused = ImGui::IsItemActive() || ImGui::IsItemFocused();
		if (filterChanged) {
			onFilterEdited();
		}
		// Typing / Backspace while focus is elsewhere (list click, Ok/Cancel) steals
		// input back into the filter - same idea as VS Code's command palette.
		if (captureFilterInput(filterId, filterFocused)) {
			filterFocused = true;
		}

		// While a parameter widget is active, leave arrows to that widget.
		const bool editingOtherField = ImGui::IsAnyItemActive() && ImGui::GetActiveID() != filterId;
		bool selectionMoved = false;
		if (!_visible.empty() && !editingOtherField) {
			if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, true)) {
				selectVisible(core_min(_selectedVisible + 1, (int)_visible.size() - 1));
				selectionMoved = true;
			} else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true)) {
				selectVisible(core_max(_selectedVisible - 1, 0));
				selectionMoved = true;
			}
		}
		if (selectionMoved && !filterFocused) {
			ImGui::ActivateItemByID(filterId);
			filterFocused = true;
		}

		// List and buttons are mouse targets only - keyboard stays on the filter.
		ImGui::PushItemFlag(ImGuiItemFlags_NoNav | ImGuiItemFlags_NoTabStop, true);
		ImGui::BeginChild("##commandpalettelist", ImVec2(-FLT_MIN, listHeight), ImGuiChildFlags_Borders,
						  ImGuiWindowFlags_NoNavInputs);
		bool drawnHistoryHeader = false;
		bool drawnCommandHeader = false;
		for (int visibleIndex = 0; visibleIndex < (int)_visible.size(); ++visibleIndex) {
			const Entry &entry = _entries[_visible[visibleIndex]];
			if (entry.history && !drawnHistoryHeader) {
				ImGui::TextDisabled("%s", _("recently used"));
				drawnHistoryHeader = true;
			} else if (!entry.history && !drawnCommandHeader) {
				if (drawnHistoryHeader) {
					ImGui::Separator();
				}
				ImGui::TextDisabled("%s", _("commands"));
				drawnCommandHeader = true;
			}

			core::String label = entry.name;
			if (!entry.help.empty() && !entry.history) {
				label.append(" - ");
				label.append(entry.help);
			}

			float shortcutWidth = 0.0f;
			if (!entry.shortcut.empty()) {
				shortcutWidth = ImGui::CalcTextSize(entry.shortcut.c_str()).x;
			}

			const bool selected = visibleIndex == _selectedVisible;
			ImGui::PushID(visibleIndex);
			if (ImGui::Selectable("##entry", selected,
								  ImGuiSelectableFlags_AllowOverlap | ImGuiSelectableFlags_NoAutoClosePopups)) {
				selectVisible(visibleIndex);
				_focusSearch = true;
			}
			if (selected && selectionMoved) {
				ImGui::SetScrollHereY();
			}

			const ImVec2 min = ImGui::GetItemRectMin();
			const ImVec2 max = ImGui::GetItemRectMax();
			const ImGuiStyle &style = ImGui::GetStyle();
			const ImVec2 textPos(min.x + style.FramePadding.x, min.y + style.FramePadding.y);
			const float textMaxX = max.x - shortcutWidth - style.ItemSpacing.x - style.FramePadding.x;
			ImGui::RenderTextEllipsis(ImGui::GetWindowDrawList(), textPos, ImVec2(textMaxX, max.y), textMaxX,
									  label.c_str(), nullptr, nullptr);
			if (!entry.shortcut.empty()) {
				const ImVec2 shortcutPos(max.x - shortcutWidth - style.FramePadding.x, textPos.y);
				ImGui::GetWindowDrawList()->AddText(shortcutPos, ImGui::GetColorU32(ImGuiCol_TextDisabled),
													entry.shortcut.c_str());
			}
			if (!entry.help.empty()) {
				ImGui::SetItemTooltip("%s\n%s", entry.name.c_str(), entry.help.c_str());
			}
			ImGui::PopID();
		}
		ImGui::EndChild();

		renderParameterFields();

		const bool canRun = !_selectedCommand.empty();
		ImGui::BeginDisabled(!canRun);
		const bool runClicked = ImGui::OkButton();
		ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::CancelButton()) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::PopItemFlag();

		// Enter runs the selection (VS Code). Works while the filter is focused.
		const bool approve =
			runClicked || (canRun && ImGui::IsKeyPressed(ImGuiKey_Enter) && !editingOtherField);
		if (approve && executeSelected(listener, console)) {
			ImGui::CloseCurrentPopup();
		}

		if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

} // namespace ui
