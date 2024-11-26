/**
 * @file
 */

#include "VoxConvertUI.h"
#include "IMGUIStyle.h"
#include "io/FilesystemEntry.h"
#include "ui/IconsLucide.h"
#include "PopupAbout.h"
#include "app/App.h"
#include "command/CommandHandler.h"
#include "core/Process.h"
#include "core/StringUtil.h"
#include "io/Archive.h"
#include "io/BufferedReadWriteStream.h"
#include "io/FormatDescription.h"
#include "ui/IMGUIEx.h"
#include "voxelformat/FormatConfig.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelui/FileDialogOptions.h"

VoxConvertUI::VoxConvertUI(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider)
	: Super(filesystem, timeProvider), _paletteCache(filesystem) {
	// use the same config as voxconvert
	init(ORGANISATION, "voxconvert");
	_allowRelativeMouseMode = false;
	_wantCrashLogs = true;
	_fullScreenApplication = false;
	_showConsole = false;
	_windowWidth = 1024;
	_windowHeight = 820;
}

app::AppState VoxConvertUI::onConstruct() {
	app::AppState state = Super::onConstruct();
	voxelformat::FormatConfig::init();

	_lastTarget = core::Var::get("ui_lasttarget", "0", "Last selected target format");
	_lastSource = core::Var::get("ui_lastsource", "0", "Last selected target format");

	return state;
}

app::AppState VoxConvertUI::onInit() {
	app::AppState state = Super::onInit();
	_voxconvertBinary = _filesystem->sysFindBinary("vengi-voxconvert");

	if (_filterFormatTextWidth < 0.0f) {
		for (const io::FormatDescription *desc = voxelformat::voxelLoad(); desc->valid(); ++desc) {
			_filterLoadEntries.push_back(*desc);
		}
		_filterLoadEntries.sort(core::Greater<io::FormatDescription>());

		for (const io::FormatDescription *desc = voxelformat::voxelSave(); desc->valid(); ++desc) {
			_filterSaveEntries.push_back(*desc);
		}
		_filterSaveEntries.sort(core::Greater<io::FormatDescription>());
	}
	if (_lastTarget->intVal() < 0 || _lastTarget->intVal() >= (int)_filterSaveEntries.size()) {
		_lastTarget->setVal(0);
	}
	if (_lastSource->intVal() < 0 || _lastSource->intVal() >= (int)_filterLoadEntries.size()) {
		_lastSource->setVal(0);
	}

	_paletteCache.detectPalettes();

	return state;
}

