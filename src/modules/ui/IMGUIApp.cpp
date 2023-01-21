/**
 * @file
 */

#include "IMGUIApp.h"

#include "core/BindingContext.h"
#include "dearimgui/backends/imgui_impl_sdl.h"
#include "dearimgui/backends/imgui_impl_opengl3.h"
#include "engine-config.h"
#include "io/Filesystem.h"
#include "command/Command.h"
#include "core/Var.h"
#include "core/TimeProvider.h"
#include "core/Color.h"
#include "core/UTF8.h"
#include "core/Common.h"
#include "core/ArrayLength.h"
#include "core/Log.h"
#include "io/FormatDescription.h"
#include "math/Rect.h"
#include "video/Renderer.h"
#include "video/Shader.h"
#include "video/ScopedViewPort.h"
#include "video/TextureConfig.h"
#include "video/Types.h"

#include "IMGUIEx.h"
#include "FontAwesomeSolid.h"
#include "ForkAwesomeWebFont.h"
#include "ArimoRegular.h"
#include "IconsFontAwesome6.h"
#include "IconsForkAwesome.h"
#include "IMGUIStyle.h"
#include "FileDialog.h"

#include <SDL.h>
#include <SDL_syswm.h>
#include <SDL_events.h>
#include <string.h>
#include <glm/mat4x4.hpp>

