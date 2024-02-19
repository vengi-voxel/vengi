/**
 * @file
 */

#include "ScriptPanel.h"
#include "DragAndDropPayload.h"
#include "core/Algorithm.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraphNode.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "voxedit-util/SceneManager.h"
#include "voxel/Voxel.h"
#include "voxelgenerator/LUAGenerator.h"

#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace voxedit {

void ScriptPanel::reloadScriptParameters(const core::String &script) {
	voxelgenerator::LUAGenerator &luaGenerator = sceneMgr().luaGenerator();
	_scriptParameterDescription.clear();
	luaGenerator.argumentInfo(script, _scriptParameterDescription);
	const int parameterCount = (int)_scriptParameterDescription.size();
	_scriptParameters.clear();
	_scriptParameters.resize(parameterCount);
	_enumValues.clear();
	_enumValues.resize(parameterCount);
	for (int i = 0; i < parameterCount; ++i) {
		const voxelgenerator::LUAParameterDescription &p = _scriptParameterDescription[i];
		_scriptParameters[i] = p.defaultValue;
		_enumValues[i] = p.enumValues;
	}
}

bool ScriptPanel::updateScriptExecutionPanel(command::CommandExecutionListener &listener) {
	if (_scripts.empty()) {
		return false;
	}

	voxelgenerator::LUAGenerator &luaGenerator = sceneMgr().luaGenerator();
	if (ImGui::ComboItems("##script", &_currentScript, _scripts)) {
		if (_currentScript >= 0 && _currentScript < (int)_scripts.size()) {
			const core::String &scriptName = _scripts[_currentScript].filename;
			_activeScript = luaGenerator.load(scriptName);
			reloadScriptParameters(_activeScript);
		}
	}
	ImGui::TooltipText("LUA scripts for manipulating the voxel volumes");

	ImGui::SameLine();

	const bool validScriptIndex = _currentScript >= 0 && _currentScript < (int)_scripts.size();
	const bool validScript = validScriptIndex && _scripts[_currentScript].valid;
	if (ImGui::DisabledButton("Run##scriptpanel", !validScript)) {
		sceneMgr().runScript(_activeScript, _scriptParameters);
		core::DynamicArray<core::String> args;
		args.reserve(_scriptParameters.size() + 1);
		args.push_back(_scripts[_currentScript].filename);
		args.append(_scriptParameters);
		listener("xs", _scriptParameters);
	}
	ImGui::TooltipText("Execute the selected script for the currently loaded voxel volumes");

	const int n = (int)_scriptParameterDescription.size();
	if (n && ImGui::CollapsingHeader("Script parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
		for (int i = 0; i < n; ++i) {
			const voxelgenerator::LUAParameterDescription &p = _scriptParameterDescription[i];
			switch (p.type) {
			case voxelgenerator::LUAParameterType::ColorIndex: {
				const palette::Palette &palette = sceneMgr().activePalette();
				core::String &str = _scriptParameters[i];
				int val = core::string::toInt(str);
				if (val >= 0 && val < palette.colorCount()) {
					const float size = 20;
					const ImVec2 v1 = ImGui::GetCursorScreenPos();
					const ImVec2 v2(v1.x + size, v1.y + size);
					ImDrawList *drawList = ImGui::GetWindowDrawList();
					drawList->AddRectFilled(v1, v2, ImGui::GetColorU32(palette.color(val)));
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + size);
				}
				if (ImGui::InputInt(p.name.c_str(), &val)) {
					if (val >= 0 && val < palette.colorCount()) {
						str = core::string::toString(val);
					}
				}

				if (ImGui::BeginDragDropTarget()) {
					if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(dragdrop::PaletteIndexPayload)) {
						const int palIdx = *(const uint8_t *)payload->Data;
						str = core::string::toString(palIdx);
					}
					ImGui::EndDragDropTarget();
				}

				break;
			}
			case voxelgenerator::LUAParameterType::Integer: {
				core::String &str = _scriptParameters[i];
				int val = core::string::toInt(str);
				if (p.shouldClamp()) {
					int maxVal = (int)(p.maxValue + glm::epsilon<double>());
					int minVal = (int)(p.minValue + glm::epsilon<double>());
					if (ImGui::SliderInt(p.name.c_str(), &val, minVal, maxVal)) {
						str = core::string::toString(val);
					}
				} else if (ImGui::InputInt(p.name.c_str(), &val)) {
					str = core::string::toString(val);
				}
				break;
			}
			case voxelgenerator::LUAParameterType::Float: {
				core::String &str = _scriptParameters[i];
				float val = core::string::toFloat(str);
				if (p.shouldClamp()) {
					const float maxVal = (float)p.maxValue;
					const float minVal = (float)p.minValue;
					const char *format;
					if (glm::abs(maxVal - minVal) <= 10.0f) {
						format = "%.6f";
					} else {
						format = "%.3f";
					}

					if (ImGui::SliderFloat(p.name.c_str(), &val, minVal, maxVal, format)) {
						str = core::string::toString(val);
					}
				} else if (ImGui::InputFloat(p.name.c_str(), &val)) {
					str = core::string::toString(val);
				}
				break;
			}
			case voxelgenerator::LUAParameterType::String: {
				core::String &str = _scriptParameters[i];
				ImGui::InputText(p.name.c_str(), &str);
				break;
			}
			case voxelgenerator::LUAParameterType::Enum: {
				core::String &str = _scriptParameters[i];
				core::DynamicArray<core::String> tokens;
				core::string::splitString(_enumValues[i], tokens, ",");
				const auto i = core::find(tokens.begin(), tokens.end(), str);
				int selected = i == tokens.end() ? 0 : i - tokens.begin();
				if (ImGui::ComboItems(p.name.c_str(), &selected, tokens)) {
					str = tokens[selected];
				}
				break;
			}
			case voxelgenerator::LUAParameterType::Boolean: {
				core::String &str = _scriptParameters[i];
				bool checked = core::string::toBool(str);
				if (ImGui::Checkbox(p.name.c_str(), &checked)) {
					str = checked ? "1" : "0";
				}
				break;
			}
			case voxelgenerator::LUAParameterType::Max:
				return validScriptIndex;
			}
			ImGui::TooltipText("%s", p.description.c_str());
		}
	}
	return validScriptIndex;
}

