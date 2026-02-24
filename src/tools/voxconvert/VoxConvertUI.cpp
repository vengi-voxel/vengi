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
		_inputFile = _argv[_argc - 1];
	} else {
		_inputFile = loadingDocument();
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
					ImGui::CheckboxVar(cfg::UIMultiMonitor);
					ImGui::InputVarInt(cfg::UIFontSize, 1, 5);
					const core::VarPtr &uiStyleVar = core::getVar(cfg::UIStyle);
					int currentStyle = uiStyleVar->intVal();
					if (ImGui::BeginCombo(_("Color theme"), ImGui::GetStyleName(currentStyle))) {
						for (int i = 0; i < ImGui::MaxStyles; ++i) {
							const bool isSelected = (currentStyle == i);
							if (ImGui::Selectable(ImGui::GetStyleName(i), isSelected)) {
								uiStyleVar->setVal(i);
							}
							if (isSelected) {
								ImGui::SetItemDefaultFocus();
							}
						}
						ImGui::EndCombo();
					}
					ImGui::InputVarFloat(cfg::UINotifyDismissMillis);
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

		ImGui::InputFile(_("Input"), true, &_inputFile, voxelformat::voxelLoad());
		if (ImGui::IsItemHovered()) {
			if (!_droppedFile.empty()) {
				_inputFile = _droppedFile;
				_droppedFile.clear();
			}
		}
		ImGui::InputFile(_("Output"), false, &_outputFile, voxelformat::voxelSave());
		if (ImGui::IsItemHovered()) {
			if (!_droppedFile.empty()) {
				_outputFile = _droppedFile;
				_droppedFile.clear();
			}
		}

		const io::FormatDescription *inputDesc = nullptr;
		if (!_inputFile.empty()) {
			inputDesc = io::getDescription(_inputFile, 0, voxelformat::voxelLoad());
		}

		const io::FormatDescription *outputDesc = nullptr;
		if (!_outputFile.empty()) {
			outputDesc = io::getDescription(_outputFile, 0, voxelformat::voxelSave());
		}
		if (_oldOutputFile != _outputFile) {
			_oldOutputFile = _outputFile;
			_outputFileExists = io::Filesystem::sysExists(_outputFile);
		}

		if (_voxconvertBinary.empty()) {
			ImGui::IconDialog(ICON_LC_CIRCLE_ALERT, _("vengi-voxconvert not found in current dir or PATH"));
		}

		if (ImGui::CollapsingHeader(_("Options"), ImGuiTreeNodeFlags_DefaultOpen)) {
			if (_outputFileExists) {
				ImGui::IconDialog(ICON_LC_TRIANGLE_ALERT, _("File already exists"));
				ImGui::Checkbox(_("Overwrite existing output file"), &_overwriteOutputFile);
			}
			if (inputDesc) {
				voxelui::genericOptions(inputDesc);
			}
		}

		if (ImGui::CollapsingHeader(_("Input options"), ImGuiTreeNodeFlags_DefaultOpen)) {
			if (inputDesc) {
				const io::FilesystemEntry entry{core::string::extractFilenameWithExtension(_inputFile), _inputFile};
				voxelui::loadOptions(inputDesc, entry, _paletteCache);
			}
		}

		if (ImGui::CollapsingHeader(_("Output options"), ImGuiTreeNodeFlags_DefaultOpen)) {
			if (outputDesc) {
				const io::FilesystemEntry entry{core::string::extractFilenameWithExtension(_outputFile), _outputFile};
				voxelui::saveOptions(outputDesc, entry);
			}
		}

		if (ImGui::CollapsingHeader(_("Script options"), ImGuiTreeNodeFlags_DefaultOpen)) {
			_luaApiWidget.updateScriptExecutionPanel(_luaApi, voxel::getPalette(), _luaApiCtx,
													 voxelui::LUAAPI_WIDGET_FLAG_NOTIFY);
		}

		if (ImGui::IconButton(ICON_LC_CHECK, _("Convert"))) {
			if (_inputFile.empty()) {
				_output = "No input file selected";
				Log::warn("No input file selected");
			} else if (_outputFile.empty()) {
				_output = "No output file selected";
				Log::warn("No output file selected");
			} else {
				core::DynamicArray<core::String> arguments{"--input", _inputFile, "--output", _outputFile};
				// only dirty and non-default variables are set for the voxconvert call
				core::Var::visit([&](const core::VarPtr &var) {
					if (var->isDirty()) {
						arguments.push_back("-set");
						arguments.push_back(var->name());
						arguments.push_back(var->strVal());
					}
				});
				if (_overwriteOutputFile) {
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
					_outputFileExists = _filesystem->exists(_outputFile);
					Log::info("%s", _output.c_str());
				}
			}
		}
		ImGui::TooltipText(_("Found vengi-voxconvert at %s"), _voxconvertBinary.c_str());
		if (_outputFileExists) {
			ImGui::SameLine();
			if (ImGui::Button(_("Open output file"))) {
				command::executeCommands("url \"file://" + _outputFile + "\"");
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
