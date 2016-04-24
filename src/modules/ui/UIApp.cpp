#include "UIApp.h"
#include "TurboBadger.h"

#include "io/File.h"
#include "core/Command.h"
#include "core/Common.h"
#include "ui_renderer_gl.h"

extern void register_tbbf_font_renderer();

namespace ui {

namespace {
tb::MODIFIER_KEYS mapModifier(int16_t modifier) {
	tb::MODIFIER_KEYS code = tb::TB_MODIFIER_NONE;
	if (modifier & KMOD_ALT)
		code |= tb::TB_ALT;
	if (modifier & KMOD_CTRL)
		code |= tb::TB_CTRL;
	if (modifier & KMOD_SHIFT)
		code |= tb::TB_SHIFT;
	if (modifier & KMOD_GUI)
		code |= tb::TB_SUPER;
	return code;
}

tb::SPECIAL_KEY mapSpecialKey(int32_t key) {
	switch (key) {
	case SDLK_F1:
		return tb::TB_KEY_F1;
	case SDLK_F2:
		return tb::TB_KEY_F2;
	case SDLK_F3:
		return tb::TB_KEY_F3;
	case SDLK_F4:
		return tb::TB_KEY_F4;
	case SDLK_F5:
		return tb::TB_KEY_F5;
	case SDLK_F6:
		return tb::TB_KEY_F6;
	case SDLK_F7:
		return tb::TB_KEY_F7;
	case SDLK_F8:
		return tb::TB_KEY_F8;
	case SDLK_F9:
		return tb::TB_KEY_F9;
	case SDLK_F10:
		return tb::TB_KEY_F10;
	case SDLK_F11:
		return tb::TB_KEY_F11;
	case SDLK_F12:
		return tb::TB_KEY_F12;
	case SDLK_LEFT:
		return tb::TB_KEY_LEFT;
	case SDLK_UP:
		return tb::TB_KEY_UP;
	case SDLK_RIGHT:
		return tb::TB_KEY_RIGHT;
	case SDLK_DOWN:
		return tb::TB_KEY_DOWN;
	case SDLK_PAGEUP:
		return tb::TB_KEY_PAGE_UP;
	case SDLK_PAGEDOWN:
		return tb::TB_KEY_PAGE_DOWN;
	case SDLK_HOME:
		return tb::TB_KEY_HOME;
	case SDLK_END:
		return tb::TB_KEY_END;
	case SDLK_INSERT:
		return tb::TB_KEY_INSERT;
	case SDLK_TAB:
		return tb::TB_KEY_TAB;
	case SDLK_DELETE:
		return tb::TB_KEY_DELETE;
	case SDLK_BACKSPACE:
		return tb::TB_KEY_BACKSPACE;
	case SDLK_RETURN:
	case SDLK_KP_ENTER:
		return tb::TB_KEY_ENTER;
	case SDLK_ESCAPE:
		return tb::TB_KEY_ESC;
	}
	return tb::TB_KEY_UNDEFINED;
}

int mapKey(int32_t key) {
	if (mapSpecialKey(key) == tb::TB_KEY_UNDEFINED)
		return key;
	return 0;
}

}

tb::UIRendererGL _renderer;

UIApp::UIApp(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, uint16_t traceport) :
		WindowedApp(filesystem, eventBus, traceport), _quit(false) {
}

UIApp::~UIApp() {
}

bool UIApp::loadKeyBindings() {
	const std::string& bindings = filesystem()->load("ui/keybindings.cfg");
	if (bindings.empty())
		return false;
	const KeybindingParser p(bindings);
	_bindings = p.getBindings();
	return true;
}

bool UIApp::invokeKey(int key, tb::SPECIAL_KEY special, tb::MODIFIER_KEYS mod, bool down) {
#ifdef MACOSX
	bool shortcutKey = (mod & tb::TB_SUPER) ? true : false;
#else
	bool shortcutKey = (mod & tb::TB_CTRL) ? true : false;
#endif
	if (tb::TBWidget::focused_widget && down && shortcutKey) {
		bool reverseKey = (mod & tb::TB_SHIFT) ? true : false;
		if (key >= 'a' && key <= 'z')
			key += 'A' - 'a';
		tb::TBID id;
		if (key == 'X')
			id = TBIDC("cut");
		else if (key == 'C' || special == tb::TB_KEY_INSERT)
			id = TBIDC("copy");
		else if (key == 'V' || (special == tb::TB_KEY_INSERT && reverseKey))
			id = TBIDC("paste");
		else if (key == 'A')
			id = TBIDC("selectall");
		else if (key == 'Z' || key == 'Y') {
			bool undo = key == 'Z';
			if (reverseKey)
				undo = !undo;
			id = undo ? TBIDC("undo") : TBIDC("redo");
		} else if (key == 'N')
			id = TBIDC("new");
		else if (key == 'O')
			id = TBIDC("open");
		else if (key == 'S')
			id = TBIDC("save");
		else if (key == 'W')
			id = TBIDC("close");
		else if (special == tb::TB_KEY_PAGE_UP)
			id = TBIDC("prev_doc");
		else if (special == tb::TB_KEY_PAGE_DOWN)
			id = TBIDC("next_doc");
		else
			return false;

		tb::TBWidgetEvent ev(tb::EVENT_TYPE_SHORTCUT);
		ev.modifierkeys = mod;
		ev.ref_id = id;
		return tb::TBWidget::focused_widget->InvokeEvent(ev);
	}

	if (key >= ' ' && key <= '~')
		return false;
	return _root.InvokeKey(key, special, mod, down);
}

void UIApp::onMouseWheel(int32_t x, int32_t y) {
	int posX, posY;
	SDL_GetMouseState(&posX, &posY);
	_root.InvokeWheel(posX, posY, x, -y, mapModifier(SDL_GetModState()));
}

void UIApp::onMouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) {
	if (tb::TBWidget::captured_widget != nullptr)
		return;
	_root.InvokePointerMove(x, y, mapModifier(SDL_GetModState()), false);
}