void ScriptPanel::update(const char *title, command::CommandExecutionListener &listener) {
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		voxelgenerator::LUAGenerator &luaGenerator = sceneMgr().luaGenerator();
		if (_scripts.empty()) {
			_scripts = luaGenerator.listScripts();
		}
		bool validScriptIndex = updateScriptExecutionPanel(listener);
		if (ImGui::Button("New##scriptpanel")) {
			_scriptEditor = true;
			_activeScriptFilename = "";
			if (!_scripts.empty()) {
				const core::String &scriptName = _scripts[0].filename;
				const core::String &script = luaGenerator.load(scriptName);
				_textEditor.SetText(script);
			} else {
				_textEditor.SetText("");
			}
		}
		ImGui::TooltipText("Create a new lua script");
		if (validScriptIndex) {
			ImGui::SameLine();
			if (ImGui::Button("Edit##scriptpanel")) {
				_scriptEditor = true;
				_activeScriptFilename = _scripts[_currentScript].filename;
				_textEditor.SetText(_activeScript.c_str());
			}
			ImGui::TooltipText("Edit the selected lua script");
		}

		ImGui::URLIconButton(ICON_LC_BOOK, "Scripting manual", "https://vengi-voxel.github.io/vengi/LUAScript/");
	}
	ImGui::End();
}

bool ScriptPanel::updateEditor(const char *title, ui::IMGUIApp *app) {
	if (!_scriptEditor) {
		return false;
	}
	if (ImGui::Begin(title, &_scriptEditor, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_HorizontalScrollbar)) {
		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginIconMenu(ICON_LC_FILE, _("File##scripteditor"))) {
				if (ImGui::IconMenuItem(ICON_LC_CHECK, _("Apply and execute##scripteditor"))) {
					_activeScript = _textEditor.GetText();
					reloadScriptParameters(_activeScript);
				}
				if (!_activeScriptFilename.empty()) {
					if (ImGui::IconMenuItem(ICON_LC_SAVE, _("Save##scripteditor"))) {
						if (app->filesystem()->write(core::string::path("scripts", _activeScriptFilename),
													 _textEditor.GetText())) {
							_activeScript = _textEditor.GetText();
							reloadScriptParameters(_activeScript);
						}
					}
					ImGui::TooltipText(_("Overwrite scripts/%s"), _activeScriptFilename.c_str());
				}
				if (ImGui::IconMenuItem(ICON_LC_SAVE, _("Save As##scripteditor"))) {
					core::Var::getSafe(cfg::UILastDirectory)->setVal("scripts/");
					app->saveDialog(
						[&](const core::String &file, const io::FormatDescription *desc) {
							if (app->filesystem()->write(file, _textEditor.GetText())) {
								_scripts.clear();
								_currentScript = -1;
								Log::info("Saved script to %s", file.c_str());
							} else {
								Log::warn("Failed to save script %s", file.c_str());
							}
						},
						{}, io::format::lua());
				}
				if (ImGui::MenuItem(_("Close##scripteditor"))) {
					_scriptEditor = false;
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginIconMenu(ICON_LC_PENCIL, _("Edit##scripteditor"))) {
				if (ImGui::IconMenuItem(ICON_LC_ROTATE_CCW, _("Undo##scripteditor"), nullptr, false,
									_textEditor.CanUndo())) {
					_textEditor.Undo();
				}
				if (ImGui::IconMenuItem(ICON_LC_ROTATE_CW, _("Redo##scripteditor"), nullptr, false, _textEditor.CanRedo())) {
					_textEditor.Redo();
				}

				ImGui::Separator();

				if (ImGui::IconMenuItem(ICON_LC_COPY, _("Copy##scripteditor"), nullptr, false, _textEditor.HasSelection())) {
					_textEditor.Copy();
				}
				if (ImGui::IconMenuItem(ICON_LC_SCISSORS, _("Cut##scripteditor"), nullptr, false,
									_textEditor.HasSelection())) {
					_textEditor.Cut();
				}
				if (ImGui::MenuItem(_("Delete##scripteditor"), nullptr, nullptr, _textEditor.HasSelection())) {
					_textEditor.Delete();
				}
				if (ImGui::IconMenuItem(ICON_LC_CLIPBOARD_PASTE, _("Paste##scripteditor"), nullptr, false,
									ImGui::GetClipboardText() != nullptr)) {
					_textEditor.Paste();
				}

				ImGui::Separator();

				if (ImGui::MenuItem(_("Select all##scripteditor"), nullptr, nullptr)) {
					_textEditor.SetSelection(TextEditor::Coordinates(),
											 TextEditor::Coordinates(_textEditor.GetTotalLines(), 0));
				}

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		_textEditor.Render(title);
	}
	ImGui::End();
	return true;
}

} // namespace voxedit
