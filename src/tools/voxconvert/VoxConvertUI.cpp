/**
 * @file
 */

#include "VoxConvertUI.h"
#include "IMGUIStyle.h"
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
	: Super(filesystem, timeProvider) {
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
			_filterEntries.push_back(*desc);
		}
		_filterEntries.sort(core::Greater<io::FormatDescription>());
	}
	if (_lastTarget->intVal() < 0 || _lastTarget->intVal() >= (int)_filterEntries.size()) {
		_lastTarget->setVal(0);
	}
	if (_lastSource->intVal() < 0 || _lastSource->intVal() >= (int)_filterEntries.size()) {
		_lastSource->setVal(0);
	}
	return state;
}

void VoxConvertUI::onRenderUI() {
	ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);

	if (_filterFormatTextWidth < 0.0f) {
		for (const io::FormatDescription &desc : _filterEntries) {
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
					ImGui::EndMenu();
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginIconMenu(ICON_LC_CIRCLE_HELP, _("Help"))) {
				if (ImGui::IconMenuItem(ICON_LC_INFO, _("About"))) {
					ImGui::OpenPopup(POPUP_TITLE_ABOUT); // FIXME
				}
				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		io::FormatDescription entries[] = {_filterEntries[_lastSource->intVal()], {"", {}, {}, 0u}};
		if (ImGui::InputFile(_("Source"), &_source, entries)) {
			_dirtyTargetFile = true;
		}

		ImGui::PushItemWidth(_filterFormatTextWidth);
		const core::String sourceStr = io::convertToFilePattern(_filterEntries[_lastSource->intVal()]);
		if (ImGui::BeginCombo(_("Source format"), sourceStr.c_str())) {
			for (int i = 0; i < (int)_filterEntries.size(); ++i) {
				const bool selected = i == _lastSource->intVal();
				const io::FormatDescription &format = _filterEntries[i];
				const core::String &text = io::convertToFilePattern(format);
				if (ImGui::Selectable(text.c_str(), selected)) {
					_lastSource->setVal(i);
					_source = "";
					_dirtyTargetFile = true;
				}
				if (selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		const core::String targetStr = io::convertToFilePattern(_filterEntries[_lastTarget->intVal()]);
		if (ImGui::BeginCombo(_("Target format"), targetStr.c_str())) {
			for (int i = 0; i < (int)_filterEntries.size(); ++i) {
				const bool selected = i == _lastTarget->intVal();
				const io::FormatDescription &format = _filterEntries[i];
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

		if (_dirtyTargetFile) {
			if (_source.empty()) {
				_targetFile = "";
				_targetFileExists = false;
			} else {
				_targetFile =
					core::string::replaceExtension(_source, _filterEntries[_lastTarget->intVal()].mainExtension());
				_targetFileExists = _filesystem->exists(_targetFile);
			}
			_dirtyTargetFile = false;
		}

		if (ImGui::CollapsingHeader(_("Options"), ImGuiTreeNodeFlags_DefaultOpen)) {
			genericOptions(&_filterEntries[_lastSource->intVal()], _targetFileExists, _overwriteTargetFile);
		}

		if (ImGui::CollapsingHeader(_("Source options"), ImGuiTreeNodeFlags_DefaultOpen)) {
			sourceOptions(&_filterEntries[_lastSource->intVal()]);
		}

		if (ImGui::CollapsingHeader(_("Target options"), ImGuiTreeNodeFlags_DefaultOpen)) {
			targetOptions(&_filterEntries[_lastTarget->intVal()]);
		}

		if (ImGui::Button(_("Convert"))) {
			if (_source.empty()) {
				_output = "No source file selected";
			} else {
				io::BufferedReadWriteStream stream;
				core::DynamicArray<core::String> arguments{"--input", _source, "--output", _targetFile};
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
				const int exitCode = core::Process::exec(_voxconvertBinary, arguments, nullptr, &stream);
				stream.seek(0);
				stream.readString(stream.size(), _output);
				_output = core::string::removeAnsiColors(_output.c_str());
				if (exitCode != 0) {
					Log::error("Failed to convert: %s", _output.c_str());
				} else {
					Log::info("Converted successfully");
					_targetFileExists = _filesystem->exists(_targetFile);
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
