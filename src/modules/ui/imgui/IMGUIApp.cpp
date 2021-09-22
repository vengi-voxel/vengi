/**
 * @file
 */

#include "IMGUIApp.h"

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

#include "IMGUI.h"
#include "FontAwesomeSolid.h"
#include "ForkAwesomeWebFont.h"
#include "ArimoRegular.h"
#include "IconsFontAwesome5.h"
#include "IconsForkAwesome.h"
#include "IMGUIStyle.h"
#include "FileDialog.h"

#include <SDL.h>

namespace ui {
namespace imgui {

IMGUIApp::IMGUIApp(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, size_t threadPoolSize) :
		Super(metric, filesystem, eventBus, timeProvider, threadPoolSize), _camera(video::CameraType::FirstPerson, video::CameraMode::Orthogonal) {
}

IMGUIApp::~IMGUIApp() {
}

bool IMGUIApp::onMouseWheel(int32_t x, int32_t y) {
	if (_console.onMouseWheel(x, y)) {
		return true;
	}
	if (y > 0) {
		_mouseWheelY += 1;
	} else if (y < 0) {
		_mouseWheelY -= 1;
	}
	if (x > 0) {
		_mouseWheelX += 1;
	} else if (x < 0) {
		_mouseWheelX -= 1;
	}
	return Super::onMouseWheel(x, y);
}

void IMGUIApp::onMouseButtonRelease(int32_t x, int32_t y, uint8_t button) {
	if (_console.isActive()) {
		return;
	}
	Super::onMouseButtonRelease(x, y, button);
}

void IMGUIApp::onMouseButtonPress(int32_t x, int32_t y, uint8_t button, uint8_t clicks) {
	if (_console.onMouseButtonPress(x, y, button)) {
		return;
	}
	if (button == SDL_BUTTON_LEFT) {
		_mousePressed[0] = true;
	} else if (button == SDL_BUTTON_RIGHT) {
		_mousePressed[1] = true;
	} else if (button == SDL_BUTTON_MIDDLE) {
		_mousePressed[2] = true;
	}
	Super::onMouseButtonPress(x, y, button, clicks);
}

bool IMGUIApp::onTextInput(const core::String& text) {
	if (_console.onTextInput(text)) {
		return true;
	}
	ImGuiIO& io = ImGui::GetIO();
	io.AddInputCharactersUTF8(text.c_str());
	return true;
}

bool IMGUIApp::onKeyPress(int32_t key, int16_t modifier) {
	if (_console.onKeyPress(key, modifier)) {
		return true;
	}
	if (Super::onKeyPress(key, modifier)) {
		return true;
	}
	ImGuiIO& io = ImGui::GetIO();
	key &= ~SDLK_SCANCODE_MASK;
	core_assert(key >= 0 && key < lengthof(io.KeysDown));
	io.KeysDown[key] = true;
	const int16_t modifiers = SDL_GetModState();
	io.KeyShift = (modifiers & KMOD_SHIFT) != 0;
	io.KeyCtrl  = (modifiers & KMOD_CTRL) != 0;
	io.KeyAlt   = (modifiers & KMOD_ALT) != 0;
	io.KeySuper = (modifiers & KMOD_GUI) != 0;
	return false;
}

bool IMGUIApp::onKeyRelease(int32_t key, int16_t modifier) {
	if (_console.isActive()) {
		return true;
	}
	if (Super::onKeyRelease(key, modifier)) {
		return true;
	}
	ImGuiIO& io = ImGui::GetIO();
	key &= ~SDLK_SCANCODE_MASK;
	core_assert(key >= 0 && key < lengthof(io.KeysDown));
	io.KeysDown[key] = false;
	io.KeyShift = (modifier & KMOD_SHIFT) != 0;
	io.KeyCtrl  = (modifier & KMOD_CTRL) != 0;
	io.KeyAlt   = (modifier & KMOD_ALT) != 0;
#ifdef _WIN32
	io.KeySuper = false;
#else
	io.KeySuper = (modifier & KMOD_GUI) != 0;
#endif
	return true;
}

void IMGUIApp::onWindowClose(void *windowHandle) {
	Super::onWindowClose(windowHandle);
	if (ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle(windowHandle)) {
		viewport->PlatformRequestClose = true;
	}
}

void IMGUIApp::onWindowMoved(void *windowHandle) {
	Super::onWindowMoved(windowHandle);
	if (ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle(windowHandle)) {
		viewport->PlatformRequestMove = true;
	}
}

void IMGUIApp::onWindowFocusGained(void *windowHandle) {
	Super::onWindowFocusGained(windowHandle);
	ImGuiIO& io = ImGui::GetIO();
	io.AddFocusEvent(true);
}

void IMGUIApp::onWindowFocusLost(void *windowHandle) {
	Super::onWindowFocusLost(windowHandle);
	ImGuiIO& io = ImGui::GetIO();
	io.AddFocusEvent(false);
}

void IMGUIApp::onWindowResize(void *windowHandle, int windowWidth, int windowHeight) {
	Super::onWindowResize(windowHandle, windowWidth, windowHeight);
	if (ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle(windowHandle)) {
		viewport->PlatformRequestResize = true;
	}
	ImGuiIO& io = ImGui::GetIO();
	int w = _windowDimension.x;
	int h = _windowDimension.y;
	if (SDL_GetWindowFlags(_window) & SDL_WINDOW_MINIMIZED) {
		w = h = 0;
	}

	io.DisplaySize = _windowDimension;
	if (w > 0 && h > 0) {
		const float xScale = (float)_frameBufferDimension.x / (float)_windowDimension.x;
		const float yScale = (float)_frameBufferDimension.y / (float)_windowDimension.y;
		io.DisplayFramebufferScale = ImVec2(xScale, yScale);
	}

	_camera.init(glm::ivec2(0), frameBufferDimension(), windowDimension());
	_camera.update(0.0);
	video::ScopedShader scoped(_shader);
	_shader.setViewprojection(_camera.projectionMatrix());
	_shader.setModel(glm::mat4(1.0f));
}

app::AppState IMGUIApp::onConstruct() {
	const app::AppState state = Super::onConstruct();
	_console.construct();
	_lastDirectory = core::Var::get(cfg::UILastDirectory, io::filesystem()->homePath().c_str());
	_renderUI = core::Var::get(cfg::ClientRenderUI, "true");
	_showMetrics = core::Var::get(cfg::UIShowMetrics, "false", core::CV_NOPERSIST);
	_uiFontSize = core::Var::get(cfg::UIFontSize, "14", -1, "Allow to change the ui font size",
								[](const core::String &val) {
									const float size = core::string::toFloat(val);
									return size >= 2.0f;
								});
	return state;
}

static const char* _getClipboardText(void*) {
	const char* text = SDL_GetClipboardText();
	if (!text) {
		return nullptr;
	}
	const int len = SDL_strlen(text);
	if (len == 0) {
		SDL_free((void*) text);
		return "";
	}
	static ImVector<char> clipboardBuffer;
	// Optional branch to keep clipboardBuffer.capacity() low:
	if (len <= clipboardBuffer.capacity() && clipboardBuffer.capacity() > 512) {
		ImVector<char> emptyBuffer;
		clipboardBuffer.swap(emptyBuffer);
	}
	clipboardBuffer.resize(len + 1);
	SDL_strlcpy(&clipboardBuffer[0], text, clipboardBuffer.size());
	SDL_free((void*) text);
	return (const char*) &clipboardBuffer[0];
}

static void _setClipboardText(void*, const char* text) {
	SDL_SetClipboardText(text);
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
											_uiFontSize->floatVal() * _dpiFactor, nullptr, rangesBasic);

