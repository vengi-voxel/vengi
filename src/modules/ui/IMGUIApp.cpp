/**
 * @file
 */

#include "IMGUIApp.h"
#include "IconsLucide.h"
#include "Style.h"
#include "app/i18n/Language.h"
#include "imgui.h"
#include "imgui_internal.h"
#ifdef IMGUI_ENABLE_FREETYPE
#include "misc/freetype/imgui_freetype.h"
#endif

#include "command/Command.h"
#include "core/BindingContext.h"
#include "core/Color.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/TimeProvider.h"
#include "core/Var.h"
#include "dearimgui/backends/imgui_impl_opengl3.h"
#include "dearimgui/backends/imgui_impl_sdl2.h"
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
#endif

#include "ArimoRegular.h"
#include "FileDialog.h"
#include "IMGUIEx.h"
#include "IMGUIStyle.h"

#include "FontLucide.h"

#include <SDL.h>
#include <SDL_events.h>
#include <SDL_syswm.h>
#include <glm/mat4x4.hpp>

namespace ui {

IMGUIApp::IMGUIApp(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider,
				   size_t threadPoolSize)
	: Super(filesystem, timeProvider, threadPoolSize) {
}

IMGUIApp::~IMGUIApp() {
}

void IMGUIApp::onMouseMotion(void *windowHandle, int32_t x, int32_t y, int32_t relX, int32_t relY) {
	Super::onMouseMotion(windowHandle, x, y, relX, relY);

	SDL_Event ev{};
	ev.type = SDL_MOUSEMOTION;
	ev.motion.x = x;
	ev.motion.y = y;
	ev.motion.windowID = SDL_GetWindowID((SDL_Window *)windowHandle);
	ImGui_ImplSDL2_ProcessEvent(&ev);
}

bool IMGUIApp::onMouseWheel(int32_t x, int32_t y) {
	if (!Super::onMouseWheel(x, y)) {
		SDL_Event ev{};
		ev.type = SDL_MOUSEWHEEL;
#if SDL_VERSION_ATLEAST(2, 0, 18)
		ev.wheel.preciseX = (float)x;
		ev.wheel.preciseY = (float)y;
#endif
		ev.wheel.x = x;
		ev.wheel.y = y;
		ImGui_ImplSDL2_ProcessEvent(&ev);
	}
	return true;
}

void IMGUIApp::onMouseButtonRelease(int32_t x, int32_t y, uint8_t button) {
	Super::onMouseButtonRelease(x, y, button);
	SDL_Event ev{};
	ev.type = SDL_MOUSEBUTTONUP;
	ev.button.button = button;
	ev.button.x = x;
	ev.button.y = y;
	ImGui_ImplSDL2_ProcessEvent(&ev);
}

void IMGUIApp::onMouseButtonPress(int32_t x, int32_t y, uint8_t button, uint8_t clicks) {
	Super::onMouseButtonPress(x, y, button, clicks);
	SDL_Event ev{};
	ev.type = SDL_MOUSEBUTTONDOWN;
	ev.button.button = button;
	ev.button.clicks = clicks;
	ev.button.x = x;
	ev.button.y = y;
	ImGui_ImplSDL2_ProcessEvent(&ev);
}

bool IMGUIApp::onTextInput(const core::String &text) {
	SDL_Event ev{};
	ev.type = SDL_TEXTINPUT;
	core::string::strncpyz(text.c_str(), sizeof(ev.text.text), ev.text.text, sizeof(ev.text.text));
	ImGui_ImplSDL2_ProcessEvent(&ev);
	return true;
}

bool IMGUIApp::onKeyPress(int32_t key, int16_t modifier) {
	if (!Super::onKeyPress(key, modifier) ||
		(core::bindingContext() == core::BindingContext::UI && key == SDLK_ESCAPE)) {
		SDL_Event ev{};
		ev.type = SDL_KEYDOWN;
		ev.key.keysym.scancode = (SDL_Scancode)SDL_SCANCODE_UNKNOWN;
		ev.key.keysym.sym = (SDL_Keycode)key;
		ev.key.keysym.mod = modifier;
		ImGui_ImplSDL2_ProcessEvent(&ev);
		_keys.insert(key);
	}
	return true;
}

bool IMGUIApp::onKeyRelease(int32_t key, int16_t modifier) {
	if (!Super::onKeyRelease(key, modifier) || _keys.has(key)) {
		SDL_Event ev{};
		ev.type = SDL_KEYUP;
		ev.key.keysym.scancode = (SDL_Scancode)SDL_SCANCODE_UNKNOWN;
		ev.key.keysym.sym = key;
		ev.key.keysym.mod = modifier;
		ImGui_ImplSDL2_ProcessEvent(&ev);
		_keys.remove(key);
	}
	return true;
}

bool IMGUIApp::handleSDLEvent(SDL_Event &event) {
	const bool state = Super::handleSDLEvent(event);
	if (event.type == SDL_WINDOWEVENT) {
		ImGui_ImplSDL2_ProcessEvent(&event);
	}
	return state;
}

app::AppState IMGUIApp::onConstruct() {
	_console.registerOutputCallbacks();
	const app::AppState state = Super::onConstruct();
	_console.construct();
	_fileDialog.construct();
	_lastDirectory = core::Var::getSafe(cfg::UILastDirectory);

	core::String uiStyleDefaultValue = core::string::toString(ImGui::StyleCorporateGrey);
	if (!isDarkMode()) {
		uiStyleDefaultValue = core::string::toString(ImGui::StyleLight);
	}
	_uistyle =
		core::Var::get(cfg::UIStyle, uiStyleDefaultValue.c_str(), "Change the ui colors - [0-3]", [](const core::String &val) {
			const int themeIdx = core::string::toInt(val);
			return themeIdx >= ImGui::StyleCorporateGrey && themeIdx < ImGui::MaxStyles;
		});
	core::Var::get(cfg::UINotifyDismissMillis, "3000", "Timeout for notifications in millis");
	core::Var::get(cfg::UIMultiMonitor, "true", "Allow multi monitor setups - requires a restart",
				   core::Var::boolValidator);
	_renderUI = core::Var::get(cfg::ClientRenderUI, "true", "Render the ui", core::Var::boolValidator);
#ifdef IMGUI_ENABLE_TEST_ENGINE
	_showMetrics = core::Var::get(cfg::UIShowMetrics, "true", core::CV_NOPERSIST, "Show metric and debug window",
								  core::Var::boolValidator);
#else
	_showMetrics = core::Var::get(cfg::UIShowMetrics, "false", core::CV_NOPERSIST, "Show metric and debug window",
								  core::Var::boolValidator);
#endif
	_uiFontSize =
		core::Var::get(cfg::UIFontSize, "14", -1, "Allow to change the ui font size", [](const core::String &val) {
			const float size = core::string::toFloat(val);
			return size >= 2.0f;
		});

	_uiKeyMap = core::Var::get(cfg::UIKeyMap, "0", "Which keybinding to use");
	core_assert(!_uiKeyMap->isDirty());

	command::Command::registerCommand("ui_showtextures",
									  [&](const command::CmdArgs &args) { _showTexturesDialog = true; });
	command::Command::registerCommand("ui_close", [&](const command::CmdArgs &args) { _closeModalPopup = true; });

	return state;
}

void IMGUIApp::loadFonts() {
	ImGuiIO &io = ImGui::GetIO();
	io.Fonts->Clear();

	const ImWchar *rangesBasic = io.Fonts->GetGlyphRangesDefault();

	const float fontSize = _uiFontSize->floatVal();

	ImFontConfig fontCfg;
	fontCfg.MergeMode = true;
#ifdef IMGUI_ENABLE_FREETYPE
	fontCfg.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_LightHinting;
#endif

	ImFontConfig bigFontIconCfg;
	bigFontIconCfg.MergeMode = true;
	bigFontIconCfg.GlyphMinAdvanceX = fontSize * 2.0f;
	bigFontIconCfg.GlyphMaxAdvanceX = bigFontIconCfg.GlyphMinAdvanceX;
#ifdef IMGUI_ENABLE_FREETYPE
	bigFontIconCfg.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_LightHinting;
#endif

	ImFontConfig fontIconCfg;
	fontIconCfg.MergeMode = true;
	fontIconCfg.GlyphMinAdvanceX = fontSize;
	fontIconCfg.GlyphMaxAdvanceX = bigFontIconCfg.GlyphMinAdvanceX;

	static const ImWchar rangesLCIcons[] = {ICON_MIN_LC, ICON_MAX_LC, 0};

	_defaultFont = io.Fonts->AddFontFromMemoryCompressedTTF(ArimoRegular_compressed_data, ArimoRegular_compressed_size,
															fontSize, nullptr, rangesBasic);
	io.Fonts->AddFontFromMemoryCompressedTTF(FontLucide_compressed_data, FontLucide_compressed_size, fontSize,
											 &fontIconCfg, rangesLCIcons);

	_bigFont = io.Fonts->AddFontFromMemoryCompressedTTF(ArimoRegular_compressed_data, ArimoRegular_compressed_size,
														fontSize * 2.0f, nullptr, rangesBasic);

	_bigIconFont = io.Fonts->AddFontFromMemoryCompressedTTF(FontLucide_compressed_data, FontLucide_compressed_size,
															fontSize * 1.5f, &bigFontIconCfg, rangesLCIcons);

	_smallFont = io.Fonts->AddFontFromMemoryCompressedTTF(ArimoRegular_compressed_data, ArimoRegular_compressed_size,
														  fontSize * 0.8f, nullptr, rangesBasic);
	io.Fonts->AddFontFromMemoryCompressedTTF(FontLucide_compressed_data, FontLucide_compressed_size, fontSize,
											 &fontIconCfg, rangesLCIcons);

	unsigned char *pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	video::TextureConfig cfg;
	cfg.format(video::TextureFormat::RGBA);
	if (_texture == video::InvalidId) {
		_texture = video::genTexture(cfg);
	}

	video::bindTexture(video::TextureUnit::Upload, cfg.type(), _texture);
	video::setupTexture(cfg);
	video::uploadTexture(cfg.type(), cfg.format(), width, height, pixels, 0, cfg.samples());

	io.Fonts->TexID = (ImTextureID)(intptr_t)_texture;
}

static void *_imguiAlloc(size_t size, void *) {
	return core_malloc(size);
}

static void _imguiFree(void *mem, void *) {
	core_free(mem);
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
	test_io.ConfigVerboseLevel = ImGuiTestVerboseLevel_Info;
	test_io.ConfigVerboseLevelOnError = ImGuiTestVerboseLevel_Debug;
#endif

	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	if (!isSingleWindowMode() && core::Var::getSafe(cfg::UIMultiMonitor)->boolVal()) {
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		// io.ConfigViewportsNoAutoMerge = true;
		// io.ConfigViewportsNoTaskBarIcon = true;
	}
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	// io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports;
	// io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;
	// test dpi related issues on linux with
	// xrandr | grep connected | grep -v disconnected | awk '{print $1}'
	// xrandr --output <screen-name> --scale 1.6x1.6

	if (_persistUISettings) {
		const core::String iniFile = core::string::format("%s-%i-imgui.ini", _appname.c_str(), _iniVersion);
		_writePathIni = _filesystem->writePath(iniFile);
		io.IniFilename = _writePathIni.c_str();
	} else {
		io.IniFilename = nullptr;
	}
	const core::String logFile = _appname + "-imgui.log";
	_writePathLog = _filesystem->writePath(logFile);
	io.LogFilename = _writePathLog.c_str();
	io.DisplaySize = _windowDimension;

	loadFonts();
	setColorTheme();
	_imguiBackendInitialized = ImGui_ImplSDL2_InitForOpenGL(_window, _rendererContext);
	ImGui_ImplOpenGL3_Init(nullptr);

	ImGui::SetColorEditOptions(ImGuiColorEditFlags_Float);

	_console.init();

	Log::debug("Set up imgui");

#ifdef IMGUI_ENABLE_TEST_ENGINE
	if (registerUITests()) {
		ImGuiTestEngine_Start(_imguiTestEngine, ImGui::GetCurrentContext());
	}
#endif

	return state;
}

void IMGUIApp::setColorTheme() {
	// reset to default
	ImGuiStyle &style = ImGui::GetStyle();
	style = ImGuiStyle();

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
	ImGui::StyleColorsNeoSequencer();
	ImGui::StyleImGuizmo();
}

const glm::vec4 &IMGUIApp::color(style::StyleColor color) {
	const int style = _uistyle->intVal();
	switch (color) {
	case style::ColorInactiveNode:
		return core::Color::Gray();
	case style::ColorGridBorder:
		if (style == ImGui::StyleLight) {
			return core::Color::DarkGray();
		}
		return core::Color::White();
	case style::ColorReferenceNode:
		return core::Color::LightGreen();
	case style::ColorHighlightArea: {
		static const glm::vec4 c = core::Color::alpha(core::Color::Green(), 0.2f);
		return c;
	}
	case style::ColorActiveNode:
		if (style == ImGui::StyleLight || style == ImGui::StyleClassic) {
			return core::Color::DarkGreen();
		}
		return core::Color::White();
	case style::ColorBone:
		return core::Color::LightGray();
	}
	return core::Color::White();
}

void IMGUIApp::beforeUI() {
}

void IMGUIApp::renderBindingsDialog() {
	if (ImGui::Begin(_("Bindings"), &_showBindingsDialog, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_Modal)) {
		const util::BindMap &bindings = _keybindingHandler.bindings();
		static const uint32_t TableFlags = ImGuiTableFlags_Reorderable | ImGuiTableFlags_Resizable |
										   ImGuiTableFlags_Hideable | ImGuiTableFlags_BordersInner |
										   ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY;
		const ImVec2 outerSize(0.0f, 400.0f);
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
				const core::String &deleteButton = core::string::format(ICON_LC_TRASH "##del-key-%i", n++);
				if (ImGui::Button(deleteButton.c_str())) {
					command::executeCommands(core::string::format("unbind \"%s\"", keyBinding.c_str()));
					// TODO: _lastExecutedCommand
				}
				ImGui::SameLine();
				ImGui::TextUnformatted(keyBinding.c_str());
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(command.c_str());
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(core::bindingContextString(pair.context).c_str());
				ImGui::TableNextColumn();
				if (!cmd) {
					ImGui::TextColored(core::Color::Red(), _("Failed to get command for %s"), command.c_str());
				} else {
					ImGui::TextUnformatted(cmd->help().c_str());
				}
			}
			ImGui::EndTable();
		}
		if (!_uiKeyMaps.empty()) {
			const int currentKeyMap = _uiKeyMap->intVal();
			if (ImGui::BeginCombo(_("Keymap"), _uiKeyMaps[(int)currentKeyMap].c_str(), ImGuiComboFlags_None)) {
				for (int i = 0; i < (int)_uiKeyMaps.size(); ++i) {
					const bool selected = i == currentKeyMap;
					if (ImGui::Selectable(_uiKeyMaps[i].c_str(), selected)) {
						_uiKeyMap->setVal(core::string::toString(i));
					}
					if (selected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
		} else {
			if (ImGui::Button(_("Reset to default"))) {
				_resetKeybindings = true;
			}
		}
		ImGui::SameLine();
		ImGui::InputText(_("Filter"), &_bindingsFilter);
	}
	ImGui::End();
}

void IMGUIApp::renderCvarDialog() {
	if (ImGui::Begin(_("Configuration variables"), &_showCvarDialog, ImGuiWindowFlags_AlwaysAutoResize)) {
		static const uint32_t TableFlags = ImGuiTableFlags_Reorderable | ImGuiTableFlags_Resizable |
										   ImGuiTableFlags_Hideable | ImGuiTableFlags_BordersInner |
										   ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY;
		const ImVec2 outerSize(0.0f, 400.0f);
		if (ImGui::BeginTable("##cvars", 4, TableFlags, outerSize)) {
			ImGui::TableSetupColumn(_("Name"), ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn(_("Value"), ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("##reset", ImGuiTableFlags_SizingFixedFit);
			ImGui::TableSetupColumn(_("Description"), ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableHeadersRow();

			core::Var::visit([](const core::VarPtr &var) {
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(var->name().c_str());
				ImGui::TableNextColumn();
				const core::String type = "##" + var->name();
				if (var->typeIsBool()) {
					bool value = var->boolVal();
					if (ImGui::Checkbox(type.c_str(), &value)) {
						var->setVal(value);
					}
				} else {
					core::String value = var->strVal();
					if (ImGui::InputText(type.c_str(), &value)) {
						var->setVal(value);
					}
				}
				ImGui::TableNextColumn();
				if (ImGui::Button(_("Reset"))) {
					var->reset();
				}
				ImGui::TooltipTextUnformatted(_("Reset to default value"));
				ImGui::TableNextColumn();
				ImGui::Text("%s", var->help() ? var->help() : "");
			});
			ImGui::EndTable();
		}
		if (ImGui::Button(_("Close"))) {
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
		const ImVec2 outerSize(0.0f, 400.0f);
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
		if (ImGui::Button(_("Close"))) {
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

app::AppState IMGUIApp::onRunning() {
	core_trace_scoped(IMGUIAppOnRunning);
	app::AppState state = Super::onRunning();

	if (state != app::AppState::Running) {
		return state;
	}
	video::clear(video::ClearFlag::Color);

	_console.update(_deltaFrameSeconds);

	if (_uiKeyMap->isDirty() || _resetKeybindings) {
		_keybindingHandler.clear();
		loadKeymap(_uiKeyMap->intVal());
		_uiKeyMap->markClean();
		_resetKeybindings = false;
	}

	const float dpiScale = core_max(0.1f, ImGui::GetMainViewport()->DpiScale);
	if (_uiFontSize->isDirty() || glm::abs(dpiScale - _dpiScale) > 0.001f) {
		_dpiScale = dpiScale;
		loadFonts();
		_uiFontSize->markClean();
	}

	if (_uistyle->isDirty()) {
		setColorTheme();
		_uistyle->markClean();
	}

	{
		core_trace_scoped(IMGUIAppBeforeUI);
		beforeUI();
	}

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	const bool renderUI = _renderUI->boolVal();
	if (renderUI) {
		core_trace_scoped(IMGUIAppOnRenderUI);
		onRenderUI();
		_console.render(_lastExecutedCommand);

#ifdef IMGUI_ENABLE_TEST_ENGINE
		ImGuiTestEngine_ShowTestEngineWindows(_imguiTestEngine, nullptr);
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
		if (_showFileDialog && _fileDialog.showFileDialog(_fileDialogOptions, buf, _fileDialogMode, &formatDesc)) {
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
	return app::AppState::Running;
}

void IMGUIApp::loadKeymap(int keymap) {
	_keybindingHandler.registerBinding("escape", "ui_close", "ui");
}

void IMGUIApp::languageOption() {
	const core::VarPtr &languageVar = core::Var::getSafe(cfg::CoreLanguage);
	core::String currentLanguage = languageVar->strVal();
	if (ImGui::BeginCombo(_("Language"), languageVar->strVal().c_str())) {
		const core::DynamicArray<app::Language> &languages = _dictManager.getLanguages();
		app::Language currentLang = app::Language::fromEnv(currentLanguage);
		for (const auto &lang : languages) {
			bool isSelected = lang == currentLang;
			if (ImGui::Selectable(lang.str().c_str(), isSelected)) {
				languageVar->setVal(lang.str());
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

app::AppState IMGUIApp::onCleanup() {
#ifdef IMGUI_ENABLE_TEST_ENGINE
	ImGuiTestEngine_Stop(_imguiTestEngine);
#endif
	if (_imguiBackendInitialized) {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		_imguiBackendInitialized = false;
	}
	if (ImGui::GetCurrentContext() != nullptr) {
		ImPlot::DestroyContext();
		ImGui::DestroyPlatformWindows();
		ImGui::DestroyContext();
	}
#ifdef IMGUI_ENABLE_TEST_ENGINE
	ImGuiTestEngine_DestroyContext(_imguiTestEngine);
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

} // namespace ui
