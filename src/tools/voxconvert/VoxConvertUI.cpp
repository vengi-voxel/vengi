/**
 * @file
 */

#include "VoxConvertUI.h"
#include "IMGUIStyle.h"
#include "PopupAbout.h"
#include "app/App.h"
#include "command/CommandHandler.h"
#include "core/Process.h"
#include "core/StringUtil.h"
#include "io/Archive.h"
#include "io/BufferedReadWriteStream.h"
#include "io/FilesystemEntry.h"
#include "io/FormatDescription.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "voxel/MaterialColor.h"
#include "voxelformat/FormatConfig.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelgenerator/LUAApi.h"
#include "voxelui/FileDialogOptions.h"
#include "voxelui/LUAApiWidget.h"

VoxConvertUI::VoxConvertUI(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider)
	: Super(filesystem, timeProvider), _paletteCache(filesystem), _luaApi(filesystem) {
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
	if (!filesystem()->registerPath("scripts/")) {
		Log::warn("Failed to register lua generator script path");
	}
	_luaApi.construct();
	return state;
}

app::AppState VoxConvertUI::onInit() {
	app::AppState state = Super::onInit();
	if (!_luaApi.init()) {
		Log::error("Failed to initialize LUA API");
	}
	_voxconvertBinary = _filesystem->sysFindBinary("vengi-voxconvert");

	_paletteCache.detectPalettes();

	if (_argc >= 2) {
		_sourceFile = _argv[_argc - 1];
	} else {
		_sourceFile = loadingDocument();
	}

	return state;
}

app::AppState VoxConvertUI::onCleanup() {
	app::AppState state = Super::onCleanup();
	_luaApi.shutdown();
	return state;
}

void VoxConvertUI::onDropFile(void *windowHandle, const core::String &file) {
	_droppedFile = file;
}

void VoxConvertUI::onRenderUI() {
	ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);
	bool popupAbout = false;

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
			if (ImGui::BeginIconMenu(ICON_LC_CIRCLE_QUESTION_MARK, _("Help"))) {
				if (ImGui::IconMenuItem(ICON_LC_INFO, _("About"))) {
					popupAbout = true;
				}
				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		ImGui::InputFile(_("Source"), true, &_sourceFile, voxelformat::voxelLoad());
		if (ImGui::IsItemHovered()) {
			if (!_droppedFile.empty()) {
				_sourceFile = _droppedFile;
				_droppedFile.clear();
			}
		}
		ImGui::InputFile(_("Target"), false, &_targetFile, voxelformat::voxelSave());
		if (ImGui::IsItemHovered()) {
			if (!_droppedFile.empty()) {
				_targetFile = _droppedFile;
				_droppedFile.clear();
			}
		}

		const io::FormatDescription *sourceDesc = nullptr;
		if (!_sourceFile.empty()) {
			sourceDesc = io::getDescription(_sourceFile, 0, voxelformat::voxelLoad());
		}

		const io::FormatDescription *targetDesc = nullptr;
		if (!_targetFile.empty()) {
			targetDesc = io::getDescription(_targetFile, 0, voxelformat::voxelSave());
		}
		if (_oldTargetFile != _targetFile) {
			_oldTargetFile = _targetFile;
			_targetFileExists = io::Filesystem::sysExists(_targetFile);
		}

		if (_voxconvertBinary.empty()) {
			ImGui::IconDialog(ICON_LC_CIRCLE_ALERT, _("vengi-voxconvert not found in current dir or PATH"));
		}

		if (ImGui::CollapsingHeader(_("Options"), ImGuiTreeNodeFlags_DefaultOpen)) {
			if (_targetFileExists) {
				ImGui::IconDialog(ICON_LC_TRIANGLE_ALERT, _("File already exists"));
				ImGui::Checkbox(_("Overwrite existing target file"), &_overwriteTargetFile);
			}
			if (sourceDesc) {
				voxelui::genericOptions(sourceDesc);
			}
		}

		if (ImGui::CollapsingHeader(_("Source options"), ImGuiTreeNodeFlags_DefaultOpen)) {
			if (sourceDesc) {
				const io::FilesystemEntry entry{core::string::extractFilenameWithExtension(_sourceFile), _sourceFile};
				voxelui::loadOptions(sourceDesc, entry, _paletteCache);
			}
		}

		if (ImGui::CollapsingHeader(_("Target options"), ImGuiTreeNodeFlags_DefaultOpen)) {
			if (targetDesc) {
				const io::FilesystemEntry entry{core::string::extractFilenameWithExtension(_targetFile), _targetFile};
				voxelui::saveOptions(targetDesc, entry);
			}
		}

		if (ImGui::CollapsingHeader(_("Script options"), ImGuiTreeNodeFlags_DefaultOpen)) {
			_luaApiWidget.updateScriptExecutionPanel(_luaApi, voxel::getPalette(), _luaApiCtx,
													 voxelui::LUAAPI_WIDGET_FLAG_NOTIFY);
		}

		if (ImGui::IconButton(ICON_LC_CHECK, _("Convert"))) {
			if (_sourceFile.empty()) {
				_output = "No source file selected";
				Log::warn("No source file selected");
			} else if (_targetFile.empty()) {
				_output = "No target file selected";
				Log::warn("No target file selected");
			} else {
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

				if (!_luaApiCtx._scriptFilename.empty()) {
					arguments.push_back("--script");
					const core::String scriptArgs =
						core::string::join(_luaApiCtx._args.begin(), _luaApiCtx._args.end(), " ");
					arguments.push_back(
						core::String::format("\"%s %s\"", _luaApiCtx._scriptFilename.c_str(), scriptArgs.c_str()));
				}

				const core::String args = core::string::join(arguments.begin(), arguments.end(), " ");
				Log::info("%s %s", _voxconvertBinary.c_str(), args.c_str());
				io::BufferedReadWriteStream stream;
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

	if (popupAbout) {
		ImGui::OpenPopup(POPUP_TITLE_ABOUT);
	}

	ui::popupAbout();
}

int main(int argc, char *argv[]) {
	const io::FilesystemPtr &filesystem = core::make_shared<io::Filesystem>();
	const core::TimeProviderPtr &timeProvider = core::make_shared<core::TimeProvider>();
	VoxConvertUI app(filesystem, timeProvider);
	return app.startMainLoop(argc, argv);
}