void VoxConvertUI::onRenderUI() {
	ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);

	if (_filterFormatTextWidth < 0.0f) {
		for (const io::FormatDescription &desc : _filterLoadEntries) {
			const core::String &str = io::convertToFilePattern(desc);
			const ImVec2 filterTextSize = ImGui::CalcTextSize(str.c_str());
			_filterFormatTextWidth = core_max(_filterFormatTextWidth, filterTextSize.x);
		}
	}

	if (ImGui::Begin("##main", nullptr,
					 ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
						 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
						 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
						 ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav)) {
		if (ImGui::BeginMenuBar()) {
			core_trace_scoped(MenuBar);
			if (ImGui::BeginIconMenu(ICON_LC_FILE, _("File"))) {
				ImGui::Separator();
				if (ImGui::IconMenuItem(ICON_LC_DOOR_CLOSED, _("Quit"))) {
					requestQuit();
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginIconMenu(ICON_LC_MENU, _("Edit"))) {
				if (ImGui::BeginIconMenu(ICON_LC_MENU, _("Options"))) {
					ui::metricOption();
					languageOption();
					ImGui::CheckboxVar(_("Allow multi monitor"), cfg::UIMultiMonitor);
					ImGui::InputVarInt(_("Font size"), cfg::UIFontSize, 1, 5);
					const core::Array<core::String, ImGui::MaxStyles> uiStyles = {_("CorporateGrey"), _("Dark"),
																				  _("Light"), _("Classic")};
					ImGui::ComboVar(_("Color theme"), cfg::UIStyle, uiStyles);
					ImGui::InputVarFloat(_("Notifications"), cfg::UINotifyDismissMillis);
					ImGui::Checkbox(_("Show console"), &_showConsole);
					ImGui::EndMenu();
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginIconMenu(ICON_LC_CIRCLE_HELP, _("Help"))) {
				if (ImGui::IconMenuItem(ICON_LC_INFO, _("About"))) {
					ImGui::OpenPopup(POPUP_TITLE_ABOUT);
				}
				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		ImGui::PushItemWidth(_filterFormatTextWidth);
		const core::String sourceStr = io::convertToFilePattern(_filterLoadEntries[_lastSource->intVal()]);
		if (ImGui::BeginCombo(_("Source format"), sourceStr.c_str())) {
			for (int i = 0; i < (int)_filterLoadEntries.size(); ++i) {
				const bool selected = i == _lastSource->intVal();
				const io::FormatDescription &format = _filterLoadEntries[i];
				const core::String &text = io::convertToFilePattern(format);
				if (ImGui::Selectable(text.c_str(), selected)) {
					_lastSource->setVal(i);
					_sourceFile = "";
					_dirtyTargetFile = true;
				}
				if (selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::PopItemWidth();

		io::FormatDescription sourceEntries[] = {_filterLoadEntries[_lastSource->intVal()], {"", {}, {}, 0u}};
		ImGui::InputFile(_("Source"), &_sourceFile, sourceEntries);
		if (_lastSourceFile != _sourceFile) {
			_lastSourceFile = _sourceFile;
			_dirtyTargetFile = true;
		}

		ImGui::PushItemWidth(_filterFormatTextWidth);
		const core::String targetStr = io::convertToFilePattern(_filterSaveEntries[_lastTarget->intVal()]);
		if (ImGui::BeginCombo(_("Target format"), targetStr.c_str())) {
			for (int i = 0; i < (int)_filterSaveEntries.size(); ++i) {
				const bool selected = i == _lastTarget->intVal();
				const io::FormatDescription &format = _filterSaveEntries[i];
				const core::String &text = io::convertToFilePattern(format);
				if (ImGui::Selectable(text.c_str(), selected)) {
					_lastTarget->setVal(i);
					_dirtyTargetFile = true;
				}
				if (selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::PopItemWidth();

		ImGui::Text(_("Target file: %s"), _targetFile.c_str());

		if (_dirtyTargetFile) {
			if (_sourceFile.empty()) {
				_targetFile = "";
				_targetFileExists = false;
			} else {
				_targetFile =
					core::string::replaceExtension(_sourceFile, _filterSaveEntries[_lastTarget->intVal()].mainExtension());
				Log::error("Target file: %s", _targetFile.c_str());
				_targetFileExists = _filesystem->exists(_targetFile);
			}
			_dirtyTargetFile = false;
		}

		if (ImGui::CollapsingHeader(_("Options"), ImGuiTreeNodeFlags_DefaultOpen)) {
			if (_targetFileExists) {
				ImGui::IconDialog(ICON_LC_TRIANGLE_ALERT, _("File already exists"));
				ImGui::Checkbox(_("Overwrite existing target file"), &_overwriteTargetFile);
			}
			voxelui::genericOptions(&_filterLoadEntries[_lastSource->intVal()]);
		}

		if (ImGui::CollapsingHeader(_("Source options"), ImGuiTreeNodeFlags_DefaultOpen)) {
			const io::FilesystemEntry entry{core::string::extractFilenameWithExtension(_sourceFile), _sourceFile};
			voxelui::loadOptions(&_filterLoadEntries[_lastSource->intVal()], entry, _paletteCache);
		}

		if (ImGui::CollapsingHeader(_("Target options"), ImGuiTreeNodeFlags_DefaultOpen)) {
			const io::FilesystemEntry entry{core::string::extractFilenameWithExtension(_targetFile), _targetFile};
			voxelui::saveOptions(&_filterSaveEntries[_lastTarget->intVal()], entry);
		}

		// TODO: VOXCONVERT: add script execution support

		if (ImGui::IconButton(ICON_LC_CHECK, _("Convert"))) {
			if (_sourceFile.empty()) {
				_output = "No source file selected";
				Log::warn("No source file selected");
			} else if (_targetFile.empty()) {
				_output = "No target file selected";
				Log::warn("No target file selected");
			} else {
				io::BufferedReadWriteStream stream;
				core::DynamicArray<core::String> arguments{"--input", _sourceFile, "--output", _targetFile};
				// only dirty and non-default variables are set for the voxconvert call
				core::Var::visit([&](const core::VarPtr &var) {
					if (var->isDirty()) {
						arguments.push_back("-set");
						arguments.push_back(var->name());
						arguments.push_back(var->strVal());
					}
				});
				if (_overwriteTargetFile) {
					arguments.push_back("-f");
				}
				const core::String args =  core::string::join(arguments.begin(), arguments.end(), " ");
				Log::info("%s %s", _voxconvertBinary.c_str(), args.c_str());
				const int exitCode = core::Process::exec(_voxconvertBinary, arguments, nullptr, &stream);
				stream.seek(0);
				stream.readString(stream.size(), _output);
				_output = core::string::removeAnsiColors(_output.c_str());
				if (exitCode != 0) {
					Log::error("Failed to convert: %s", _output.c_str());
				} else {
					Log::info("Converted successfully");
					_targetFileExists = _filesystem->exists(_targetFile);
					Log::info("%s", _output.c_str());
				}
			}
		}
		ImGui::TooltipText(_("Found vengi-voxconvert at %s"), _voxconvertBinary.c_str());
		if (_targetFileExists) {
			ImGui::SameLine();
			if (ImGui::Button(_("Open target file"))) {
				command::executeCommands("url \"file://" + _targetFile + "\"");
			}
		}
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::InputTextMultiline("##output", &_output, ImVec2(0, 0), ImGuiInputTextFlags_ReadOnly);
	}
	ImGui::End();

	ui::popupAbout();
}

int main(int argc, char *argv[]) {
	const io::FilesystemPtr &filesystem = core::make_shared<io::Filesystem>();
	const core::TimeProviderPtr &timeProvider = core::make_shared<core::TimeProvider>();
	VoxConvertUI app(filesystem, timeProvider);
	return app.startMainLoop(argc, argv);
}
