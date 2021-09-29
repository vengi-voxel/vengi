/**
 * @file
 */

#include "ScriptPanel.h"
#include "voxedit-util/SceneManager.h"
#include "ui/imgui/IMGUI.h"
#include "ui/imgui/IconsForkAwesome.h"
#include "ui/imgui/IconsFontAwesome5.h"

namespace voxedit {

void ScriptPanel::reloadScriptParameters(const core::String& script) {
	_scriptParameterDescription.clear();
	sceneMgr().luaGenerator().argumentInfo(script, _scriptParameterDescription);
	const int parameterCount = _scriptParameterDescription.size();
	_scriptParameters.clear();
	_scriptParameters.resize(parameterCount);
	for (int i = 0; i < parameterCount; ++i) {
		const voxelgenerator::LUAParameterDescription &p = _scriptParameterDescription[i];
		_scriptParameters[i] = p.defaultValue;
	}
}

void ScriptPanel::update(const char *title, const char *scriptEditorTitle, ui::imgui::IMGUIApp* app) {
	if (ImGui::Begin(title)) {
		core_trace_scoped(ScriptPanel)
		if (_scripts.empty()) {
			_scripts = sceneMgr().luaGenerator().listScripts();
		}

		ImGui::TextUnformatted("LUA scripts for manipulating the voxel volumes");

		if (ImGui::ComboStl("Script", &_currentScript, _scripts)) {
			const core::String& scriptName = _scripts[_currentScript].filename;
			_activeScript = sceneMgr().luaGenerator().load(scriptName);
			reloadScriptParameters(_activeScript);
		}

		const int n = _scriptParameterDescription.size();
		for (int i = 0; i < n; ++i) {
			const voxelgenerator::LUAParameterDescription &p = _scriptParameterDescription[i];
			switch (p.type) {
			case voxelgenerator::LUAParameterType::ColorIndex: {
				const voxel::MaterialColorArray &colors = voxel::getMaterialColors();
				core::String &str = _scriptParameters[i];
				int val = core::string::toInt(str);
				if (val >= 0 && val < (int)colors.size()) {
					const float size = ImGui::Size(20);
					const ImVec2 v1(ImGui::GetWindowPos().x + ImGui::GetCursorPosX(), ImGui::GetWindowPos().y + ImGui::GetCursorPosY());
					const ImVec2 v2(v1.x + size, v1.y + size);
					ImDrawList* drawList = ImGui::GetWindowDrawList();
					drawList->AddRectFilled(v1, v2, ImGui::GetColorU32(colors[val]));
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + size);
				}
				if (ImGui::InputInt(p.name.c_str(), &val)) {
					if (val >= 0 && val < (int)colors.size()) {
						str = core::string::toString(val);
					}
				}
				break;
			}
			case voxelgenerator::LUAParameterType::Integer: {
				core::String &str = _scriptParameters[i];
				int val = core::string::toInt(str);
				if (ImGui::InputInt(p.name.c_str(), &val)) {
					str = core::string::toString(val);
				}
				break;
			}
			case voxelgenerator::LUAParameterType::Float: {
				core::String &str = _scriptParameters[i];
				float val = core::string::toFloat(str);
				if (ImGui::InputFloat(p.name.c_str(), &val)) {
					str = core::string::toString(val);
				}
				break;
			}
			case voxelgenerator::LUAParameterType::String: {
				core::String &str = _scriptParameters[i];
				ImGui::InputText(p.name.c_str(), &str);
				break;
			}
			case voxelgenerator::LUAParameterType::Boolean: {
				core::String &str = _scriptParameters[i];
				bool checked = str == "1";
				if (ImGui::Checkbox(p.name.c_str(), &checked)) {
					str = checked ? "1" : "0";
				}
				break;
			}
			case voxelgenerator::LUAParameterType::Max:
				return;
			}
			ImGui::TooltipText("%s", p.description.c_str());
		}
		const bool validScriptIndex = _currentScript >= 0 && _currentScript < (int)_scripts.size();
		const bool validScript = validScriptIndex && _scripts[_currentScript].valid;
		if (ImGui::DisabledButton("Execute##scriptpanel", !validScript)) {
			sceneMgr().runScript(_activeScript, _scriptParameters);
		}
		ImGui::TooltipText("Execute the selected script for the currently loaded voxel volumes");
		ImGui::SameLine();
		if (ImGui::Button("New##scriptpanel")) {
			_scriptEditor = true;
			_activeScriptFilename = "";
			_textEditor.SetText("");
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

		ImGui::URLButton(ICON_FA_BOOK " Scripting manual", "https://mgerhardy.github.io/engine/voxedit/LUAScript/");
	}
	ImGui::End();

	if (_scriptEditor) {
		if (ImGui::Begin(scriptEditorTitle, &_scriptEditor, ImGuiWindowFlags_MenuBar)) {
			if (ImGui::BeginMenuBar()) {
				if (ImGui::BeginMenu(ICON_FA_FILE " File##scripteditor")) {
					if (ImGui::MenuItem(ICON_FA_CHECK " Apply and execute##scripteditor")) {
						_activeScript = _textEditor.GetText();
						reloadScriptParameters(_activeScript);
					}
					if (!_activeScriptFilename.empty()) {
						if (ImGui::MenuItem(ICON_FA_SAVE " Save##scripteditor")) {
							if (app->filesystem()->write("scripts/" + _activeScriptFilename, _textEditor.GetText())) {
								_activeScript = _textEditor.GetText();
								reloadScriptParameters(_activeScript);
							}
						}
						ImGui::TooltipText("Overwrite scripts/%s", _activeScriptFilename.c_str());
					}
					if (ImGui::MenuItem(ICON_FA_SAVE " Save As##scripteditor")) {
						core::Var::getSafe(cfg::UILastDirectory)->setVal("scripts/");
						app->fileDialog(
							[&](const core::String &file) {
								if (app->filesystem()->write(file, _textEditor.GetText())) {
									_scripts.clear();
									_currentScript = -1;
									Log::info("Saved script to %s", file.c_str());
								} else {
									Log::warn("Failed to save script %s", file.c_str());
								}
							},
							video::WindowedApp::OpenFileMode::Save, "*.lua");
					}
					if (ImGui::MenuItem("Close##scripteditor")) {
						_scriptEditor = false;
					}
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu(ICON_FK_PENCIL " Edit##scripteditor")) {
					if (ImGui::MenuItem(ICON_FA_UNDO " Undo##scripteditor", nullptr, nullptr, _textEditor.CanUndo()))
						_textEditor.Undo();
					if (ImGui::MenuItem(ICON_FA_REDO " Redo##scripteditor", nullptr, nullptr, _textEditor.CanRedo()))
						_textEditor.Redo();

					ImGui::Separator();

					if (ImGui::MenuItem(ICON_FA_COPY " Copy##scripteditor", nullptr, nullptr, _textEditor.HasSelection()))
						_textEditor.Copy();
					if (ImGui::MenuItem(ICON_FA_CUT " Cut##scripteditor", nullptr, nullptr, _textEditor.HasSelection()))
						_textEditor.Cut();
					if (ImGui::MenuItem("Delete##scripteditor", nullptr, nullptr, _textEditor.HasSelection()))
						_textEditor.Delete();
					if (ImGui::MenuItem(ICON_FA_PASTE " Paste##scripteditor", nullptr, nullptr,
										ImGui::GetClipboardText() != nullptr))
						_textEditor.Paste();

					ImGui::Separator();

					if (ImGui::MenuItem("Select all##scripteditor", nullptr, nullptr))
						_textEditor.SetSelection(TextEditor::Coordinates(),
												 TextEditor::Coordinates(_textEditor.GetTotalLines(), 0));

					ImGui::EndMenu();
				}

				ImGui::EndMenuBar();
			}

			_textEditor.Render(scriptEditorTitle);
		}
		ImGui::End();
	}
}

}
