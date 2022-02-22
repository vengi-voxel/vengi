/**
 * @file
 */

#include "IMGUIApp.h"

#include "core/BindingContext.h"
#include "dearimgui/backends/imgui_impl_sdl.h"
#include "io/Filesystem.h"
#include "command/Command.h"
#include "core/Var.h"
#include "core/TimeProvider.h"
#include "core/Color.h"
#include "core/UTF8.h"
#include "core/Common.h"
#include "core/ArrayLength.h"
#include "core/Log.h"
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
#include "IconsFontAwesome5.h"
#include "IconsForkAwesome.h"
#include "IMGUIStyle.h"
#include "FileDialog.h"

#include <SDL.h>
#include <SDL_syswm.h>
#include <SDL_events.h>
#include <string.h>
#include <glm/mat4x4.hpp>

namespace ui {
namespace imgui {

IMGUIApp::IMGUIApp(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, size_t threadPoolSize) :
		Super(metric, filesystem, eventBus, timeProvider, threadPoolSize) {
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
	strncpy(ev.text.text, text.c_str(), sizeof(ev.text.text));
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
	}
	return true;
}

bool IMGUIApp::onKeyRelease(int32_t key, int16_t modifier) {
	if (_console.isActive()) {
		return true;
	}
	if (!Super::onKeyRelease(key, modifier)) {
		SDL_Event ev {};
		ev.type = SDL_KEYUP;
		ev.key.keysym.scancode = (SDL_Scancode)SDL_SCANCODE_UNKNOWN;
		ev.key.keysym.sym = key;
		ev.key.keysym.mod = modifier;
		ImGui_ImplSDL2_ProcessEvent(&ev);
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
	core::Var::get(cfg::UILastFilter, "0");
	core::Var::get(cfg::UIStyle, "0");
	core::Var::get(cfg::UIShowHidden, "false")->setHelp("Show hidden file system entities");
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
	ImFontConfig fontCfg;
	fontCfg.MergeMode = true;
	static const ImWchar rangesBasic[] = {
		0x0020, 0x00FF, // Basic Latin + Latin Supplement
		0x03BC, 0x03BC, // micro
		0x03C3, 0x03C3, // small sigma
		0x2013, 0x2013, // en dash
		0x2264, 0x2264, // less-than or equal to
		0,
	};
	io.Fonts->AddFontFromMemoryCompressedTTF(ArimoRegular_compressed_data, ArimoRegular_compressed_size,
											_uiFontSize->floatVal(), nullptr, rangesBasic);

	static const ImWchar rangesFAIcons[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
	io.Fonts->AddFontFromMemoryCompressedTTF(FontAwesomeSolid_compressed_data, FontAwesomeSolid_compressed_size,
											_uiFontSize->floatVal(), &fontCfg, rangesFAIcons);

	static const ImWchar rangesFKIcons[] = {ICON_MIN_FK, ICON_MAX_FK, 0};
	io.Fonts->AddFontFromMemoryCompressedTTF(ForkAwesomeWebFont_compressed_data, ForkAwesomeWebFont_compressed_size,
											_uiFontSize->floatVal(), &fontCfg, rangesFKIcons);

	_bigFont = io.Fonts->AddFontFromMemoryCompressedTTF(ArimoRegular_compressed_data, ArimoRegular_compressed_size,
											_uiFontSize->floatVal() * 2.0f);
	_defaultFont = io.Fonts->AddFontFromMemoryCompressedTTF(ArimoRegular_compressed_data, ArimoRegular_compressed_size,
											_uiFontSize->floatVal());
	_smallFont = io.Fonts->AddFontFromMemoryCompressedTTF(ArimoRegular_compressed_data, ArimoRegular_compressed_size,
											_uiFontSize->floatVal() * 0.8f);

	unsigned char *pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	video::TextureConfig cfg;
	cfg.format(video::TextureFormat::RGBA);
	video::bindTexture(video::TextureUnit::Upload, cfg.type(), _texture);
	video::setupTexture(cfg);
	video::uploadTexture(cfg.type(), cfg.format(), width, height, pixels, 0);
	io.Fonts->TexID = (ImTextureID)(intptr_t)_texture;
}

static void* _imguiAlloc(size_t size, void*) {
	return core_malloc(size);
}

static void _imguiFree(void *mem, void*) {
	core_free(mem);
}

static void _rendererRenderWindow(ImGuiViewport *viewport, void *renderArg) {
	if (!(viewport->Flags & ImGuiViewportFlags_NoRendererClear)) {
		const glm::vec4 clearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		video::clearColor(clearColor);
		video::clear(video::ClearFlag::Color);
	}
	ImGuiIO &io = ImGui::GetIO();
	IMGUIApp *app = (IMGUIApp *)io.BackendRendererUserData;
	app->executeDrawCommands(viewport->DrawData);
}

app::AppState IMGUIApp::onInit() {
	const app::AppState state = Super::onInit();
	video::checkError();
	if (state != app::AppState::Running) {
		return state;
	}

	if (!_shader.setup()) {
		Log::error("Could not load the ui shader");
		return app::AppState::InitFailure;
	}

	_bufferIndex = _vbo.create();
	if (_bufferIndex < 0) {
		Log::error("Failed to create ui vertex buffer");
		return app::AppState::InitFailure;
	}
	_vbo.setMode(_bufferIndex, video::BufferMode::Stream);
	_indexBufferIndex = _vbo.create(nullptr, 0, video::BufferType::IndexBuffer);
	if (_indexBufferIndex < 0) {
		Log::error("Failed to create ui index buffer");
		return app::AppState::InitFailure;
	}
	_vbo.setMode(_indexBufferIndex, video::BufferMode::Stream);

	_vbo.addAttribute(_shader.getColorAttribute(_bufferIndex, &ImDrawVert::r, true));
	_vbo.addAttribute(_shader.getTexcoordAttribute(_bufferIndex, &ImDrawVert::u));
	_vbo.addAttribute(_shader.getPosAttribute(_bufferIndex, &ImDrawVert::x));

	IMGUI_CHECKVERSION();
	ImGui::SetAllocatorFunctions(_imguiAlloc, _imguiFree);
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	if (!isSingleWindowMode()) {
		// io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		// io.ConfigViewportsNoAutoMerge = true;
		// io.ConfigViewportsNoTaskBarIcon = true;
	}
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports;
	io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;

	if (_persistUISettings) {
		const core::String iniFile = _appname + "-imgui.ini";
		_writePathIni = _filesystem->writePath(iniFile.c_str());
		io.IniFilename = _writePathIni.c_str();
	} else {
		io.IniFilename = nullptr;
	}
	const core::String logFile = _appname + "-imgui.log";
	_writePathLog = _filesystem->writePath(logFile.c_str());
	io.LogFilename = _writePathLog.c_str();
	io.DisplaySize = _windowDimension;

	// Setup backend capabilities flags
	io.BackendRendererUserData = (void *)this;
	io.BackendRendererName = _appname.c_str();
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset; // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
	io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports; // We can create multi-viewports on the Renderer side (optional)
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
		platform_io.Renderer_RenderWindow = _rendererRenderWindow;
	}

	_texture = video::genTexture();
	loadFonts();

	switch (core::Var::get(cfg::UIStyle)->intVal()) {
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
	}

	ImGui_ImplSDL2_InitForOpenGL(_window, _rendererContext);

	ImGui::SetColorEditOptions(ImGuiColorEditFlags_Float);

	_console.init();

	Log::debug("Set up imgui");

	return state;
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

	core_assert(_bufferIndex > -1);
	core_assert(_indexBufferIndex > -1);

	{
		core_trace_scoped(IMGUIAppBeforeUI);
		beforeUI();
	}

	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	const bool renderUI = _renderUI->boolVal();
	if (renderUI) {
		core_trace_scoped(IMGUIAppOnRenderUI);
		onRenderUI();

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
			if (ImGui::Begin("Bindings", &_showBindingsDialog, ImGuiWindowFlags_NoScrollbar)) {
				const util::BindMap& bindings = _keybindingHandler.bindings();
				static const uint32_t TableFlags =
					ImGuiTableFlags_Reorderable | ImGuiTableFlags_Resizable | ImGuiTableFlags_Hideable |
					ImGuiTableFlags_BordersInner | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY;
				const ImVec2 &outerSize = ImGui::GetContentRegionAvail();
				if (ImGui::BeginTable("##bindingslist", 3, TableFlags, outerSize)) {
					ImGui::TableSetupColumn("Keys##bindingslist", ImGuiTableColumnFlags_WidthFixed);
					ImGui::TableSetupColumn("Command##bindingslist", ImGuiTableColumnFlags_WidthFixed);
					ImGui::TableSetupColumn("Description##bindingslist", ImGuiTableColumnFlags_WidthStretch);
					ImGui::TableHeadersRow();

					for (util::BindMap::const_iterator i = bindings.begin(); i != bindings.end(); ++i) {
						const util::CommandModifierPair& pair = i->second;
						const core::String& command = pair.command;
						const core::String& keyBinding = _keybindingHandler.getKeyBindingsString(command.c_str(), pair.count);
						ImGui::TableNextColumn();
						ImGui::TextUnformatted(keyBinding.c_str());
						ImGui::TableNextColumn();
						ImGui::TextUnformatted(command.c_str());
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
			}
			ImGui::End();
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
		if (_fileDialog.showFileDialog(&_showFileDialog, buf, sizeof(buf), _fileDialogMode)) {
			if (buf[0] != '\0') {
				_fileDialogCallback(buf);
			}
			_showFileDialog = false;
		}

		if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) {
			core::setBindingContext(core::BindingContext::UserInterface);
		} else {
			core::setBindingContext(core::BindingContext::World);
		}
	} else {
		core::setBindingContext(core::BindingContext::World);
	}

	const math::Rect<int> rect(0, 0, _frameBufferDimension.x, _frameBufferDimension.y);
	_console.render(rect, _deltaFrameSeconds);
	ImGui::EndFrame();
	ImGui::Render();

	executeDrawCommands(ImGui::GetDrawData());

	// Update and Render additional Platform Windows
	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		video::activateContext(_window, _rendererContext);
	}

	video::scissor(0, 0, _frameBufferDimension.x, _frameBufferDimension.y);
	return app::AppState::Running;
}

void IMGUIApp::executeDrawCommands(ImDrawData* drawData) {
	core_trace_scoped(ExecuteDrawCommands);

	const int fbWidth = (int)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
	const int fbHeight = (int)(drawData->DisplaySize.y * drawData->FramebufferScale.y);
	if (fbWidth <= 0 || fbHeight <= 0) {
		return;
	}

	video::ScopedViewPort scopedViewPort(0, 0, fbWidth, fbHeight);

	video::enable(video::State::Blend);
	video::blendEquation(video::BlendEquation::Add);
	video::blendFunc(video::BlendMode::SourceAlpha, video::BlendMode::OneMinusSourceAlpha);
	video::disable(video::State::CullFace);
	video::disable(video::State::DepthTest);
	video::disable(video::State::StencilTest);
	video::disable(video::State::PrimitiveRestart);
	video::enable(video::State::Scissor);
	video::polygonMode(video::Face::FrontAndBack, video::PolygonMode::Solid);

	const float L = drawData->DisplayPos.x;
	const float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
	float T = drawData->DisplayPos.y;
	float B = drawData->DisplayPos.y + drawData->DisplaySize.y;
	if (!video::isClipOriginLowerLeft()) {
		float tmp = T;
		T = B;
		B = tmp;
	}
	const glm::mat4 orthoMatrix = {
		{2.0f / (R - L), 0.0f, 0.0f, 0.0f},
		{0.0f, 2.0f / (T - B), 0.0f, 0.0f},
		{0.0f, 0.0f, -1.0f, 0.0f},
		{(R + L) / (L - R), (T + B) / (B - T), 0.0f, 1.0f},
	};
	video::ScopedShader scopedShader(_shader);
	_shader.setViewprojection(orthoMatrix);
	_shader.setModel(glm::mat4(1.0f));
	_shader.setTexture(video::TextureUnit::Zero);

	int64_t drawCommands = 0;

	ImVec2 clipOff = drawData->DisplayPos;		   // (0,0) unless using multi-viewports
	ImVec2 clipScale = drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

	for (int n = 0; n < drawData->CmdListsCount; ++n) {
		const ImDrawList* cmdList = drawData->CmdLists[n];

		core_assert_always(_vbo.update(_bufferIndex, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert), true));
		core_assert_always(_vbo.update(_indexBufferIndex, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx), true));
		video::ScopedBuffer scopedBuf(_vbo);

		for (int i = 0; i < cmdList->CmdBuffer.Size; ++i) {
			const ImDrawCmd* cmd = &cmdList->CmdBuffer[i];
			if (cmd->UserCallback) {
				cmd->UserCallback(cmdList, cmd);
			} else {
				// Project scissor/clipping rectangles into framebuffer space
				ImVec2 clipMin((cmd->ClipRect.x - clipOff.x) * clipScale.x,
								(cmd->ClipRect.y - clipOff.y) * clipScale.y);
				ImVec2 clipMax((cmd->ClipRect.z - clipOff.x) * clipScale.x,
								(cmd->ClipRect.w - clipOff.y) * clipScale.y);
				if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y) {
					continue;
				}
				video::scissor((int)clipMin.x, (int)clipMin.y, (int)clipMax.x - (int)clipMin.x, (int)clipMax.y - (int)clipMin.y);
				video::bindTexture(video::TextureUnit::Zero, video::TextureType::Texture2D, (video::Id)(intptr_t)cmd->TextureId);
				video::drawElementsBaseVertex<ImDrawIdx>(video::Primitive::Triangles, cmd->ElemCount, (int)cmd->IdxOffset, (int)cmd->VtxOffset);
			}
			++drawCommands;
		}
	}
	// Recreate the VAO every time (this is to easily allow multiple GL contexts to be rendered to. VAO are not shared
	// among GL contexts)
	_vbo.destroyVertexArray();
	core_trace_plot("UIDrawCommands", drawCommands);
}

app::AppState IMGUIApp::onCleanup() {
	ImGui_ImplSDL2_Shutdown();
	if (ImGui::GetCurrentContext() != nullptr) {
		ImGui::DestroyPlatformWindows();
		ImGui::DestroyContext();
	}
	_console.shutdown();
	_shader.shutdown();
	_vbo.shutdown();
	_indexBufferIndex = -1;
	_bufferIndex = -1;
	return Super::onCleanup();
}

void IMGUIApp::fileDialog(const std::function<void(const core::String&)>& callback, OpenFileMode mode, const io::FormatDescription* formats, const core::String &filename) {
	_showFileDialog = true;
	_fileDialogCallback = callback;
	_fileDialogMode = mode;
	_fileDialog.openDir(formats);
}

}
}