	static const ImWchar rangesFAIcons[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
	io.Fonts->AddFontFromMemoryCompressedTTF(FontAwesomeSolid_compressed_data, FontAwesomeSolid_compressed_size,
											_uiFontSize->floatVal() * _dpiFactor, &fontCfg, rangesFAIcons);

	static const ImWchar rangesFKIcons[] = {ICON_MIN_FK, ICON_MAX_FK, 0};
	io.Fonts->AddFontFromMemoryCompressedTTF(ForkAwesomeWebFont_compressed_data, ForkAwesomeWebFont_compressed_size,
											_uiFontSize->floatVal() * _dpiFactor, &fontCfg, rangesFKIcons);

	_bigFont = io.Fonts->AddFontFromMemoryCompressedTTF(ArimoRegular_compressed_data, ArimoRegular_compressed_size,
											_uiFontSize->floatVal() * 2.0f * _dpiFactor);
	_defaultFont = io.Fonts->AddFontFromMemoryCompressedTTF(ArimoRegular_compressed_data, ArimoRegular_compressed_size,
											_uiFontSize->floatVal() * _dpiFactor);
	_smallFont = io.Fonts->AddFontFromMemoryCompressedTTF(ArimoRegular_compressed_data, ArimoRegular_compressed_size,
											_uiFontSize->floatVal() * 0.8f * _dpiFactor);

	unsigned char *pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	video::TextureConfig cfg;
	cfg.format(video::TextureFormat::RGBA);
	video::bindTexture(video::TextureUnit::Upload, cfg.type(), _texture);
	video::setupTexture(cfg);
	video::uploadTexture(cfg.type(), cfg.format(), width, height, pixels, 0);
	io.Fonts->TexID = (void *) (intptr_t) _texture;
}

// Helper structure we store in the void* RenderUserData field of each ImGuiViewport to easily retrieve our backend
// data.
struct _imguiViewportData {
	SDL_Window *window = nullptr;
	uint32_t windowID = 0;
	bool windowOwned = false;
	void* renderContext = nullptr;

