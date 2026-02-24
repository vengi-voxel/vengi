/**
 * @file
 */

#include "ScriptPanel.h"
#include "command/CommandHandler.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "ui/IMGUIApp.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "voxedit-util/SceneManager.h"
#include "voxelgenerator/LUAApi.h"
#include "voxelui/LUAApiWidget.h"

#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace voxedit {

namespace priv {
struct ScriptPanelExecutorContext : public voxelui::LUAApiExecutorContext {
	SceneManagerPtr _sceneMgr;

	ScriptPanelExecutorContext(const SceneManagerPtr &sceneMgr, command::CommandExecutionListener &listenerRef)
		: _sceneMgr(sceneMgr) {
		listener = &listenerRef;
		isRunning = _sceneMgr->isScriptRunning();
	}
	virtual void runScript(const core::String &script, const core::DynamicArray<core::String> &args) {
		_sceneMgr->runScript(script, args);
	}
};
} // namespace priv

void ScriptPanel::update(const char *id, command::CommandExecutionListener &listener) {
	core_trace_scoped(ScriptPanel);
	const core::String title = makeTitle(ICON_LC_CODE, _("Scripts"), id);
	if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_MenuBar)) {
		voxelgenerator::LUAApi &luaApi = _sceneMgr->luaApi();
		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginIconMenu(ICON_LC_FILE, _("File"))) {
				if (ImGui::IconMenuItem(ICON_LC_SQUARE, _("New"))) {
					const core::String savePath = _app->filesystem()->homeWritePath("scripts");
					_app->filesystem()->sysCreateDir(savePath);
					_app->saveDialog([&](const core::String &file, const io::FormatDescription *desc) {
						if (_app->filesystem()->sysWrite(file, _luaApiWidget._activeScript)) {
							_luaApiWidget.clear();
						}
					}, {}, io::format::lua(), core::string::path(savePath, "new_script.lua"));
				}
				ImGui::TooltipTextUnformatted(_("Create a new lua script"));

				if (_luaApiWidget.currentScript().valid) {
					if (ImGui::IconMenuItem(ICON_LC_FILE_INPUT, _("Edit script"))) {
						_scriptEditor = true;
						_activeScriptFilename = _luaApiWidget.currentScript().filename;
						_textEditor.SetText(_luaApiWidget._activeScript.c_str());
					}
					ImGui::TooltipTextUnformatted(_("Edit the selected lua script"));
					if (ImGui::IconMenuItem(ICON_LC_LOADER_CIRCLE, _("Reload"))) {
						_luaApiWidget.reloadCurrentScript(luaApi);
					}
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginIconMenu(ICON_LC_LIGHTBULB, _("Help"))) {
				// TODO: open the embedded manual
				ImGui::URLIconButton(ICON_LC_BOOK, _("Scripting manual"), "https://vengi-voxel.github.io/vengi/LUAScript/");
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		priv::ScriptPanelExecutorContext ctx(_sceneMgr, listener);
		_luaApiWidget.updateScriptExecutionPanel(luaApi, _sceneMgr->activePalette(), ctx,
																	voxelui::LUAAPI_WIDGET_FLAG_RUN);
	}
	ImGui::End();
}

bool ScriptPanel::updateEditor(const char *id) {
	if (!_scriptEditor) {
		return false;
	}
	core_trace_scoped(ScriptEditor);
	const core::String title = makeTitle(ICON_LC_CODE, _("Script Editor"), id);
	if (ImGui::Begin(title.c_str(), &_scriptEditor, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_HorizontalScrollbar)) {
		voxelgenerator::LUAApi &luaApi = _sceneMgr->luaApi();
		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginIconMenu(ICON_LC_FILE, _("File"))) {
				if (ImGui::IconMenuItem(ICON_LC_LOADER_CIRCLE, _("Reload"))) {
					_luaApiWidget.reloadCurrentScript(luaApi);
				}
				if (!_activeScriptFilename.empty()) {
					if (ImGui::IconMenuItem(ICON_LC_SAVE, _("Save"))) {
						if (_app->filesystem()->homeWrite(core::string::path("scripts", _activeScriptFilename),
														  _textEditor.GetText())) {
							_luaApiWidget.reloadScriptParameters(luaApi, _luaScript, _textEditor.GetText());
						}
					}
					ImGui::TooltipText(_("Overwrite scripts/%s"), _activeScriptFilename.c_str());
				}
				if (ImGui::IconMenuItem(ICON_LC_SAVE, _("Save as"))) {
					core::Var::getVar(cfg::UILastDirectory)->setVal("scripts/");
					const core::String savePath = _app->filesystem()->homeWritePath("scripts");
					_app->saveDialog(
						[&](const core::String &file, const io::FormatDescription *desc) {
							if (_app->filesystem()->homeWrite(file, _textEditor.GetText())) {
								_luaApiWidget.clear();
								Log::info("Saved script to %s", file.c_str());
							} else {
								Log::warn("Failed to save script %s", file.c_str());
							}
						},
						{}, io::format::lua(), core::string::path(savePath, _activeScriptFilename));
				}
				if (ImGui::IconMenuItem(ICON_LC_X, _("Close"))) {
					_scriptEditor = false;
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginIconMenu(ICON_LC_PENCIL, _("Edit"))) {
				if (ImGui::IconMenuItem(ICON_LC_UNDO, _("Undo"), nullptr, false, _textEditor.CanUndo())) {
					_textEditor.Undo();
				}
				if (ImGui::IconMenuItem(ICON_LC_REDO, _("Redo"), nullptr, false, _textEditor.CanRedo())) {
					_textEditor.Redo();
				}

				ImGui::Separator();

				if (ImGui::IconMenuItem(ICON_LC_CLIPBOARD_COPY, _("Copy"), nullptr, false, _textEditor.HasSelection())) {
					_textEditor.Copy();
				}
				if (ImGui::IconMenuItem(ICON_LC_SCISSORS, _("Cut"), nullptr, false, _textEditor.HasSelection())) {
					_textEditor.Cut();
				}
				if (ImGui::IconMenuItem(ICON_LC_DELETE, _("Delete"), nullptr, false, _textEditor.HasSelection())) {
					_textEditor.Delete();
				}
				if (ImGui::IconMenuItem(ICON_LC_CLIPBOARD_PASTE, _("Paste"), nullptr, false,
										ImGui::GetClipboardText() != nullptr)) {
					_textEditor.Paste();
				}

				ImGui::Separator();

				if (ImGui::MenuItem(_("Select all"), nullptr, nullptr)) {
					_textEditor.SetSelection(TextEditor::Coordinates(),
											 TextEditor::Coordinates(_textEditor.GetTotalLines(), 0));
				}

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		_textEditor.Render(id);
	}
	ImGui::End();
	return true;
}

} // namespace voxedit
