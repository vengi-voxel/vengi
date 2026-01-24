/**
 * @file
 */

#include "IMGUIApp.h"
#include "IconsLucide.h"
#include "Panel.h"
#include "ScopedID.h"
#include "ScopedStyle.h"
#include "Style.h"
#include "app/App.h"
#include "app/i18n/Language.h"
#include "color/Quantize.h"
#include "core/ConfigVar.h"
#include "core/collection/DynamicArray.h"
#include "dearimgui/imgui.h"
#include "dearimgui/imgui_internal.h"
#include "io/File.h"
#ifdef IMGUI_ENABLE_FREETYPE
#include "dearimgui/misc/freetype/imgui_freetype.h"
#endif

#include "command/Command.h"
#include "core/BindingContext.h"
#include "color/Color.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/TimeProvider.h"
#include "core/Var.h"
#include "dearimgui/backends/imgui_impl_opengl3.h"
#include "dearimgui/implot.h"
#include "io/Filesystem.h"
#include "io/FormatDescription.h"
#include "util/KeybindingHandler.h"
#include "video/Renderer.h"
#include "video/TextureConfig.h"
#include "video/Types.h"

#ifdef IMGUI_ENABLE_TEST_ENGINE
#include "dearimgui/imgui_test_engine/imgui_te_ui.h"
#include "dearimgui/imgui_test_engine/imgui_te_engine.h"
#include "dearimgui/imgui_test_engine/imgui_te_context.h"
#include "dearimgui/imgui_test_engine/imgui_te_exporters.h"
#endif

#include "ArimoRegular.h"
#include "FileDialog.h"
#include "IMGUIEx.h"
#include "IMGUIStyle.h"

#include "FontLucide.h"

#include <glm/mat4x4.hpp>
#include <SDL_events.h>
#include <SDL_version.h>
#if SDL_VERSION_ATLEAST(3, 2, 0)
#define SDL_MOUSEMOTION SDL_EVENT_MOUSE_MOTION
#define SDL_MOUSEWHEEL SDL_EVENT_MOUSE_WHEEL
#define SDL_MOUSEBUTTONUP SDL_EVENT_MOUSE_BUTTON_UP
#define SDL_MOUSEBUTTONDOWN SDL_EVENT_MOUSE_BUTTON_DOWN
#define SDL_TEXTINPUT SDL_EVENT_TEXT_INPUT
#define SDL_KEYDOWN SDL_EVENT_KEY_DOWN
#define SDL_KEYUP SDL_EVENT_KEY_UP
#include "dearimgui/backends/imgui_impl_sdl3.h"
#else
#include <SDL_syswm.h>
#include "dearimgui/backends/imgui_impl_sdl2.h"
#endif