void UIApp::onMouseButtonPress(int32_t x, int32_t y, uint8_t button) {
	if (button != SDL_BUTTON_LEFT)
		return;
	static double lastTime = 0;
	static int lastX = 0;
	static int lastY = 0;
	static int counter = 1;

	const double time = tb::TBSystem::GetTimeMS();
	if (time < lastTime + 600 && lastX == x && lastY == y)
		++counter;
	else
		counter = 1;
	lastX = x;
	lastY = y;
	lastTime = time;

	_root.InvokePointerDown(x, y, counter, mapModifier(SDL_GetModState()), false);
}

void UIApp::onMouseButtonRelease(int32_t x, int32_t y, uint8_t button) {
	if (button == SDL_BUTTON_RIGHT) {
		_root.InvokePointerMove(x, y, mapModifier(SDL_GetModState()), false);
		tb::TBWidget* hover = tb::TBWidget::hovered_widget;
		if (hover != nullptr) {
			hover->ConvertFromRoot(x, y);
			tb::TBWidgetEvent ev(tb::EVENT_TYPE_CONTEXT_MENU, x, y, false, mapModifier(SDL_GetModState()));
			hover->InvokeEvent(ev);
		}
	} else {
		_root.InvokePointerUp(x, y, mapModifier(SDL_GetModState()), false);
	}
}

bool UIApp::onKeyRelease(int32_t key) {
	auto range = _bindings.equal_range(key);
	for (auto i = range.first; i != range.second; ++i) {
		const std::string& command = i->second.first;
		if (command[0] == '+' && _keys.erase(key) > 0) {
			core_assert(1 == core::Command::execute(command + " false"));
		}
	}

	return invokeKey(mapKey(key), mapSpecialKey(key), mapModifier(SDL_GetModState()), false);
}

bool UIApp::onTextInput(const std::string& text) {
	const char *c = text.c_str();
	for (;;) {
		const int key = core::string::getUTF8Next(&c);
		if (key == -1)
			return true;
		_root.InvokeKey(key, tb::TB_KEY_UNDEFINED, tb::TB_MODIFIER_NONE, true);
		_root.InvokeKey(key, tb::TB_KEY_UNDEFINED, tb::TB_MODIFIER_NONE, false);
	}
	return true;
}

bool UIApp::onKeyPress(int32_t key, int16_t modifier) {
	auto range = _bindings.equal_range(key);
	for (auto i = range.first; i != range.second; ++i) {
		const std::string& command = i->second.first;
		const int mod = i->second.second;
		if (mod == KMOD_NONE && modifier != 0 && modifier != KMOD_NUM) {
			continue;
		}
		if (mod != KMOD_NONE && !(modifier & mod)) {
			continue;
		}
		if (command[0] == '+') {
			if (core::Command::execute(command + " true") == 1) {
				_keys[key] = modifier;
			}
		} else {
			core::Command::execute(command);
		}
		return true;
	}

	return invokeKey(mapKey(key), mapSpecialKey(key), mapModifier(modifier), true);
}