namespace ui {

IMGUIApp::IMGUIApp(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider, size_t threadPoolSize)
	: Super(filesystem, timeProvider, threadPoolSize) {
}

IMGUIApp::~IMGUIApp() {
}

void IMGUIApp::onMouseMotion(void *windowHandle, int32_t x, int32_t y, int32_t relX, int32_t relY) {
	Super::onMouseMotion(windowHandle, x, y, relX, relY);

	SDL_Event ev {};
	ev.type = SDL_MOUSEMOTION;
	ev.motion.x = x;
	ev.motion.y = y;
	ev.motion.windowID = SDL_GetWindowID((SDL_Window*)windowHandle);
	ImGui_ImplSDL2_ProcessEvent(&ev);
}

bool IMGUIApp::onMouseWheel(int32_t x, int32_t y) {
	if (_console.onMouseWheel(x, y)) {
		return true;
	}
	if (!Super::onMouseWheel(x, y)) {
		SDL_Event ev {};
		ev.type = SDL_MOUSEWHEEL;
		ev.wheel.x = x;
		ev.wheel.y = y;
		ImGui_ImplSDL2_ProcessEvent(&ev);
	}
	return true;
}

void IMGUIApp::onMouseButtonRelease(int32_t x, int32_t y, uint8_t button) {
	if (_console.isActive()) {
		return;
	}
	Super::onMouseButtonRelease(x, y, button);
	SDL_Event ev {};
	ev.type = SDL_MOUSEBUTTONUP;
	ev.button.button = button;
	ev.button.x = x;
	ev.button.y = y;
	ImGui_ImplSDL2_ProcessEvent(&ev);
}

void IMGUIApp::onMouseButtonPress(int32_t x, int32_t y, uint8_t button, uint8_t clicks) {
	if (_console.onMouseButtonPress(x, y, button)) {
		return;
	}
	Super::onMouseButtonPress(x, y, button, clicks);
	SDL_Event ev {};
	ev.type = SDL_MOUSEBUTTONDOWN;
	ev.button.button = button;
	ev.button.clicks = clicks;
	ev.button.x = x;
	ev.button.y = y;
	ImGui_ImplSDL2_ProcessEvent(&ev);
}

bool IMGUIApp::onTextInput(const core::String& text) {
	if (_console.onTextInput(text)) {
		return true;
	}
	SDL_Event ev {};
	ev.type = SDL_TEXTINPUT;
	core::string::strncpyz(text.c_str(), sizeof(ev.text.text), ev.text.text, sizeof(ev.text.text));
	ImGui_ImplSDL2_ProcessEvent(&ev);
	return true;
}

bool IMGUIApp::onKeyPress(int32_t key, int16_t modifier) {
	if (_console.onKeyPress(key, modifier)) {
		return true;
	}
	if (!Super::onKeyPress(key, modifier)) {
		SDL_Event ev {};
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
	if (_console.isActive()) {
		return true;
	}
	if (!Super::onKeyRelease(key, modifier) || _keys.has(key)) {
		SDL_Event ev {};
		ev.type = SDL_KEYUP;
		ev.key.keysym.scancode = (SDL_Scancode)SDL_SCANCODE_UNKNOWN;
		ev.key.keysym.sym = key;
		ev.key.keysym.mod = modifier;
		ImGui_ImplSDL2_ProcessEvent(&ev);
		_keys.remove(key);
	}
	return true;
}

bool IMGUIApp::handleSDLEvent(SDL_Event& event) {
	const bool state = Super::handleSDLEvent(event);
	if (event.type == SDL_WINDOWEVENT) {
		ImGui_ImplSDL2_ProcessEvent(&event);
	}
	return state;
}

app::AppState IMGUIApp::onConstruct() {
	const app::AppState state = Super::onConstruct();
	_console.construct();
	_lastDirectory = core::Var::get(cfg::UILastDirectory, io::filesystem()->homePath().c_str());
	core::Var::get(cfg::UILastFilter, "0", "The last selected file type filter in the file dialog");

	const char *uiStyleDefaultValue = "0";
	if (!isDarkMode()) {
		uiStyleDefaultValue = "2";
	}
	_uistyle = core::Var::get(cfg::UIStyle, uiStyleDefaultValue, "Change the ui colors - [0-3]", [](const core::String &val) {
		const int themeIdx = core::string::toInt(val);
		return themeIdx >= 0 && themeIdx <= 3;
	});
	core::Var::get(cfg::UIFileDialogShowHidden, "false", "Show hidden file system entities");
	core::Var::get(cfg::UINotifyDismissMillis, "3000", "Timeout for notifications in millis");
	_renderUI = core::Var::get(cfg::ClientRenderUI, "true");
	_showMetrics = core::Var::get(cfg::UIShowMetrics, "false", core::CV_NOPERSIST);
	_uiFontSize = core::Var::get(cfg::UIFontSize, "14", -1, "Allow to change the ui font size",
								[](const core::String &val) {
									const float size = core::string::toFloat(val);
									return size >= 2.0f;
								});
	command::Command::registerCommand("ui_showtextures", [&] (const command::CmdArgs& args) {
		_showTexturesDialog = true;
	});
	return state;
}

void IMGUIApp::loadFonts() {
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->Clear();

	const ImWchar *rangesBasic = io.Fonts->GetGlyphRangesDefault();

	ImFontConfig fontCfg;
	fontCfg.MergeMode = true;

	ImFontConfig bigFontIconCfg;
	bigFontIconCfg.MergeMode = true;
	bigFontIconCfg.GlyphMinAdvanceX = _uiFontSize->floatVal() * 2.0f;
	bigFontIconCfg.GlyphMaxAdvanceX = bigFontIconCfg.GlyphMinAdvanceX;

	ImFontConfig fontIconCfg;
	fontIconCfg.MergeMode = true;
	fontIconCfg.GlyphMinAdvanceX = _uiFontSize->floatVal();
	fontIconCfg.GlyphMaxAdvanceX = bigFontIconCfg.GlyphMinAdvanceX;

	static const ImWchar rangesFAIcons[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
	static const ImWchar rangesFKIcons[] = {ICON_MIN_FK, ICON_MAX_FK, 0};

	_defaultFont = io.Fonts->AddFontFromMemoryCompressedTTF(ArimoRegular_compressed_data, ArimoRegular_compressed_size,
											_uiFontSize->floatVal(), nullptr, rangesBasic);
	io.Fonts->AddFontFromMemoryCompressedTTF(FontAwesomeSolid_compressed_data, FontAwesomeSolid_compressed_size,
											_uiFontSize->floatVal(), &fontIconCfg, rangesFAIcons);
	io.Fonts->AddFontFromMemoryCompressedTTF(ForkAwesomeWebFont_compressed_data, ForkAwesomeWebFont_compressed_size,
											_uiFontSize->floatVal(), &fontIconCfg, rangesFKIcons);

	_bigFont = io.Fonts->AddFontFromMemoryCompressedTTF(ArimoRegular_compressed_data, ArimoRegular_compressed_size,
											_uiFontSize->floatVal() * 2.0f, nullptr, rangesBasic);

	_bigIconFont = io.Fonts->AddFontFromMemoryCompressedTTF(FontAwesomeSolid_compressed_data, FontAwesomeSolid_compressed_size,
											_uiFontSize->floatVal() * 1.5f, &bigFontIconCfg, rangesFAIcons);
	io.Fonts->AddFontFromMemoryCompressedTTF(ForkAwesomeWebFont_compressed_data, ForkAwesomeWebFont_compressed_size,
											_uiFontSize->floatVal() * 1.5f, &bigFontIconCfg, rangesFKIcons);

	_smallFont = io.Fonts->AddFontFromMemoryCompressedTTF(ArimoRegular_compressed_data, ArimoRegular_compressed_size,
											_uiFontSize->floatVal() * 0.8f, nullptr, rangesBasic	);
	io.Fonts->AddFontFromMemoryCompressedTTF(FontAwesomeSolid_compressed_data, FontAwesomeSolid_compressed_size,
											_uiFontSize->floatVal(), &fontIconCfg, rangesFAIcons);
	io.Fonts->AddFontFromMemoryCompressedTTF(ForkAwesomeWebFont_compressed_data, ForkAwesomeWebFont_compressed_size,
											_uiFontSize->floatVal(), &fontIconCfg, rangesFKIcons);

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

static void* _imguiAlloc(size_t size, void*) {
	return core_malloc(size);
}

static void _imguiFree(void *mem, void*) {
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

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	if (!isSingleWindowMode()) {
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		// io.ConfigViewportsNoAutoMerge = true;
		// io.ConfigViewportsNoTaskBarIcon = true;
	}
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports;
	io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;
	// test dpi related issues on linux with
	// xrandr | grep connected | grep -v disconnected | awk '{print $1}'
	// xrandr --output <screen-name> --scale 1.6x1.6

	if (_persistUISettings) {
		const core::String iniFile = core::string::format("%s-%i-imgui.ini", _appname.c_str(), _iniVersion);
		_writePathIni = _filesystem->writePath(iniFile.c_str());
		io.IniFilename = _writePathIni.c_str();
	} else {
		io.IniFilename = nullptr;
	}
	const core::String logFile = _appname + "-imgui.log";
	_writePathLog = _filesystem->writePath(logFile.c_str());
	io.LogFilename = _writePathLog.c_str();
	io.DisplaySize = _windowDimension;

	loadFonts();
	setColorTheme();
	_imguiBackendInitialized = ImGui_ImplSDL2_InitForOpenGL(_window, _rendererContext);
	ImGui_ImplOpenGL3_Init(nullptr);

	ImGui::SetColorEditOptions(ImGuiColorEditFlags_Float);

	_console.init();

	Log::debug("Set up imgui");

	return state;
}

void IMGUIApp::setColorTheme() {
	switch (_uistyle->intVal()) {
	case 0:
		ImGui::StyleColorsCorporateGrey();
		break;
	case 1:
		ImGui::StyleColorsDark();
		break;
	case 2:
		ImGui::StyleColorsLight();
		break;
	case 3:
		ImGui::StyleColorsClassic();
		break;
	default:
		_uistyle->setVal("0");
		break;
	}
}

void IMGUIApp::beforeUI() {
}

app::AppState IMGUIApp::onRunning() {
	core_trace_scoped(IMGUIAppOnRunning);
	app::AppState state = Super::onRunning();

	if (state != app::AppState::Running) {
		return state;
	}
	video::clear(video::ClearFlag::Color);

	_console.update(_deltaFrameSeconds);

	if (_uiFontSize->isDirty()) {
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
		if (_console.isActive()) {
			if (ImGui::IsPopupOpen(0u, ImGuiPopupFlags_AnyPopupId)) {
				_console.toggle();
			}
		}

		if (_showTexturesDialog) {
			if (ImGui::Begin("Textures", &_showTexturesDialog)) {
				const core::Set<video::Id>& textures = video::textures();
				const ImVec2 size(512, 512);
				int textureCnt = 0;
				for (const auto& e : textures) {
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

		if (_showBindingsDialog) {
			ImGui::OpenPopup("Bindings");
			_showBindingsDialog = false;
		}
		bool showBindings_unused = true;
		if (ImGui::BeginPopupModal("Bindings", &showBindings_unused, ImGuiWindowFlags_AlwaysAutoResize)) {
			const util::BindMap& bindings = _keybindingHandler.bindings();
			static const uint32_t TableFlags =
				ImGuiTableFlags_Reorderable | ImGuiTableFlags_Resizable | ImGuiTableFlags_Hideable |
				ImGuiTableFlags_BordersInner | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY;
			const ImVec2 outerSize(0.0f, 400.0f);
			if (ImGui::BeginTable("##bindingslist", 4, TableFlags, outerSize)) {
				ImGui::TableSetupColumn("Keys##bindingslist", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Command##bindingslist", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Context##bindingslist", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Description##bindingslist", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableHeadersRow();

				int n = 0;
				for (util::BindMap::const_iterator i = bindings.begin(); i != bindings.end(); ++i) {
					const util::CommandModifierPair& pair = i->second;
					const core::String& command = pair.command;
					const core::String& keyBinding = _keybindingHandler.getKeyBindingsString(command.c_str(), pair.count);
					ImGui::TableNextColumn();
					// TODO: change binding
					const core::String &deleteButton = core::string::format(ICON_FA_TRASH "##del-key-%i", n++);
					if (ImGui::Button(deleteButton.c_str())) {
						command::executeCommands(core::string::format("unbind \"%s\"", keyBinding.c_str()));
					}
					ImGui::SameLine();
					ImGui::TextUnformatted(keyBinding.c_str());
					ImGui::TableNextColumn();
					ImGui::TextUnformatted(command.c_str());
					ImGui::TableNextColumn();
					ImGui::TextUnformatted(core::bindingContextString(pair.context).c_str());
					const command::Command* cmd = nullptr;
					if (command.contains(" ")) {
						cmd = command::Command::getCommand(command.substr(0, command.find(" ")));
					} else {
						cmd = command::Command::getCommand(command);
					}
					ImGui::TableNextColumn();
					if (!cmd) {
						ImGui::TextColored(core::Color::Red, "Failed to get command for %s", command.c_str());
					} else {
						ImGui::TextUnformatted(cmd->help() ? cmd->help() : "");
					}
				}
				ImGui::EndTable();
			}
			if (ImGui::Button("Reset to default")) {
				resetKeybindings();
			}
			// TODO: add binding
			ImGui::EndPopup();
		}

		bool showMetrics = _showMetrics->boolVal();
		if (showMetrics) {
			ImGui::ShowMetricsWindow(&showMetrics);
			if (!showMetrics) {
				_showMetrics->setVal("false");
			}
		}
		_console.renderNotifications();

		char buf[512] = "";
		const io::FormatDescription *formatDesc = nullptr;
		if (_fileDialog.showFileDialog(&_showFileDialog, _fileDialogOptions, buf, sizeof(buf), _fileDialogMode, &formatDesc)) {
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

	const math::Rect<int> rect(0, 0, _frameBufferDimension.x, _frameBufferDimension.y);
	_console.render(rect, _deltaFrameSeconds);
	ImGui::EndFrame();
	ImGui::Render();

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	// Update and Render additional Platform Windows
	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		video::activateContext(_window, _rendererContext);
	}

	video::scissor(0, 0, _frameBufferDimension.x, _frameBufferDimension.y);
	return app::AppState::Running;
}

app::AppState IMGUIApp::onCleanup() {
	if (_imguiBackendInitialized) {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		_imguiBackendInitialized = false;
	}
	if (ImGui::GetCurrentContext() != nullptr) {
		ImGui::DestroyPlatformWindows();
		ImGui::DestroyContext();
	}
	_console.shutdown();
	return Super::onCleanup();
}

void IMGUIApp::fileDialog(const video::FileDialogSelectionCallback& callback, const video::FileDialogOptions& options, video::OpenFileMode mode, const io::FormatDescription* formats, const core::String &filename) {
	_showFileDialog = true;
	_fileDialogCallback = callback;
	_fileDialogOptions = options;
	_fileDialogMode = mode;
	_fileDialog.openDir(formats, filename);
}

}