	~_imguiViewportData() {
		core_assert(window == nullptr && renderContext == nullptr);
	}
};

static void* _imguiAlloc(size_t size, void*) {
	return core_malloc(size);
}

static void _imguiFree(void *mem, void*) {
	core_free(mem);
}

static IMGUIApp *_imguiGetBackendUserdata() {
	return ImGui::GetCurrentContext() ? (IMGUIApp *)ImGui::GetIO().BackendPlatformUserData : nullptr;
}

static void _imguiCreateWindow(ImGuiViewport *viewport) {
	IMGUIApp *bd = _imguiGetBackendUserdata();
	_imguiViewportData *vd = IM_NEW(_imguiViewportData)();
	viewport->PlatformUserData = vd;

	ImGuiViewport *main_viewport = ImGui::GetMainViewport();
	_imguiViewportData *main_viewport_data = (_imguiViewportData *)main_viewport->PlatformUserData;

	// Share GL resources with main context
	bool use_opengl = (main_viewport_data->renderContext != NULL);
	SDL_GLContext backup_context = NULL;
	if (use_opengl) {
		backup_context = SDL_GL_GetCurrentContext();
		SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
		SDL_GL_MakeCurrent(main_viewport_data->window, main_viewport_data->renderContext);
	}

	Uint32 sdl_flags = 0;
	sdl_flags |= use_opengl ? SDL_WINDOW_OPENGL : 0;
	sdl_flags |= SDL_GetWindowFlags((SDL_Window*)bd->windowHandle()) & SDL_WINDOW_ALLOW_HIGHDPI;
	sdl_flags |= SDL_WINDOW_HIDDEN;
	sdl_flags |= (viewport->Flags & ImGuiViewportFlags_NoDecoration) ? SDL_WINDOW_BORDERLESS : 0;
	sdl_flags |= (viewport->Flags & ImGuiViewportFlags_NoDecoration) ? 0 : SDL_WINDOW_RESIZABLE;
#if !defined(_WIN32)
	// See SDL hack in ImGui_ImplSDL2_ShowWindow().
	sdl_flags |= (viewport->Flags & ImGuiViewportFlags_NoTaskBarIcon) ? SDL_WINDOW_SKIP_TASKBAR : 0;
#endif
	sdl_flags |= (viewport->Flags & ImGuiViewportFlags_TopMost) ? SDL_WINDOW_ALWAYS_ON_TOP : 0;
	vd->window = SDL_CreateWindow("No Title Yet", (int)viewport->Pos.x, (int)viewport->Pos.y, (int)viewport->Size.x,
								  (int)viewport->Size.y, sdl_flags);
	vd->windowOwned = true;
	if (use_opengl) {
		vd->renderContext = SDL_GL_CreateContext(vd->window);
		SDL_GL_SetSwapInterval(0);
	}
	if (use_opengl && backup_context)
		SDL_GL_MakeCurrent(vd->window, backup_context);

	viewport->PlatformHandle = (void *)vd->window;
#if defined(_WIN32)
	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);
	if (SDL_GetWindowWMInfo(vd->Window, &info))
		viewport->PlatformHandleRaw = info.info.win.window;
#endif
}

static void _imguiDestroyWindow(ImGuiViewport *viewport) {
	if (_imguiViewportData *vd = (_imguiViewportData *)viewport->PlatformUserData) {
		if (vd->renderContext && vd->windowOwned)
			SDL_GL_DeleteContext(vd->renderContext);
		if (vd->window && vd->windowOwned)
			SDL_DestroyWindow(vd->window);
		vd->renderContext = NULL;
		vd->window = NULL;
		IM_DELETE(vd);
	}
	viewport->PlatformUserData = viewport->PlatformHandle = NULL;
}

static void _imguiShowWindow(ImGuiViewport *viewport) {
	_imguiViewportData *vd = (_imguiViewportData *)viewport->PlatformUserData;
#if defined(_WIN32)
	HWND hwnd = (HWND)viewport->PlatformHandleRaw;

	// SDL hack: Hide icon from task bar
	// Note: SDL 2.0.6+ has a SDL_WINDOW_SKIP_TASKBAR flag which is supported under Windows but the way it create the
	// window breaks our seamless transition.
	if (viewport->Flags & ImGuiViewportFlags_NoTaskBarIcon) {
		LONG ex_style = ::GetWindowLong(hwnd, GWL_EXSTYLE);
		ex_style &= ~WS_EX_APPWINDOW;
		ex_style |= WS_EX_TOOLWINDOW;
		::SetWindowLong(hwnd, GWL_EXSTYLE, ex_style);
	}

	// SDL hack: SDL always activate/focus windows :/
	if (viewport->Flags & ImGuiViewportFlags_NoFocusOnAppearing) {
		::ShowWindow(hwnd, SW_SHOWNA);
		return;
	}
#endif

	SDL_ShowWindow(vd->window);
}

static ImVec2 _imguiGetWindowPos(ImGuiViewport *viewport) {
	_imguiViewportData *vd = (_imguiViewportData *)viewport->PlatformUserData;
	int x = 0, y = 0;
	SDL_GetWindowPosition(vd->window, &x, &y);
	return ImVec2((float)x, (float)y);
}

static void _imguiSetWindowPos(ImGuiViewport *viewport, ImVec2 pos) {
	_imguiViewportData *vd = (_imguiViewportData *)viewport->PlatformUserData;
	SDL_SetWindowPosition(vd->window, (int)pos.x, (int)pos.y);
}

static ImVec2 _imguiGetWindowSize(ImGuiViewport *viewport) {
	_imguiViewportData *vd = (_imguiViewportData *)viewport->PlatformUserData;
	int w = 0, h = 0;
	SDL_GetWindowSize(vd->window, &w, &h);
	return ImVec2((float)w, (float)h);
}

static void _imguiSetWindowSize(ImGuiViewport *viewport, ImVec2 size) {
	_imguiViewportData *vd = (_imguiViewportData *)viewport->PlatformUserData;
	SDL_SetWindowSize(vd->window, (int)size.x, (int)size.y);
}

static void _imguiSetWindowTitle(ImGuiViewport *viewport, const char *title) {
	_imguiViewportData *vd = (_imguiViewportData *)viewport->PlatformUserData;
	SDL_SetWindowTitle(vd->window, title);
}

static void _imguiSetWindowAlpha(ImGuiViewport *viewport, float alpha) {
	_imguiViewportData *vd = (_imguiViewportData *)viewport->PlatformUserData;
	SDL_SetWindowOpacity(vd->window, alpha);
}

static void _imguiSetWindowFocus(ImGuiViewport *viewport) {
	_imguiViewportData *vd = (_imguiViewportData *)viewport->PlatformUserData;
	SDL_RaiseWindow(vd->window);
}

static bool _imguiGetWindowFocus(ImGuiViewport *viewport) {
	_imguiViewportData *vd = (_imguiViewportData *)viewport->PlatformUserData;
	return (SDL_GetWindowFlags(vd->window) & SDL_WINDOW_INPUT_FOCUS) != 0;
}

static bool _imguiGetWindowMinimized(ImGuiViewport *viewport) {
	_imguiViewportData *vd = (_imguiViewportData *)viewport->PlatformUserData;
	return (SDL_GetWindowFlags(vd->window) & SDL_WINDOW_MINIMIZED) != 0;
}

static void _imguiRenderWindow(ImGuiViewport *viewport, void *) {
	_imguiViewportData *vd = (_imguiViewportData *)viewport->PlatformUserData;
	if (vd->renderContext)
		SDL_GL_MakeCurrent(vd->window, vd->renderContext);
}

static void _imguiSwapBuffers(ImGuiViewport *viewport, void *) {
	_imguiViewportData *vd = (_imguiViewportData *)viewport->PlatformUserData;
	if (vd->renderContext) {
		SDL_GL_MakeCurrent(vd->window, vd->renderContext);
		SDL_GL_SwapWindow(vd->window);
	}
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

	_camera = video::uiCamera(glm::ivec2(0), frameBufferDimension(), windowDimension());

	_vbo.addAttribute(_shader.getColorAttribute(_bufferIndex, &ImDrawVert::r, true));
	_vbo.addAttribute(_shader.getTexcoordAttribute(_bufferIndex, &ImDrawVert::u));
	_vbo.addAttribute(_shader.getPosAttribute(_bufferIndex, &ImDrawVert::x));

	// Set SDL hint to receive mouse click events on window focus, otherwise SDL doesn't emit the event.
	// Without this, when clicking to gain focus, our widgets wouldn't activate even though they showed as hovered.
	// (This is unfortunately a global SDL setting, so enabling it might have a side-effect on your application.
	// It is unlikely to make a difference, but if your app absolutely needs to ignore the initial on-focus click:
	// you can ignore SDL_MOUSEBUTTONDOWN events coming right after a SDL_WINDOWEVENT_FOCUS_GAINED)
	SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");

	ImGui::SetAllocatorFunctions(_imguiAlloc, _imguiFree);
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	const core::String iniFile = _appname + "-imgui.ini";
	_writePathIni = _filesystem->writePath(iniFile.c_str());
	io.IniFilename = _writePathIni.c_str();
	const core::String logFile = _appname + "-imgui.log";
	_writePathLog = _filesystem->writePath(logFile.c_str());
	io.LogFilename = _writePathLog.c_str();

	io.DisplaySize = _windowDimension;
	const float xScale = (float)_frameBufferDimension.x / (float)_windowDimension.x;
	const float yScale = (float)_frameBufferDimension.y / (float)_windowDimension.y;
	io.DisplayFramebufferScale = ImVec2(xScale, yScale);

	_texture = video::genTexture();
	loadFonts();

	ImGui::StyleColorsCorporateGrey();
	//ImGui::StyleColorsDark();
	ImGuiStyle &style = ImGui::GetStyle();
	style.ScaleAllSizes(_dpiFactor);

	io.KeyMap[ImGuiKey_Tab] = SDLK_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
	io.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
	io.KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
	io.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
	io.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
	io.KeyMap[ImGuiKey_Insert] = SDL_SCANCODE_INSERT;
	io.KeyMap[ImGuiKey_Delete] = SDLK_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = SDLK_BACKSPACE;
	io.KeyMap[ImGuiKey_Space] = SDL_SCANCODE_SPACE;
	io.KeyMap[ImGuiKey_Enter] = SDLK_RETURN;
	io.KeyMap[ImGuiKey_Escape] = SDLK_ESCAPE;
	io.KeyMap[ImGuiKey_KeyPadEnter] = SDL_SCANCODE_KP_ENTER;
	io.KeyMap[ImGuiKey_A] = SDLK_a;
	io.KeyMap[ImGuiKey_C] = SDLK_c;
	io.KeyMap[ImGuiKey_V] = SDLK_v;
	io.KeyMap[ImGuiKey_X] = SDLK_x;
	io.KeyMap[ImGuiKey_Y] = SDLK_y;
	io.KeyMap[ImGuiKey_Z] = SDLK_z;
	io.SetClipboardTextFn = _setClipboardText;
	io.GetClipboardTextFn = _getClipboardText;
	io.ClipboardUserData = nullptr;

	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports;
	io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;

	io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
	io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;
	io.BackendPlatformUserData = (void *)this;

	ImGuiPlatformIO &platformIO = ImGui::GetPlatformIO();
	platformIO.Platform_CreateWindow = _imguiCreateWindow;
	platformIO.Platform_DestroyWindow = _imguiDestroyWindow;
	platformIO.Platform_ShowWindow = _imguiShowWindow;
	platformIO.Platform_SetWindowPos = _imguiSetWindowPos;
	platformIO.Platform_GetWindowPos = _imguiGetWindowPos;
	platformIO.Platform_SetWindowSize = _imguiSetWindowSize;
	platformIO.Platform_GetWindowSize = _imguiGetWindowSize;
	platformIO.Platform_SetWindowFocus = _imguiSetWindowFocus;
	platformIO.Platform_GetWindowFocus = _imguiGetWindowFocus;
	platformIO.Platform_GetWindowMinimized = _imguiGetWindowMinimized;
	platformIO.Platform_SetWindowTitle = _imguiSetWindowTitle;
	platformIO.Platform_RenderWindow = _imguiRenderWindow;
	platformIO.Platform_SwapBuffers = _imguiSwapBuffers;
	platformIO.Platform_SetWindowAlpha = _imguiSetWindowAlpha;

	platformIO.Monitors.resize(0);
	int displayCount = SDL_GetNumVideoDisplays();
	for (int n = 0; n < displayCount; n++) {
		// Warning: the validity of monitor DPI information on Windows depends on the application DPI awareness
		// settings, which generally needs to be set in the manifest or at runtime.
		ImGuiPlatformMonitor monitor;
		SDL_Rect r;
		SDL_GetDisplayBounds(n, &r);
		monitor.MainPos = monitor.WorkPos = ImVec2((float)r.x, (float)r.y);
		monitor.MainSize = monitor.WorkSize = ImVec2((float)r.w, (float)r.h);
		SDL_GetDisplayUsableBounds(n, &r);
		monitor.WorkPos = ImVec2((float)r.x, (float)r.y);
		monitor.WorkSize = ImVec2((float)r.w, (float)r.h);
		float dpi = 0.0f;
		if (!SDL_GetDisplayDPI(n, &dpi, nullptr, nullptr)) {
			monitor.DpiScale = dpi / 96.0f;
		}
		platformIO.Monitors.push_back(monitor);
	}

	ImGui::SetColorEditOptions(ImGuiColorEditFlags_Float);

	_mouseCursors[ImGuiMouseCursor_Arrow] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
	_mouseCursors[ImGuiMouseCursor_TextInput] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
	_mouseCursors[ImGuiMouseCursor_ResizeAll] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
	_mouseCursors[ImGuiMouseCursor_ResizeNS] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
	_mouseCursors[ImGuiMouseCursor_ResizeEW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
	_mouseCursors[ImGuiMouseCursor_ResizeNESW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
	_mouseCursors[ImGuiMouseCursor_ResizeNWSE] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
	_mouseCursors[ImGuiMouseCursor_Hand] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
	_mouseCursors[ImGuiMouseCursor_NotAllowed] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NO);

	ImGuiViewport *mainViewport = ImGui::GetMainViewport();
	mainViewport->PlatformHandle = (void *)_window;

	_imguiViewportData *vd = IM_NEW(_imguiViewportData)();
	vd->window = _window;
	vd->windowID = SDL_GetWindowID(_window);
	vd->windowOwned = false;
	vd->renderContext = _rendererContext;
	mainViewport->PlatformUserData = vd;
	mainViewport->PlatformHandle = vd->window;

	SDL_StartTextInput();

	_console.init();

	Log::info("Set up imgui");

	return state;
}

void IMGUIApp::beforeUI() {
	ImGuiIO &io = ImGui::GetIO();

	io.DeltaTime = (float)_deltaFrameSeconds;

	ImVec2 mousePosPrev = io.MousePos;
	io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
	io.MouseHoveredViewport = 0;

	io.MouseWheel = (float)_mouseWheelY;
	io.MouseWheelH = (float)_mouseWheelX;
	_mouseWheelX = _mouseWheelY = 0;

	// Update mouse buttons
	int mouseXLocal, mouseYLocal;
	Uint32 mouseButtons = SDL_GetMouseState(&mouseXLocal, &mouseYLocal);
	// If a mouse press event came, always pass it as "mouse held this frame", so we
	// don't miss click-release events that are shorter than 1 frame.
	io.MouseDown[0] = _mousePressed[0] || (mouseButtons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
	io.MouseDown[1] = _mousePressed[1] || (mouseButtons & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
	io.MouseDown[2] = _mousePressed[2] || (mouseButtons & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
	_mousePressed[0] = _mousePressed[1] = _mousePressed[2] = false;

	SDL_Window *mouseWindow = NULL;
	if (isSingleWindowMode()) {
		mouseWindow = (SDL_GetWindowFlags(_window) & SDL_WINDOW_INPUT_FOCUS) ? _window : NULL;
	} else {
		// Obtain focused and hovered window. We forward mouse input when focused or when hovered (and no other window
		// is capturing)
		SDL_Window *focusedWindow = SDL_GetKeyboardFocus();
		SDL_Window *hoveredWindow = SDL_GetMouseFocus();
		if (hoveredWindow && (_window == hoveredWindow || ImGui::FindViewportByPlatformHandle((void *)hoveredWindow))) {
			mouseWindow = hoveredWindow;
		} else if (focusedWindow &&
				   (_window == focusedWindow || ImGui::FindViewportByPlatformHandle((void *)focusedWindow))) {
			mouseWindow = focusedWindow;
		}

		// SDL_CaptureMouse() let the OS know e.g. that our imgui drag outside the SDL window boundaries shouldn't e.g.
		// trigger other operations outside
		SDL_CaptureMouse(ImGui::IsAnyMouseDown() ? SDL_TRUE : SDL_FALSE);
	}

	if (mouseWindow == nullptr) {
		return;
	}

	// Set OS mouse position from Dear ImGui if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos
	// is enabled by user)
	if (io.WantSetMousePos) {
		if (!isSingleWindowMode() && (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0) {
			SDL_WarpMouseGlobal((int)mousePosPrev.x, (int)mousePosPrev.y);
		} else {
			SDL_WarpMouseInWindow(_window, (int)mousePosPrev.x, (int)mousePosPrev.y);
		}
	}

	if (isSingleWindowMode()) {
		io.MousePos = ImVec2((float)mouseXLocal, (float)mouseYLocal);
	} else {
		// Set Dear ImGui mouse position from OS position + get buttons. (this is the common behavior)
		int mouseXGlobal, mouseYGlobal;
		SDL_GetGlobalMouseState(&mouseXGlobal, &mouseYGlobal);
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			// Multi-viewport mode: mouse position in OS absolute coordinates (io.MousePos is (0,0) when the mouse is on
			// the upper-left of the primary monitor)
			io.MousePos = ImVec2((float)mouseXGlobal, (float)mouseYGlobal);
		} else {
			// Single-viewport mode: mouse position in client window coordinates (io.MousePos is (0,0) when the mouse is
			// on the upper-left corner of the app window) Unlike local position obtained earlier this will be valid
			// when straying out of bounds.
			int windowX, windowY;
			SDL_GetWindowPosition(mouseWindow, &windowX, &windowY);
			io.MousePos = ImVec2((float)(mouseXGlobal - windowX), (float)(mouseYGlobal - windowY));
		}
	}
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

	ImGui::NewFrame();

	const bool renderUI = _renderUI->boolVal();
	if (renderUI) {
		core_trace_scoped(IMGUIAppOnRenderUI);
		onRenderUI();

		char buf[512] = "";
		if (showFileDialog(&_showFileDialog, buf, sizeof(buf), _fileDialogMode, _fileDialogFilter)) {
			_fileDialogCallback(buf);
			_showFileDialog = false;
		}

		bool showMetrics = _showMetrics->boolVal();
		if (showMetrics) {
			ImGui::ShowMetricsWindow(&showMetrics);
			if (!showMetrics) {
				_showMetrics->setVal("false");
			}
		}
	}

	if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) {
		core::setBindingContext(core::BindingContext::UserInterface);
	} else {
		core::setBindingContext(core::BindingContext::World);
	}

	video::ScopedShader scopedShader(_shader);
	_shader.setViewprojection(_camera.projectionMatrix());
	_shader.setModel(glm::mat4(1.0f));
	_shader.setTexture(video::TextureUnit::Zero);

	video::ScopedViewPort scopedViewPort(0, 0, _frameBufferDimension.x, _frameBufferDimension.y);
	video::scissor(0, 0, _frameBufferDimension.x, _frameBufferDimension.y);

	video::enable(video::State::Blend);
	video::disable(video::State::DepthTest);
	video::enable(video::State::Scissor);
	video::disable(video::State::CullFace);
	video::blendFunc(video::BlendMode::SourceAlpha, video::BlendMode::OneMinusSourceAlpha);
	video::blendEquation(video::BlendEquation::Add);

	const math::Rect<int> rect(0, 0, _frameBufferDimension.x, _frameBufferDimension.y);
	_console.render(rect, _deltaFrameSeconds);
	ImGui::EndFrame();
	ImGui::Render();

	executeDrawCommands();

	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		SDL_Window *backup_current_window = SDL_GL_GetCurrentWindow();
		SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
	}

	video::scissor(0, 0, _frameBufferDimension.x, _frameBufferDimension.y);
	return app::AppState::Running;
}

void IMGUIApp::executeDrawCommands() {
	core_trace_scoped(ExecuteDrawCommands);

	ImGuiIO& io = ImGui::GetIO();
	ImDrawData* drawData = ImGui::GetDrawData();
	drawData->ScaleClipRects(io.DisplayFramebufferScale);
	int64_t drawCommands = 0;
	for (int n = 0; n < drawData->CmdListsCount; ++n) {
		const ImDrawList* cmdList = drawData->CmdLists[n];
		const ImDrawIdx* idxBufferOffset = nullptr;

		core_assert_always(_vbo.update(_bufferIndex, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert)));
		core_assert_always(_vbo.update(_indexBufferIndex, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx)));
		video::ScopedBuffer scopedBuf(_vbo);

		for (int i = 0; i < cmdList->CmdBuffer.Size; ++i) {
			const ImDrawCmd* cmd = &cmdList->CmdBuffer[i];
			if (cmd->UserCallback) {
				cmd->UserCallback(cmdList, cmd);
			} else {
				video::bindTexture(video::TextureUnit::Zero, video::TextureType::Texture2D, (video::Id)(intptr_t)cmd->TextureId);
				const ImVec4& cr = cmd->ClipRect;
				video::scissor((int)cr.x, (int)cr.y, (int)cr.z - (int)cr.x, (int)cr.w - (int)cr.y);
				video::drawElements<ImDrawIdx>(video::Primitive::Triangles, cmd->ElemCount, (void*)idxBufferOffset);
			}
			++drawCommands;
			idxBufferOffset += cmd->ElemCount;
		}
	}
	core_trace_plot("UIDrawCommands", drawCommands);
}

app::AppState IMGUIApp::onCleanup() {
	for (int i = 0; i < ImGuiMouseCursor_COUNT; ++i) {
		SDL_FreeCursor(_mouseCursors[i]);
	}

	if (ImGui::GetCurrentContext() != nullptr) {
		ImGui::DestroyContext();
	}
	_console.shutdown();
	_shader.shutdown();
	_vbo.shutdown();
	_indexBufferIndex = -1;
	_bufferIndex = -1;
	return Super::onCleanup();
}

void IMGUIApp::fileDialog(const std::function<void(const core::String&)>& callback, OpenFileMode mode, const core::String& filter) {
	_showFileDialog = true;
	_fileDialogCallback = callback;
	_fileDialogMode = mode;
	_fileDialogFilter = filter;
}

}
}