core::AppState UIApp::onConstruct() {
	const core::AppState state = WindowedApp::onConstruct();
	core::Command::registerCommand("cl_ui_debug", [&] (const core::CmdArgs& args) {
#ifndef NDEBUG
		tb::ShowDebugInfoSettingsWindow(&_root);
#endif
	});

	core::Command::registerCommand("quit", [&] (const core::CmdArgs& args) {_quit = true;});

	return state;
}

core::AppState UIApp::onInit() {
	const core::AppState state = WindowedApp::onInit();
	if (!tb::tb_core_init(&_renderer)) {
		Log::error("failed to initialize the ui");
		return core::AppState::Cleanup;
	}

	tb::TBWidgetListener::AddGlobalListener(this);

	tb::g_tb_lng->Load("ui/lang/en.tb.txt");

	if (!tb::g_tb_skin->Load("ui/skin/skin.tb.txt", nullptr)) {
		Log::error("could not load the skin");
		return core::AppState::Cleanup;
	}

	if (!_renderer.init()) {
		Log::error("could not init ui renderer");
		return core::AppState::Cleanup;
	}

	tb::TBWidgetsAnimationManager::Init();
	if (!loadKeyBindings()) {
		Log::error("failed to init the keybindings");
	}

	register_tbbf_font_renderer();
	tb::g_font_manager->AddFontInfo("ui/font/font.tb.txt", "Segoe");
	tb::TBFontDescription fd;
	fd.SetID(TBIDC("Segoe"));
	fd.SetSize(tb::g_tb_skin->GetDimensionConverter()->DpToPx(14));
	tb::g_font_manager->SetDefaultFontDescription(fd);
	tb::TBFontFace *font = tb::g_font_manager->CreateFontFace(tb::g_font_manager->GetDefaultFontDescription());
	if (font == nullptr) {
		Log::error("could not create the font face");
		return core::AppState::Cleanup;
	}

	font->RenderGlyphs(" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNORSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~•·åäöÅÄÖ");
	_root.SetRect(tb::TBRect(0, 0, _width, _height));
	_root.SetSkinBg(TBIDC("background"));

	return state;
}

void UIApp::addChild(Window* window) {
	_root.AddChild(window);
}

core::AppState UIApp::onRunning() {
	if (_quit)
		return core::AppState::Cleanup;
	core::AppState state = WindowedApp::onRunning();

	for (KeyMapConstIter it = _keys.begin(); it != _keys.end(); ++it) {
		const int key = it->first;
		auto i = _bindings.find(key);
		const std::string& command = i->second.first;
		const int16_t modifier = it->second;
		if (i->second.second == modifier && command[0] == '+') {
			core_assert(1 == core::Command::execute(command + " true"));
			_keys[key] = modifier;
		}
	}

	if (state == core::AppState::Running) {
		beforeUI();

		tb::TBAnimationManager::Update();
		_root.InvokeProcessStates();
		_root.InvokeProcess();

		_renderer.BeginPaint(_width, _height);
		_root.InvokePaint(tb::TBWidget::PaintProps());

		++_frameCounter;

		double time = tb::TBSystem::GetTimeMS();
		if (time > _frameCounterResetRime + 1000) {
			fps = (int) ((_frameCounter / (time - _frameCounterResetRime)) * 1000);
			_frameCounterResetRime = time;
			_frameCounter = 0;
		}

		tb::TBStr str;
		str.SetFormatted("FPS: %d", fps);
		_root.GetFont()->DrawString(5, 5, tb::TBColor(255, 255, 255), str);

		afterUI();

		_renderer.EndPaint();
		// If animations are running, reinvalidate immediately
		if (tb::TBAnimationManager::HasAnimationsRunning())
			_root.Invalidate();
	}
	return state;
}

core::AppState UIApp::onCleanup() {
	tb::TBAnimationManager::AbortAllAnimations();
	tb::TBWidgetListener::RemoveGlobalListener(this);

	tb::TBWidgetsAnimationManager::Shutdown();
	tb::tb_core_shutdown();
	return WindowedApp::onCleanup();
}

void UIApp::afterUI() {
	// render the console
}

}

void tb::TBSystem::RescheduleTimer(double fireTime) {
#if 0
	static double setFireTime = -1;
	if (fireTime == TB_NOT_SOON) {
		setFireTime = -1;
		//glfwKillTimer();
	} else if (fireTime != setFireTime || fireTime == 0.0) {
		setFireTime = fireTime;
		double delay = fireTime - tb::TBSystem::GetTimeMS();
		unsigned int idelay = (unsigned int) std::max(delay, 0.0);
		//glfwRescheduleTimer(idelay);
	}
#endif
}