namespace ui {

IMGUIApp::IMGUIApp(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider,
				   size_t threadPoolSize)
	: Super(filesystem, timeProvider, threadPoolSize), _panels(), _fileDialog(this) {
}

IMGUIApp::~IMGUIApp() {
}

static inline void processEvent(SDL_Event &ev) {
#if SDL_VERSION_ATLEAST(3, 2, 0)
	ImGui_ImplSDL3_ProcessEvent(&ev);
#else
	ImGui_ImplSDL2_ProcessEvent(&ev);
#endif
}

void IMGUIApp::onMouseMotion(void *windowHandle, int32_t x, int32_t y, int32_t relX, int32_t relY, int32_t mouseId) {
	Super::onMouseMotion(windowHandle, x, y, relX, relY, mouseId);

	SDL_Event ev{};
	ev.type = SDL_MOUSEMOTION;
	ev.motion.x = x;
	ev.motion.y = y;
	ev.motion.which = mouseId;
	ev.motion.windowID = SDL_GetWindowID((SDL_Window *)windowHandle);
	processEvent(ev);
}

bool IMGUIApp::onMouseWheel(void *windowHandle, float x, float y, int32_t mouseId) {
	if (!Super::onMouseWheel(windowHandle, x, y, mouseId)) {
		SDL_Event ev{};
		ev.type = SDL_MOUSEWHEEL;
#if SDL_VERSION_ATLEAST(3, 2, 0)
#elif SDL_VERSION_ATLEAST(2, 0, 18)
		ev.wheel.preciseX = x;
		ev.wheel.preciseY = y;
#endif
		ev.wheel.x = (int)x;
		ev.wheel.y = (int)y;
		ev.wheel.which = mouseId;
		ev.wheel.windowID = SDL_GetWindowID((SDL_Window *)windowHandle);
		processEvent(ev);
	}
	return true;
}

void IMGUIApp::onMouseButtonRelease(void *windowHandle, int32_t x, int32_t y, uint8_t button, int32_t mouseId) {
	Super::onMouseButtonRelease(windowHandle, x, y, button, mouseId);
	SDL_Event ev{};
	ev.type = SDL_MOUSEBUTTONUP;
	ev.button.button = button;
	ev.button.x = x;
	ev.button.y = y;
	ev.button.which = mouseId;
	ev.button.windowID = SDL_GetWindowID((SDL_Window *)windowHandle);
	processEvent(ev);
}

void IMGUIApp::onMouseButtonPress(void *windowHandle, int32_t x, int32_t y, uint8_t button, uint8_t clicks, int32_t mouseId) {
	Super::onMouseButtonPress(windowHandle, x, y, button, clicks, mouseId);
	SDL_Event ev{};
	ev.type = SDL_MOUSEBUTTONDOWN;
	ev.button.button = button;
	ev.button.clicks = clicks;
	ev.button.x = x;
	ev.button.y = y;
	ev.button.which = mouseId;
	ev.button.windowID = SDL_GetWindowID((SDL_Window *)windowHandle);
	processEvent(ev);
}

bool IMGUIApp::onTextInput(void *windowHandle, const core::String &text) {
	if (_showFileDialog) {
		_fileDialog.onTextInput(windowHandle, text);
	}
	SDL_Event ev{};
	ev.type = SDL_TEXTINPUT;
	ev.text.windowID = SDL_GetWindowID((SDL_Window *)windowHandle);
#if SDL_VERSION_ATLEAST(3, 2, 0)
	ev.text.text = text.c_str();
#else
	core::string::strncpyz(text.c_str(), sizeof(ev.text.text), ev.text.text, sizeof(ev.text.text));
#endif
	processEvent(ev);
	return true;
}

bool IMGUIApp::onKeyPress(void *windowHandle, int32_t key, int16_t modifier) {
	if (!Super::onKeyPress(windowHandle, key, modifier) ||
		(core::bindingContext() == core::BindingContext::UI && key == SDLK_ESCAPE)) {
		SDL_Event ev{};
		ev.type = SDL_KEYDOWN;
		ev.key.windowID = SDL_GetWindowID((SDL_Window *)windowHandle);
#if SDL_VERSION_ATLEAST(3, 2, 0)
		ev.key.scancode = SDL_GetScancodeFromKey(key, nullptr);
		ev.key.key = (SDL_Keycode)key;
		ev.key.mod = modifier;
#else
		ev.key.keysym.scancode = SDL_GetScancodeFromKey(key);
		ev.key.keysym.sym = (SDL_Keycode)key;
		ev.key.keysym.mod = modifier;
#endif
		processEvent(ev);
		_keys.insert(key);
	}
	return true;
}

bool IMGUIApp::onKeyRelease(void *windowHandle, int32_t key, int16_t modifier) {
	if (!Super::onKeyRelease(windowHandle, key, modifier) || _keys.has(key)) {
		SDL_Event ev{};
		ev.type = SDL_KEYUP;
		ev.key.windowID = SDL_GetWindowID((SDL_Window *)windowHandle);
#if SDL_VERSION_ATLEAST(3, 2, 0)
		ev.key.scancode = SDL_GetScancodeFromKey(key, nullptr);
		ev.key.key = (SDL_Keycode)key;
		ev.key.mod = modifier;
#else
		ev.key.keysym.scancode = SDL_GetScancodeFromKey(key);
		ev.key.keysym.sym = key;
		ev.key.keysym.mod = modifier;
#endif
		processEvent(ev);
		_keys.remove(key);
	}
	return true;
}

bool IMGUIApp::handleSDLEvent(SDL_Event &event) {
	const bool state = Super::handleSDLEvent(event);
	if (event.type != SDL_MOUSEMOTION && event.type != SDL_MOUSEWHEEL && event.type != SDL_MOUSEBUTTONUP &&
		event.type != SDL_MOUSEBUTTONDOWN && event.type != SDL_TEXTINPUT && event.type != SDL_KEYUP &&
		event.type != SDL_KEYDOWN) {
		processEvent(event);
	}
	return state;
}

app::AppState IMGUIApp::onConstruct() {
	_console.registerOutputCallbacks();
	const app::AppState state = Super::onConstruct();
	_console.construct();
	_fileDialog.construct();
	_lastDirectory = core::Var::getSafe(cfg::UILastDirectory);
	_languageVar = core::Var::getSafe(cfg::CoreLanguage);
	core::String uiStyleDefaultValue = core::string::toString(ImGui::StyleCorporateGrey);
	if (!isDarkMode()) {
		uiStyleDefaultValue = core::string::toString(ImGui::StyleLight);
	}
	_uistyle =
		core::Var::get(cfg::UIStyle, uiStyleDefaultValue.c_str(), _("Change the ui colors - [0-3]"), [](const core::String &val) {
			const int themeIdx = core::string::toInt(val);
			return themeIdx >= ImGui::StyleCorporateGrey && themeIdx < ImGui::MaxStyles;
		});
	core::Var::get(cfg::UINotifyDismissMillis, "3000", _("Timeout for notifications in millis"));
	core::Var::get(cfg::UIMultiMonitor, "true", _("Allow multi monitor setups - requires a restart"),
				   core::Var::boolValidator);
	_renderUI = core::Var::get(cfg::ClientRenderUI, "true", _("Render the ui"), core::Var::boolValidator);
#ifdef IMGUI_ENABLE_TEST_ENGINE
	_showMetrics = core::Var::get(cfg::UIShowMetrics, "true", core::CV_NOPERSIST, _("Show metric and debug window"),
								  core::Var::boolValidator);
#else
	_showMetrics = core::Var::get(cfg::UIShowMetrics, "false", core::CV_NOPERSIST, _("Show metric and debug window"),
								  core::Var::boolValidator);
#endif
	_uiFontSize =
		core::Var::get(cfg::UIFontSize, "14", -1, _("Allow to change the ui font size"), [](const core::String &val) {
			const float size = core::string::toFloat(val);
			return size >= 8.0f;
		});

	_uiKeyMap = core::Var::get(cfg::UIKeyMap, "0", _("Which keybinding to use"));
	core_assert(!_uiKeyMap->isDirty());
	_lastOpenedFiles = core::Var::get(cfg::UIFileDialogLastFiles, "");
	loadLastOpenedFiles(_lastOpenedFiles->strVal());
	_lastOpenedFile = core::Var::get(cfg::UIFileDialogLastFile, "");

	command::Command::registerCommand("ui_showtextures",
									  [&](const command::CmdArgs &args) { _showTexturesDialog = true; });
	command::Command::registerCommand("ui_close", [&](const command::CmdArgs &args) { _closeModalPopup = true; });

	return state;
}

core::String IMGUIApp::windowTitle() const {
	core::String windowTitle = core::string::extractFilenameWithExtension(_lastOpenedFile->strVal());
	if (windowTitle.empty()) {
		windowTitle = fullAppname();
	} else {
		windowTitle.append(" - ");
		windowTitle.append(fullAppname());
	}
	return windowTitle;
}

void IMGUIApp::loadFonts() {
	ImGuiIO &io = ImGui::GetIO();
	io.Fonts->Clear();
	// const float fontScale = ImGui::GetStyle().FontScaleDpi;
	// const float fontSize = _uiFontSize->floatVal();
	// const float fontScaledSize = fontSize * fontScale;

	ImFontConfig fontIconCfg;
	fontIconCfg.MergeMode = true;
	// fontIconCfg.PixelSnapH = true;
	// fontIconCfg.GlyphOffset = {0.0f, 1.0f * fontScale};
	// fontIconCfg.GlyphMinAdvanceX = fontScaledSize;
	// fontIconCfg.GlyphMaxAdvanceX = fontScaledSize;
	// fontIconCfg.SizePixels = fontScaledSize;
#ifdef IMGUI_ENABLE_FREETYPE
	fontIconCfg.FontLoaderFlags = ImGuiFreeTypeLoaderFlags_LightHinting;
#endif

	io.Fonts->AddFontFromMemoryCompressedTTF(ArimoRegular_compressed_data, ArimoRegular_compressed_size);
	io.Fonts->AddFontFromMemoryCompressedTTF(FontLucide_compressed_data, FontLucide_compressed_size, 0.0f,
											&fontIconCfg);

	core::DynamicArray<io::FilesystemEntry> entities;
	io::filesystem()->list("font", entities, "*.ttf");
	Log::debug("Found %i additional font files", (int)entities.size());
	for (const io::FilesystemEntry &entry : entities) {
		const core::String &name = io::filesystem()->filePath(entry.fullPath);
		Log::debug("Load additional font from %s", name.c_str());
		ImFontConfig fontCfg;
		fontCfg.MergeMode = true;
		if (io.Fonts->AddFontFromFileTTF(name.c_str(), 0.0f, &fontCfg) == nullptr) {
			Log::error("Failed to load font from %s", name.c_str());
		}
	}
}

static void *_imguiAlloc(size_t size, void *) {
	return core_malloc(size);
}

static void _imguiFree(void *mem, void *) {
	core_free(mem);
}

static void ImGui_ImplSDL2_NoShowWindow(ImGuiViewport *) {
}

app::AppState IMGUIApp::onInit() {
	const app::AppState state = Super::onInit();
	video::checkError();
	if (state != app::AppState::Running) {
		return state;
	}

	IMGUI_CHECKVERSION();
	ImGui::SetAllocatorFunctions(_imguiAlloc, _imguiFree);
	ImGui::CreateContext();
	ImPlot::CreateContext();

#ifdef IMGUI_ENABLE_TEST_ENGINE
	// Initialize Test Engine
	_imguiTestEngine = ImGuiTestEngine_CreateContext();
	ImGuiTestEngineIO& test_io = ImGuiTestEngine_GetIO(_imguiTestEngine);
	test_io.ConfigLogToTTY = _showWindow ? false : true;
	test_io.ConfigVerboseLevel = ImGuiTestVerboseLevel_Info;
	test_io.ConfigVerboseLevelOnError = ImGuiTestVerboseLevel_Debug;
#endif

	ImGuiIO &io = ImGui::GetIO();
	io.ConfigDragClickToInputText = true;
	io.ConfigDpiScaleFonts = true;
	io.ConfigDpiScaleViewports = true;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	if (!isSingleWindowMode() && core::Var::getSafe(cfg::UIMultiMonitor)->boolVal()) {
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		// io.ConfigViewportsNoAutoMerge = true;
		// io.ConfigViewportsNoTaskBarIcon = true;
	}
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	// test dpi related issues on linux with
	// xrandr | grep connected | grep -v disconnected | awk '{print $1}'
	// xrandr --output <screen-name> --scale 1.6x1.6

	if (_persistUISettings) {
		const core::String iniFile = core::String::format("%s-%i-imgui.ini", _appname.c_str(), _iniVersion);
		_writePathIni = _filesystem->homeWritePath(iniFile);
		io.IniFilename = _writePathIni.c_str();
	} else {
		io.IniFilename = nullptr;
	}
	const core::String logFile = _appname + "-imgui.log";
	_writePathLog = _filesystem->homeWritePath(logFile);
	io.LogFilename = _writePathLog.c_str();
	io.DisplaySize = _windowDimension;

	setColorTheme();
	loadFonts();
	_uiFontSize->markClean();
	_languageVar->markClean();

#if SDL_VERSION_ATLEAST(3, 2, 0)
	_imguiBackendInitialized = ImGui_ImplSDL3_InitForOpenGL(_window, _rendererContext);
#else
	_imguiBackendInitialized = ImGui_ImplSDL2_InitForOpenGL(_window, _rendererContext);
#endif
	ImGui_ImplOpenGL3_Init(nullptr);

	ImGui::SetColorEditOptions(ImGuiColorEditFlags_Float);

	_console.init();

	Log::debug("Set up imgui");

#ifdef IMGUI_ENABLE_TEST_ENGINE
	if (registerUITests()) {
		ImGuiTestEngine_Start(_imguiTestEngine, ImGui::GetCurrentContext());
	}
#endif
	// if we decide to hide the window, we don't want docking to show externalized windows
	if (!_showWindow) {
		ImGuiPlatformIO &platform_io = ImGui::GetPlatformIO();
		platform_io.Platform_ShowWindow = ImGui_ImplSDL2_NoShowWindow;
	}

	return state;
}

void IMGUIApp::setColorTheme() {
	// reset to default
	ImGuiStyle &style = ImGui::GetStyle();
	style = ImGuiStyle();
	style.TreeLinesFlags = ImGuiTreeNodeFlags_DrawLinesFull;
	style.FontSizeBase = _uiFontSize->floatVal();

	switch (_uistyle->intVal()) {
	case ImGui::StyleCorporateGrey:
		ImGui::StyleColorsCorporateGrey();
		break;
	case ImGui::StyleDark:
		ImGui::StyleColorsDark();
		break;
	case ImGui::StyleLight:
		ImGui::StyleColorsLight();
		break;
	case ImGui::StyleClassic:
		ImGui::StyleColorsClassic();
		break;
	default:
		_uistyle->setVal(ImGui::StyleCorporateGrey);
		ImGui::StyleColorsCorporateGrey();
		break;
	}

	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	ImGui::StyleColorsNeoSequencer();
	ImGui::StyleImGuizmo();
#if SDL_VERSION_ATLEAST(3, 2, 0)
	const float mainScale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
#else
	const float mainScale = ImGui_ImplSDL2_GetContentScaleForDisplay(0);
#endif
	style.ScaleAllSizes(mainScale);
	style.FontScaleDpi = mainScale;
}

const glm::vec4 &IMGUIApp::color(style::StyleColor color) {
	const int style = _uistyle->intVal();
	switch (color) {
	case style::ColorAxisX:
		if (style == ImGui::StyleLight) {
			return color::LightRed();
		}
		return color::DarkRed();
	case style::ColorAxisY:
		if (style == ImGui::StyleLight) {
			return color::LightGreen();
		}
		return color::DarkGreen();
	case style::ColorAxisZ:
		if (style == ImGui::StyleLight) {
			return color::LightBlue();
		}
		return color::DarkBlue();
	case style::ColorLockedNode:
		return color::Red();
	case style::ColorInactiveNode:
		return color::Gray();
	case style::ColorSliceRegion:
	case style::ColorGridBorder:
		if (style == ImGui::StyleLight) {
			return color::DarkGray();
		}
		return color::White();
	case style::ColorReferenceNode:
		return color::LightGreen();
	case style::ColorHighlightArea: {
		static const glm::vec4 c = color::alpha(color::Green(), 0.2f);
		return c;
	}
	case style::ColorUVEditor: {
		if (style == ImGui::StyleLight || style == ImGui::StyleClassic) {
			return color::DarkRed();
		}
		return color::LightRed();
	}
	case style::ColorGroupNode:
		return color::LightYellow();
	case style::ColorActiveNode:
		if (style == ImGui::StyleLight || style == ImGui::StyleClassic) {
			return color::DarkGreen();
		}
		return color::White();
	case style::ColorBone:
		return color::LightGray();
	case style::ColorActiveBrush:
		if (style == ImGui::StyleLight) {
			return color::Green();
		}
		return color::DarkGreen();
	}
	return color::White();
}

void IMGUIApp::beforeUI() {
}

void IMGUIApp::renderBindingsDialog() {
	if (ImGui::Begin(_("Bindings"), &_showBindingsDialog, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_Modal)) {
		const util::BindMap &bindings = _keybindingHandler.bindings();
		static const uint32_t TableFlags = ImGuiTableFlags_Reorderable | ImGuiTableFlags_Resizable |
										   ImGuiTableFlags_Hideable | ImGuiTableFlags_BordersInner |
										   ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY;
		const ImVec2 outerSize(0.0f, ImGui::Height(25.0f));
		if (ImGui::BeginTable("##bindingslist", 4, TableFlags, outerSize)) {
			ImGui::TableSetupColumn(_("Keys"), ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn(_("Command"), ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn(_("Context"), ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn(_("Description"), ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableHeadersRow();

			int n = 0;
			for (util::BindMap::const_iterator i = bindings.begin(); i != bindings.end(); ++i) {
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
				ImGui::TableNextColumn();
				// TODO: change binding
				const core::String &deleteButton = core::String::format(ICON_LC_TRASH "##del-key-%i", n++);
				if (ImGui::Button(deleteButton.c_str())) {
					command::executeCommands(core::String::format("unbind \"%s\"", keyBinding.c_str()),
											 &_lastExecutedCommand);
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
			}
			ImGui::EndTable();
		}
		if (!_uiKeyMaps.empty()) {
			keyMapOption();
		} else {
			if (ImGui::Button(_("Reset to default"))) {
				_resetKeybindings = true;
			}
		}
		ImGui::SameLine();
		ImGui::InputText(_("Filter"), &_bindingsFilter);

		if (ImGui::IconButton(ICON_LC_TEXT_SELECT, _("Edit bindings"))) {
			openKeybindings();
		}
	}
	ImGui::End();
}

void IMGUIApp::renderFPSDialog() {
	if (ImGui::Begin(_("FPS"), &_showFPSDialog)) {
		static bool paused = false;
		ImGui::Checkbox(_("Pause"), &paused);

		auto getter = [](int idx, void *data) {
			IMGUIApp *app = (IMGUIApp *)data;
			return ImPlotPoint(idx, app->_fpsData[idx]);
		};

		if (ImPlot::BeginPlot("##fpsplot", ImVec2(-1, 300), ImPlotFlags_NoMenus | ImPlotFlags_Crosshairs)) {
			ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickLabels);
			ImPlot::SetupAxis(ImAxis_Y1, _("FPS"));
			if (!paused) {
				// Auto-fit to show latest data
				const int dataSize = (int)_fpsData.size();
				const int startIdx = dataSize > 100 ? dataSize - 100 : 0;
				ImPlot::SetupAxisLimits(ImAxis_X1, startIdx, (double)dataSize, ImGuiCond_Always);
			}
			ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 200, ImGuiCond_Once);
			ImPlot::SetupAxisFormat(ImAxis_Y1, "%.0f");
			ImPlot::PlotLineG(_("FPS"), getter, this, (int)_fpsData.size());
			ImPlot::EndPlot();
		}
	}
	ImGui::End();
}

void IMGUIApp::renderCvarDialog() {
	if (ImGui::Begin(_("Configuration variables"), &_showCvarDialog, ImGuiWindowFlags_AlwaysAutoResize)) {
		static const uint32_t TableFlags = ImGuiTableFlags_Reorderable | ImGuiTableFlags_Resizable |
										   ImGuiTableFlags_Hideable | ImGuiTableFlags_BordersInner |
										   ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY;
		const ImVec2 outerSize(ImGui::Height(80.0f), ImGui::Height(25.0f));
		if (ImGui::BeginTable("##cvars", 4, TableFlags, outerSize)) {
			ImGui::TableSetupColumn(_("Name"), ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn(_("Value"), ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("##reset", ImGuiTableFlags_SizingFixedFit);
			ImGui::TableSetupColumn(_("Description"), ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableHeadersRow();

			core::DynamicArray<core::Var*> vars;
			vars.reserve(core::Var::size());
			core::Var::visit([&](const core::VarPtr &var) {
				vars.push_back(var.get());
			});

			// apply filtering
			core::DynamicArray<core::Var*> filteredVars;
			if (_cvarFilter.size() >= 2u) {
				for (core::Var *var : vars) {
					const bool matchName = core::string::icontains(var->name(), _cvarFilter);
					const bool matchValue = core::string::icontains(var->strVal(), _cvarFilter);
					const bool matchHelp = var->help() ? core::string::icontains(var->help(), _cvarFilter) : false;
					if (matchName || matchValue || matchHelp) {
						filteredVars.push_back(var);
					}
				}
			} else {
				filteredVars = vars;
			}

			// use clipper for efficient rendering of only visible items
			ImGuiListClipper clipper;
			clipper.Begin((int)filteredVars.size());
			while (clipper.Step()) {
				for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
					core::Var *var = filteredVars[i];
					ImGui::TableNextColumn();
					ImGui::TextUnformatted(var->name().c_str());
					ImGui::TableNextColumn();
					const bool readOnly = var->getFlags() & core::CV_READONLY;
					ImGui::BeginDisabled(readOnly);
					const core::String type = "##" + var->name();
					if (var->typeIsBool()) {
						bool value = var->boolVal();
						if (ImGui::Checkbox(type.c_str(), &value)) {
							var->setVal(value);
						}
					} else {
						int flags = 0;
						const bool secret = var->getFlags() & core::CV_SECRET;
						if (secret) {
							flags |= ImGuiInputTextFlags_Password;
						}
						core::String value = var->strVal();
						if (ImGui::InputText(type.c_str(), &value, flags)) {
							var->setVal(value);
						}
					}
					ImGui::EndDisabled();
					ImGui::TableNextColumn();
					if (!readOnly) {
						ScopedID id(var->name());
						if (ImGui::Button(_("Reset"))) {
							var->reset();
						}
						ImGui::TooltipTextUnformatted(_("Reset to default value"));
					}
					ImGui::TableNextColumn();
					ImGui::TextUnformatted(var->help() ? var->help() : "");
				}
			}
			ImGui::EndTable();
		}
		ImGui::InputText(_("Filter"), &_cvarFilter);
		ImGui::SameLine();
		if (ImGui::IconButton(ICON_LC_X, _("Close"))) {
			_showCvarDialog = false;
		}
	}
	ImGui::End();
}

void IMGUIApp::renderCommandDialog() {
	if (ImGui::Begin(_("Commands"), &_showCommandDialog, ImGuiWindowFlags_AlwaysAutoResize)) {
		static const uint32_t TableFlags = ImGuiTableFlags_Reorderable | ImGuiTableFlags_Resizable |
										   ImGuiTableFlags_Hideable | ImGuiTableFlags_BordersInner |
										   ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY;
		const ImVec2 outerSize(0.0f, ImGui::Height(25.0f));
		if (ImGui::BeginTable("##commands", 2, TableFlags, outerSize)) {
			ImGui::TableSetupColumn(_("Name"), ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn(_("Description"), ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableHeadersRow();

			command::Command::visitSorted([](const command::Command &cmd) {
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(cmd.name().c_str());
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(cmd.help().c_str());
			});
			ImGui::EndTable();
		}
		if (ImGui::IconButton(ICON_LC_X, _("Close"))) {
			_showCommandDialog = false;
		}
	}
	ImGui::End();
}

void IMGUIApp::renderTexturesDialog() {
	if (ImGui::Begin(_("Textures"), &_showTexturesDialog)) {
		const core::DynamicSet<video::Id> &textures = video::textures();
		const ImVec2 size(512, 512);
		int textureCnt = 0;
		for (const auto &e : textures) {
			ImGui::Image(e->first, size, ImVec2(), ImVec2(1, 1), ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 1));
			// TODO GL_INVALID_OPERATION error generated. Target doesn't match the texture's target.
			++textureCnt;
			if (textureCnt % 2) {
				ImGui::SameLine();
			}
		}
	}
	ImGui::End();
}

static void localizeRegisterEntry(ImGuiLocKey key, const char* text) {
	const ImGuiLocEntry entry{key, text};
	ImGui::LocalizeRegisterEntries(&entry, 1);
}

void IMGUIApp::loadLastOpenedFiles(const core::String &string) {
	core::DynamicArray<core::String> tokens;
	core::string::splitString(string, tokens, ";");
	_lastOpenedFilesRingBuffer.clear();
	for (const core::String &s : tokens) {
		if (s.empty()) {
			continue;
		}
		_lastOpenedFilesRingBuffer.push_back(s);
	}
}

void IMGUIApp::addLastOpenedFile(const core::String &file) {
	if (file.empty()) {
		return;
	}
	for (const core::String &s : _lastOpenedFilesRingBuffer) {
		if (s == file) {
			return;
		}
	}
	_lastOpenedFilesRingBuffer.push_back(file);
	core::String str;
	for (const core::String &s : _lastOpenedFilesRingBuffer) {
		if (!str.empty()) {
			str.append(";");
		}
		str.append(s);
	}
	_lastOpenedFiles->setVal(str);
}

void IMGUIApp::lastOpenedMenu(const char *loadCommand) {
	if (ImGui::BeginIconMenu(ICON_LC_FILE_STACK, _("Recently opened"))) {
		int recentlyOpened = 0;
		for (const core::String &f : lastOpenedFiles()) {
			if (f.empty()) {
				break;
			}
			const core::String &item = core::String::format("%s##%i", f.c_str(), recentlyOpened);
			if (ImGui::MenuItem(item.c_str())) {
				// TODO: this doesn't show the file dialog options
				command::executeCommands(core::String::format("%s \"%s\"", loadCommand, f.c_str()), &_lastExecutedCommand);
			}
			++recentlyOpened;
		}
		ImGui::EndMenu();
	}
}

app::AppState IMGUIApp::onRunning() {
	core_trace_scoped(IMGUIAppOnRunning);
	app::AppState state = Super::onRunning();

	if (state != app::AppState::Running) {
		return state;
	}
	video::clear(video::ClearFlag::Color);
	_fpsData.push_back(_fps);

	_console.update(_deltaFrameSeconds);

	if (_lastOpenedFile->isDirty()) {
		addLastOpenedFile(_lastOpenedFile->strVal());
		_lastOpenedFile->markClean();
	}

	if (_lastOpenedFiles->isDirty()) {
		loadLastOpenedFiles(_lastOpenedFiles->strVal());
		_lastOpenedFiles->markClean();
	}

	if (_uiKeyMap->isDirty() || _resetKeybindings) {
		_keybindingHandler.clear();
		loadKeymap(_uiKeyMap->intVal());
		_uiKeyMap->markClean();
		_resetKeybindings = false;
	}

	if (_languageVar->isDirty()) {
		// IMPORTANT: ###xxx suffixes must be same in ALL languages to allow for automation.
		localizeRegisterEntry(ImGuiLocKey_TableSizeOne, _("Size column to fit###SizeOne"));
		localizeRegisterEntry(ImGuiLocKey_TableSizeAllFit, _("Size all columns to fit###SizeAll"));
		localizeRegisterEntry(ImGuiLocKey_TableSizeAllDefault, _("Size all columns to default###SizeAll"));
		localizeRegisterEntry(ImGuiLocKey_TableResetOrder, _("Reset order###ResetOrder"));
		localizeRegisterEntry(ImGuiLocKey_WindowingMainMenuBar, _("(Main menu bar)"));
		localizeRegisterEntry(ImGuiLocKey_WindowingPopup, _("(Popup)"));
		localizeRegisterEntry(ImGuiLocKey_WindowingUntitled, _("(Untitled)"));
		localizeRegisterEntry(ImGuiLocKey_OpenLink_s, _("Open '%s'"));
		localizeRegisterEntry(ImGuiLocKey_CopyLink, _("Copy Link###CopyLink"));
		localizeRegisterEntry(ImGuiLocKey_DockingHideTabBar, _("Hide tab bar###HideTabBar"));
		localizeRegisterEntry(ImGuiLocKey_DockingHoldShiftToDock, _("Hold SHIFT to enable Docking window."));
		localizeRegisterEntry(ImGuiLocKey_DockingDragToUndockOrMoveNode,
							  _("Click and drag to move or undock whole node."));
		static_assert(ImGuiLocKey_COUNT == 13, "Please update ImGui translations");
	}

	if (_uistyle->isDirty()) {
		setColorTheme();
		_uistyle->markClean();
	}

	const float dpiScale = ImGui::GetStyle().FontScaleDpi;
	if (_languageVar->isDirty() || _uiFontSize->isDirty() || glm::abs(dpiScale - _dpiScale) > 0.001f) {
		loadFonts();
		_dpiScale = dpiScale;
		_uiFontSize->markClean();
		_languageVar->markClean();
	}

	// if the monitor list is empty we currently don't have any monitor available (e.g. switched off)
	// in the case we don't want imgui to trigger an error
	if (ImGui::GetPlatformIO().Monitors.empty()) {
		return app::AppState::Running;
	}

	{
		core_trace_scoped(IMGUIAppBeforeUI);
		beforeUI();
	}

	ImGui_ImplOpenGL3_NewFrame();
#if SDL_VERSION_ATLEAST(3, 2, 0)
	ImGui_ImplSDL3_NewFrame();
#else
	ImGui_ImplSDL2_NewFrame();
#endif
	ImGui::NewFrame();

	const bool renderUI = _renderUI->boolVal();
	if (renderUI) {
		core_trace_scoped(IMGUIAppOnRenderUI);
		ScopedStyle fontSize;
		fontSize.pushFontSize(_uiFontSize->floatVal());
		onRenderUI();
		if (_showConsole) {
			_console.render(_lastExecutedCommand);
		}

#ifdef IMGUI_ENABLE_TEST_ENGINE
		if (_showWindow) {
			ImGuiTestEngine_ShowTestEngineWindows(_imguiTestEngine, nullptr);
		}
#endif

		if (_closeModalPopup) {
			if (ImGui::GetTopMostPopupModal() != nullptr) {
				GImGui->OpenPopupStack.resize(GImGui->OpenPopupStack.size() - 1);
			}
			_closeModalPopup = false;
		}

		if (_showTexturesDialog) {
			renderTexturesDialog();
		}

		if (_showCommandDialog) {
			renderCommandDialog();
		}

		if (_showCvarDialog) {
			renderCvarDialog();
		}

		if (_showFPSDialog) {
			renderFPSDialog();
		}

		if (_showBindingsDialog) {
			renderBindingsDialog();
		}

		bool showMetrics = _showMetrics->boolVal();
		if (showMetrics) {
			ImGui::ShowMetricsWindow(&showMetrics);
			if (!showMetrics) {
				_showMetrics->setVal("false");
			}
		}
		_console.renderNotifications();

		core::String buf;
		const io::FormatDescription *formatDesc = nullptr;
		if (_fileDialog.showFileDialog(_fileDialogOptions, buf, _fileDialogMode, &formatDesc, _showFileDialog)) {
			if (buf[0] != '\0') {
				_fileDialogCallback(buf, formatDesc);
			}
			_showFileDialog = false;
		}

		if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) {
			core::setBindingContext(core::BindingContext::UI);
		} else {
			core::setBindingContext(core::BindingContext::All);
		}
	} else {
		core::setBindingContext(core::BindingContext::All);
	}

	ImGui::EndFrame();
	ImGui::Render();

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	// Update and Render additional Platform Windows
	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		video::activateContext(_window, _rendererContext);
	}

#ifdef IMGUI_ENABLE_TEST_ENGINE
	ImGuiTestEngine_PostSwap(_imguiTestEngine);
#endif

	video::scissor(0, 0, _frameBufferDimension.x, _frameBufferDimension.y);

#ifdef IMGUI_ENABLE_TEST_ENGINE
	// run the tests in command line mode if the window is hidden
	if (!_showWindow) {
		// delay the execution of the tests a few frames to allow all tests to be registered by the app
		if (_startedFromCommandlineFrameDelay > 0) {
			_startedFromCommandlineFrameDelay--;
		}
		if (_startedFromCommandlineFrameDelay == 0) {
			ImGuiTestEngine_QueueTests(_imguiTestEngine, ImGuiTestGroup_Tests, "tests", ImGuiTestRunFlags_RunFromCommandLine);
			_startedFromCommandlineFrameDelay--;
		}

		if (_startedFromCommandlineFrameDelay == -1) {
			if (ImGuiTestEngine_IsTestQueueEmpty(_imguiTestEngine)) {
				int testedCnt = 0;
				int successCnt = 0;
				core::String testresults = _appname + "-ui-tests.xml";
				ImGuiTestEngine_ExportEx(_imguiTestEngine, ImGuiTestEngineExportFormat_JUnitXml, testresults.c_str());
				ImGuiTestEngineResultSummary summary;
				ImGuiTestEngine_GetResultSummary(_imguiTestEngine, &summary);
				testedCnt = summary.CountTested;
				successCnt = summary.CountSuccess;
				if (successCnt != testedCnt) {
					_exitCode = 1;
				}
				requestQuit();
			}
		}
	}
#endif

	return app::AppState::Running;
}

void IMGUIApp::loadKeymap(int keymap) {
	_keybindingHandler.registerBinding("escape", "ui_close", "ui");
}

void IMGUIApp::languageOption() {
	core::String currentLanguage = _languageVar->strVal();
	if (ImGui::BeginCombo(_("Language"), _languageVar->strVal().c_str())) {
		const app::Languages &languages = _dictManager.getLanguages();
		app::Language currentLang = app::Language::fromEnv(currentLanguage);
		for (const auto &lang : languages) {
			bool isSelected = lang == currentLang;
			if (ImGui::Selectable(lang.str().c_str(), isSelected)) {
				_languageVar->setVal(lang.str());
				setLanguage(lang.str());
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	ImGui::TooltipTextUnformatted(_("To change the language everywhere restart the application"));
}

bool IMGUIApp::keyMapOption() {
	if (_uiKeyMaps.empty()) {
		return false;
	}
	return ImGui::ComboVar(_("Keymap"), _uiKeyMap->name().c_str(), _uiKeyMaps);
}

void IMGUIApp::colorReductionOptions() {
	const core::VarPtr &colorReduction = core::Var::getSafe(cfg::CoreColorReduction);
	if (ImGui::BeginCombo(_("Color reduction"), colorReduction->strVal().c_str(), ImGuiComboFlags_None)) {
		color::ColorReductionType type = color::toColorReductionType(colorReduction->strVal().c_str());
		for (int i = 0; i < (int)color::ColorReductionType::Max; ++i) {
			const bool selected = i == (int)type;
			const char *str = color::toColorReductionTypeString((color::ColorReductionType)i);
			if (ImGui::Selectable(str, selected)) {
				colorReduction->setVal(str);
			}
			if (selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	ImGui::TooltipTextUnformatted(
		_("The color reduction algorithm that is used when importing RGBA colors from images or rgba formats"));
}

app::AppState IMGUIApp::onCleanup() {
#ifdef IMGUI_ENABLE_TEST_ENGINE
	ImGuiTestEngine_Stop(_imguiTestEngine);
	_fileDialog.unregisterUITests(_imguiTestEngine);
#endif
	if (_imguiBackendInitialized) {
		ImGui_ImplOpenGL3_Shutdown();
#if SDL_VERSION_ATLEAST(3, 2, 0)
		ImGui_ImplSDL3_Shutdown();
#else
		ImGui_ImplSDL2_Shutdown();
#endif
		_imguiBackendInitialized = false;
	}
	if (ImGui::GetCurrentContext() != nullptr) {
		ImPlot::DestroyContext();
		ImGui::DestroyPlatformWindows();
		ImGui::DestroyContext();
	}
#ifdef IMGUI_ENABLE_TEST_ENGINE
	ImGuiTestEngine_DestroyContext(_imguiTestEngine);
	_imguiTestEngine = nullptr;
#endif
	_console.shutdown();
	return Super::onCleanup();
}

void IMGUIApp::fileDialog(const video::FileDialogSelectionCallback &callback, const video::FileDialogOptions &options,
						  video::OpenFileMode mode, const io::FormatDescription *formats,
						  const core::String &filename) {
	_showFileDialog = true;
	_fileDialogCallback = callback;
	_fileDialogOptions = options;
	_fileDialogMode = mode;
	_fileDialog.openDir(mode, formats, filename);
}

void IMGUIApp::addPanel(Panel *panel) {
	core_assert(panel != nullptr);
	_panels.push_back(panel);
}

void IMGUIApp::removePanel(Panel *panel) {
	auto iter = core::find(_panels.begin(), _panels.end(), panel);
	if (iter != _panels.end()) {
		_panels.erase(iter);
	}
}

Panel* IMGUIApp::getPanel(const core::String &title) {
	for (Panel *panel : _panels) {
		if (panel->_title == title) {
			return panel;
		}
	}
	Log::error("Panel '%s' not found - available are:", title.c_str());
	for (Panel *panel : _panels) {
		Log::error("- %s", panel->_title.c_str());
	}
	return nullptr;
}

} // namespace ui
