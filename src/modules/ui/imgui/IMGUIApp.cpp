/**
 * @file
 */

#include "IMGUIApp.h"

#include "io/Filesystem.h"
#include "core/command/Command.h"
#include "core/Color.h"
#include "core/UTF8.h"
#include "core/Common.h"
#include "core/Array.h"
#include "math/Rect.h"
#include "video/Renderer.h"
#include "video/ScopedViewPort.h"
#include "IMGUI.h"

namespace ui {
namespace imgui {

thread_local int IMGUIApp::_currentFrameCounter = 0;

IMGUIApp::IMGUIApp(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider), _camera(video::CameraType::FirstPerson, video::CameraMode::Orthogonal) {
}

IMGUIApp::~IMGUIApp() {
}

void IMGUIApp::traceBeginFrame(const char *threadName) {
	Super::traceBeginFrame(threadName);
	std::lock_guard<std::mutex> lock(_traceMutex);
	const std::thread::id id = std::this_thread::get_id();
	auto i = _traceMeasures.find(id);
	if (i == _traceMeasures.end()) {
		i = _traceMeasures.emplace(std::make_pair(id, Frames())).first;
	}
}

void IMGUIApp::traceBegin(const char *threadName, const char* name) {
	Super::traceBegin(threadName, name);
	std::lock_guard<std::mutex> lock(_traceMutex);
	const std::thread::id id = std::this_thread::get_id();
	auto measureIter = _traceMeasures.find(id);
	if (measureIter == _traceMeasures.end()) {
		measureIter = _traceMeasures.insert(std::make_pair(id, Frames())).first;
	}
	FrameData& frame = measureIter->second[_currentFrameCounter % _maxMeasureSize];
	auto frameIter = frame.find(name);
	if (frameIter == frame.end()) {
		frame.emplace(std::make_pair(name, TraceData{1, true, core::TimeProvider::systemNanos(), 0.0}));
	} else {
		frameIter->second.value += core::TimeProvider::systemNanos();
		frameIter->second.cnt++;
		frameIter->second.begin = true;
	}
}

void IMGUIApp::traceEnd(const char *threadName) {
	Super::traceEnd(threadName);
	std::lock_guard<std::mutex> lock(_traceMutex);
	const std::thread::id id = std::this_thread::get_id();
	auto measureIter = _traceMeasures.find(id);
	core_assert(measureIter != _traceMeasures.end());
	FrameData& frame = measureIter->second[_currentFrameCounter % _maxMeasureSize];
	core_assert(!frame.empty());
	TraceData& value = frame.rbegin()->second;
	value.delta = core::TimeProvider::systemNanos() - (value.value / double(value.cnt));
	value.begin = false;
}

void IMGUIApp::traceEndFrame(const char *threadName) {
	Super::traceEndFrame(threadName);
	++_currentFrameCounter;
}

void IMGUIApp::onMouseWheel(int32_t x, int32_t y) {
	if (_console.onMouseWheel(x, y)) {
		return;
	}
	if (y > 0) {
		_mouseWheel = 1;
	} else if (y < 0) {
		_mouseWheel = -1;
	}
	Super::onMouseWheel(x, y);
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

bool IMGUIApp::onTextInput(const std::string& text) {
	if (_console.onTextInput(text)) {
		return true;
	}
	ImGuiIO& io = ImGui::GetIO();
	io.AddInputCharactersUTF8(text.c_str());
	return true;
}

bool IMGUIApp::onKeyPress(int32_t key, int16_t modifier) {
	if (_console.onKeyPress(key, modifier) && _console.isActive()) {
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
	return Super::onKeyPress(key, modifier);
}

bool IMGUIApp::onKeyRelease(int32_t key) {
	if (_console.isActive()) {
		return true;
	}
	ImGuiIO& io = ImGui::GetIO();
	key &= ~SDLK_SCANCODE_MASK;
	core_assert(key >= 0 && key < lengthof(io.KeysDown));
	io.KeysDown[key] = false;
	const int16_t modifier = SDL_GetModState();
	io.KeyShift = (modifier & KMOD_SHIFT) != 0;
	io.KeyCtrl  = (modifier & KMOD_CTRL) != 0;
	io.KeyAlt   = (modifier & KMOD_ALT) != 0;
	io.KeySuper = (modifier & KMOD_GUI) != 0;
	return Super::onKeyRelease(key);
}

void IMGUIApp::onWindowResize() {
	Super::onWindowResize();
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2((float)_dimension.x, (float)_dimension.y);

	_camera.init(glm::ivec2(0), dimension());
	_camera.update(0L);
	video::ScopedShader scoped(_shader);
	_shader.setProjection(_camera.projectionMatrix());
}

core::AppState IMGUIApp::onConstruct() {
	const core::AppState state = Super::onConstruct();
	_console.onConstruct();
	core::Command::registerCommand("cl_rendertracing", [&] (const core::CmdArgs& args) { _renderTracing ^= true; }).setHelp("Toggle the trace data rendering");
	return state;
}

static const char* _getClipboardText(void*) {
	const char* text = SDL_GetClipboardText();
	if (!text) {
		return nullptr;
	}
	const int len = strlen(text);
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
	strcpy(&clipboardBuffer[0], text);
	SDL_free((void*) text);
	return (const char*) &clipboardBuffer[0];
}

static void _setClipboardText(void*, const char* text) {
	SDL_SetClipboardText(text);
}

core::AppState IMGUIApp::onInit() {
	const core::AppState state = Super::onInit();
	video::checkError();
	if (state != core::AppState::Running) {
		return state;
	}

	_renderUI = core::Var::get(cfg::ClientRenderUI, "true");

	if (!_shader.setup()) {
		Log::error("Could not load the ui shader");
		return core::AppState::InitFailure;
	}

	_bufferIndex = _vbo.create();
	if (_bufferIndex < 0) {
		Log::error("Failed to create ui vertex buffer");
		return core::AppState::InitFailure;
	}
	_vbo.setMode(_bufferIndex, video::VertexBufferMode::Stream);
	_indexBufferIndex = _vbo.create(nullptr, 0, video::VertexBufferType::IndexBuffer);
	if (_indexBufferIndex < 0) {
		Log::error("Failed to create ui index buffer");
		return core::AppState::InitFailure;
	}

	_camera.setNearPlane(-1.0f);
	_camera.setFarPlane(1.0f);
	_camera.init(glm::ivec2(0), _dimension);
	_camera.update(0L);

	video::Attribute attributeColor;
	attributeColor.bufferIndex = _bufferIndex;
	attributeColor.index = _shader.getLocationColor();
	attributeColor.size = _shader.getComponentsColor();
	attributeColor.stride = sizeof(ImDrawVert);
	attributeColor.offset = offsetof(ImDrawVert, col);
	attributeColor.type = video::DataType::UnsignedByte;
	attributeColor.normalized = true;
	_vbo.addAttribute(attributeColor);

	video::Attribute attributeTexCoord;
	attributeTexCoord.bufferIndex = _bufferIndex;
	attributeTexCoord.index = _shader.getLocationTexcoord();
	attributeTexCoord.size = _shader.getComponentsTexcoord();
	attributeTexCoord.stride = sizeof(ImDrawVert);
	attributeTexCoord.offset = offsetof(ImDrawVert, uv);
	_vbo.addAttribute(attributeTexCoord);

	video::Attribute attributePosition;
	attributePosition.bufferIndex = _bufferIndex;
	attributePosition.index = _shader.getLocationPos();
	attributePosition.size = _shader.getComponentsPos();
	attributePosition.stride = sizeof(ImDrawVert);
	attributePosition.offset = offsetof(ImDrawVert, pos);
	_vbo.addAttribute(attributePosition);

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2((float)_dimension.x, (float)_dimension.y);
	io.Fonts->AddFontDefault();

	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	_texture = video::genTexture();
	video::bindTexture(video::TextureUnit::Upload, video::TextureType::Texture2D, _texture);
	video::setupTexture(video::TextureType::Texture2D, video::TextureWrap::None);
	video::uploadTexture(video::TextureType::Texture2D, video::TextureFormat::RGBA, width, height, pixels, 0);
	io.Fonts->TexID = (void *) (intptr_t) _texture;

	io.KeyMap[ImGuiKey_Tab] = SDLK_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
	io.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
	io.KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
	io.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
	io.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
	io.KeyMap[ImGuiKey_Delete] = SDLK_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = SDLK_BACKSPACE;
	io.KeyMap[ImGuiKey_Enter] = SDLK_RETURN;
	io.KeyMap[ImGuiKey_Escape] = SDLK_ESCAPE;
	io.KeyMap[ImGuiKey_A] = SDLK_a;
	io.KeyMap[ImGuiKey_C] = SDLK_c;
	io.KeyMap[ImGuiKey_V] = SDLK_v;
	io.KeyMap[ImGuiKey_X] = SDLK_x;
	io.KeyMap[ImGuiKey_Y] = SDLK_y;
	io.KeyMap[ImGuiKey_Z] = SDLK_z;
	io.RenderDrawListsFn = nullptr;
	io.SetClipboardTextFn = _setClipboardText;
	io.GetClipboardTextFn = _getClipboardText;
	io.ClipboardUserData = nullptr;

#ifdef _WIN32
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(_window, &wmInfo);
	io.ImeWindowHandle = wmInfo.info.win.window;
#endif
	SDL_StartTextInput();

	_console.init();

	Log::info("Set up imgui");

	return state;
}

core::AppState IMGUIApp::onRunning() {
	core::AppState state = Super::onRunning();

	if (state != core::AppState::Running) {
		return state;
	}

	ImGuiIO& io = ImGui::GetIO();
	io.MousePos = ImVec2(_mousePos.x, _mousePos.y);

	core_assert(_bufferIndex > -1);
	core_assert(_indexBufferIndex > -1);

	{
		core_trace_scoped(IMGUIAppBeforeUI);
		beforeUI();
	}

	const bool renderUI = _renderUI->boolVal();

	const int renderTargetW = (int) (io.DisplaySize.x * io.DisplayFramebufferScale.x);
	const int renderTargetH = (int) (io.DisplaySize.y * io.DisplayFramebufferScale.y);
	// Avoid rendering when minimized, scale coordinates for
	// retina displays (screen coordinates != framebuffer coordinates)
	if (renderTargetW == 0 || renderTargetH == 0) {
		return core::AppState::Running;
	}

	io.DeltaTime = float(_deltaFrame) / 1000.0f;
	const uint32_t mouseMask = SDL_GetMouseState(nullptr, nullptr);
	// If a mouse press event came, always pass it as "mouse held this frame",
	// so we don't miss click-release events that are shorter than 1 frame.
	io.MouseDown[0] = _mousePressed[0] || (mouseMask & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
	io.MouseDown[1] = _mousePressed[1] || (mouseMask & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
	io.MouseDown[2] = _mousePressed[2] || (mouseMask & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
	_mousePressed[0] = _mousePressed[1] = _mousePressed[2] = false;

	io.MouseWheel = (float)_mouseWheel;
	_mouseWheel = 0;

	ImGui::NewFrame();

	showCursor(io.MouseDrawCursor ? false : true);

	core_trace_scoped(IMGUIAppUpdateUI);
	video::ScopedShader scopedShader(_shader);
	_shader.setProjection(_camera.projectionMatrix());
	_shader.setTexture(video::TextureUnit::Zero);

	video::ScopedViewPort scopedViewPort(0, 0, renderTargetW, renderTargetH);
	video::scissor(0, 0, renderTargetW, renderTargetH);

	video::enable(video::State::Blend);
	video::disable(video::State::DepthTest);
	video::enable(video::State::Scissor);
	video::disable(video::State::CullFace);
	video::blendFunc(video::BlendMode::SourceAlpha, video::BlendMode::OneMinusSourceAlpha);
	video::blendEquation(video::BlendEquation::Add);

	if (renderUI) {
		core_trace_scoped(IMGUIAppOnRenderUI);
		onRenderUI();
	}

	const int index = _currentFrameCounter % _maxMeasureSize;
	_frameMillis[index] = _deltaFrame;

	renderTracing();

	const math::Rect<int> rect(0, 0, _dimension.x, _dimension.y);
	_console.render(rect, _deltaFrame);
	ImGui::Render();

	ImDrawData* drawData = ImGui::GetDrawData();
	drawData->ScaleClipRects(io.DisplayFramebufferScale);

	for (int n = 0; n < drawData->CmdListsCount; ++n) {
		const ImDrawList* cmdList = drawData->CmdLists[n];
		const ImDrawIdx* idxBufferOffset = nullptr;

		core_assert_always(_vbo.update(_bufferIndex, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert)));
		core_assert_always(_vbo.update(_indexBufferIndex, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx)));
		core_assert_always(_vbo.bind());

		for (int i = 0; i < cmdList->CmdBuffer.Size; ++i) {
			const ImDrawCmd* cmd = &cmdList->CmdBuffer[i];
			if (cmd->UserCallback) {
				cmd->UserCallback(cmdList, cmd);
			} else {
				video::bindTexture(video::TextureUnit::Zero, video::TextureType::Texture2D, (video::Id)(intptr_t)cmd->TextureId);
				const ImVec4& cr = cmd->ClipRect;
				video::scissor(cr.x, cr.y, cr.z - cr.x, cr.w - cr.y);
				video::drawElements<ImDrawIdx>(video::Primitive::Triangles, cmd->ElemCount, (void*)idxBufferOffset);
			}
			idxBufferOffset += cmd->ElemCount;
		}
		_vbo.unbind();
	}

	video::scissor(0, 0, renderTargetW, renderTargetH);
	return core::AppState::Running;
}

void IMGUIApp::renderTracing() {
	if (!_renderTracing) {
		return;
	}
	if (!ImGui::Begin("Tracing", &_renderTracing, ImGuiWindowFlags_AlwaysAutoResize)) {
		return;
	}
	std::array<float, _maxMeasureSize> frameMillis;
	float max = 0.0f;
	float avg = 0.0f;
	for (int i = 0; i < _maxMeasureSize; ++i) {
		const int index = (_currentFrameCounter + i) % _maxMeasureSize;
		const float value = _frameMillis[index];
		frameMillis[i] = value / 1000.0f;
		max = glm::max(max, frameMillis[i]);
		avg += frameMillis[i];
	}
	std::array<float, _maxMeasureSize> frameMillisQuantile = frameMillis;
	std::sort(frameMillisQuantile.begin(), frameMillisQuantile.end());
	const int quantileIndex = int(0.95f * (float(_maxMeasureSize + 1)));
	const float twenthyQuantile = frameMillisQuantile[quantileIndex];
	avg /= (float)_maxMeasureSize;
	const std::string& maxStr = core::string::format("max: %fms, avg: %fms, quantile: %f", max, avg, twenthyQuantile);
	ImGui::PlotLines("Frame", &frameMillis[0], frameMillis.size(), 0, maxStr.c_str() , 0.0f, max, ImVec2(500, 100));
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	ImVec2 pos = ImGui::GetCursorPos();
	ImVec2 posEnd = pos;
	posEnd.x += 30.0f;
	posEnd.y += 3.0f;
	const ImU32 colBase = ImGui::GetColorU32(ImGuiCol_PlotHistogram);
	// TODO
	if (1) {
		std::lock_guard<std::mutex> lock(_traceMutex);
		for (auto i = _traceMeasures.begin(); i != _traceMeasures.end(); ++i) {
			const Frames& frames = i->second;
			if (frames.empty()) {
				continue;
			}
			const FrameData& frameData = frames.back();
			for (auto fi = frameData.begin(); fi != frameData.end(); ++fi) {
				const char* key = fi->first;
				const TraceData& data = fi->second;
				if (data.begin) {
					// incomplete data
					continue;
				}
				posEnd.x = pos.x + data.delta;
				const ImVec2 labelSize = ImGui::CalcTextSize(key);
				const ImVec2 size(glm::max(labelSize.x, posEnd.x - pos.x), glm::max(labelSize.y, posEnd.y - pos.y));
				ImGui::ItemSize(size, 0.0f);
				Log::info("key: %s, value: %f (%f:%f) - (%f:%f)", key, (float)data.delta, pos.x, pos.y, posEnd.x, posEnd.y);
				if (!ImGui::ItemAdd(ImRect(pos, posEnd), 0u, nullptr)) {
					continue;
				}
				window->DrawList->AddRectFilled(pos, posEnd, colBase);
				window->DrawList->AddText(pos, IM_COL32_WHITE, key, nullptr);
				pos.y += 5.0f;
				posEnd.y = pos.y + 3.0f;
			}
		}
	}
	ImGui::End();
}

core::AppState IMGUIApp::onCleanup() {
	ImGui::DestroyContext();
	_console.shutdown();
	_shader.shutdown();
	_vbo.shutdown();
	_indexBufferIndex = -1;
	_bufferIndex = -1;
	return Super::onCleanup();
}

}
}
