/**
 * @file
 */

#include "UIApp.h"
#include "ui/turbobadger/TurboBadger.h"
#include "ui/turbobadger/FontUtil.h"
#include "FileDialogWindow.h"

#include "io/Filesystem.h"
#include "core/command/Command.h"
#include "core/Color.h"
#include "core/UTF8.h"
#include "core/Common.h"
#include "core/Trace.h"
#include "math/Rect.h"
#include "ui_renderer_gl.h"
#include "ui_widgets.h"
#include <stdarg.h>

namespace ui {
namespace turbobadger {

namespace {

static ImageWidgetFactory imageWidget_wf;
static ColorWidgetFactory colorWidget_wf;

static inline tb::MODIFIER_KEYS mapModifier(int32_t key, int16_t modifier) {
	tb::MODIFIER_KEYS code = tb::TB_MODIFIER_NONE;
	switch (key) {
	case SDLK_LCTRL:
	case SDLK_RCTRL:
		code |= tb::TB_CTRL;
		break;
	case SDLK_LSHIFT:
	case SDLK_RSHIFT:
		code |= tb::TB_SHIFT;
		break;
	case SDLK_LALT:
	case SDLK_RALT:
		code |= tb::TB_ALT;
		break;
	case SDLK_LGUI:
	case SDLK_RGUI:
		code |= tb::TB_SUPER;
		break;
	case SDLK_MODE:
		break;
	}

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

static tb::SPECIAL_KEY mapSpecialKey(int32_t key) {
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
	case SDLK_LSHIFT:
	case SDLK_RSHIFT:
		return tb::TB_KEY_SHIFT;
	case SDLK_LALT:
	case SDLK_RALT:
		return tb::TB_KEY_ALT;
	case SDLK_RGUI:
	case SDLK_LGUI:
		return tb::TB_KEY_GUI;
	case SDLK_LCTRL:
	case SDLK_RCTRL:
		return tb::TB_KEY_CTRL;
	case SDLK_MODE:
		return tb::TB_KEY_MODE;
	}

	return tb::TB_KEY_UNDEFINED;
}

static inline int mapKey(int32_t key) {
	switch (key) {
	case SDLK_LCTRL:
	case SDLK_LSHIFT:
	case SDLK_LALT:
	case SDLK_LGUI:
	case SDLK_RCTRL:
	case SDLK_RSHIFT:
	case SDLK_RALT:
	case SDLK_RGUI:
	case SDLK_MODE:
		break;
	default:
		if (mapSpecialKey(key) == tb::TB_KEY_UNDEFINED) {
			return key;
		}
	}
	return 0;
}

}

tb::UIRendererGL _renderer;

UIApp::UIApp(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider) {
}

UIApp::~UIApp() {
}

bool UIApp::invokeKey(int key, tb::SPECIAL_KEY special, tb::MODIFIER_KEYS mod, bool down) {
#ifdef MACOSX
	bool shortcutKey = (mod & tb::TB_SUPER) ? true : false;
#else
	bool shortcutKey = (mod & tb::TB_CTRL) ? true : false;
#endif
	Log::debug(_logId, "invoke key: %s (%i)", down ? "down" : "up", key);
	if (tb::TBWidget::focused_widget && down && shortcutKey && key != 0) {
		bool reverseKey = (mod & tb::TB_SHIFT) ? true : false;
		if (key >= 'a' && key <= 'z') {
			key += 'A' - 'a';
		}
		tb::TBID id;
		if (key == 'X') {
			id = TBIDC("cut");
		} else if (key == 'C' || special == tb::TB_KEY_INSERT) {
			id = TBIDC("copy");
		} else if (key == 'V' || (special == tb::TB_KEY_INSERT && reverseKey)) {
			id = TBIDC("paste");
		} else if (key == 'A') {
			id = TBIDC("selectall");
		} else if (key == 'Z' || key == 'Y') {
			bool undo = key == 'Z';
			if (reverseKey) {
				undo = !undo;
			}
			id = undo ? TBIDC("undo") : TBIDC("redo");
		} else if (key == 'N') {
			id = TBIDC("new");
		} else if (key == 'O') {
			id = TBIDC("open");
		} else if (key == 'S') {
			id = TBIDC("save");
		} else if (key == 'W') {
			id = TBIDC("close");
		} else if (special == tb::TB_KEY_PAGE_UP) {
			id = TBIDC("prev_doc");
		} else if (special == tb::TB_KEY_PAGE_DOWN) {
			id = TBIDC("next_doc");
		} else {
			return false;
		}

		tb::TBWidgetEvent ev(tb::EVENT_TYPE_SHORTCUT, 0, 0, tb::TB_UNKNOWN, mod);
		ev.ref_id = id;
		Log::debug(_logId, "invoke shortcut event: %i", key);
		return tb::TBWidget::focused_widget->invokeEvent(ev);
	}

	if (special == tb::TB_KEY_UNDEFINED && SDL_IsTextInputActive()) {
		return true;
	}

	if (_root->getVisibility() != tb::WIDGET_VISIBILITY_VISIBLE) {
		return false;
	}
	return _root->invokeKey(key, special, mod, down);
}

void UIApp::showStr(int x, int y, const glm::vec4& color, const char *fmt, ...) {
	static char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	SDL_vsnprintf(buf, sizeof(buf), fmt, ap);
	buf[sizeof(buf) - 1] = '\0';
	_root->getFont()->drawString(x, y, tb::TBColor(color.r * 255.0f, color.g * 255.0f, color.b * 255.0f, color.a * 255.0f), buf);
	va_end(ap);
}

void UIApp::enqueueShowStr(int x, const glm::vec4& color, const char *fmt, ...) {
	static char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	SDL_vsnprintf(buf, sizeof(buf), fmt, ap);
	buf[sizeof(buf) - 1] = '\0';
	tb::TBFontFace* font = _root->getFont();
	font->drawString(x, _lastShowTextY, tb::TBColor(color.r * 255.0f, color.g * 255.0f, color.b * 255.0f, color.a * 255.0f), buf);
	_lastShowTextY += _root->getFont()->getHeight() + 5;
	va_end(ap);
}

void UIApp::fileDialog(const std::function<void(const std::string&)>& callback, OpenFileMode mode, const std::string& filter) {
	if (isRelativeMouseMode()) {
		toggleRelativeMouseMode();
	}
	FileDialogWindow* dialog = new FileDialogWindow(this, callback, _lastDirectory);
	dialog->setMode(mode);
	if (!filter.empty()) {
		std::vector<std::string> tokens;
		core::string::splitString(filter, tokens, ";");
		const char **filters = new const char*[tokens.size() + 1];
		int n = 0;
		for (const auto& f : tokens) {
			filters[n++] = f.c_str();
		}
		filters[n] = nullptr;
		dialog->setFilter((const char**)filters);
	}
	dialog->changeDir(_lastDirectory->strVal());
	dialog->init();
}

void UIApp::onMouseWheel(int32_t x, int32_t y) {
	if (_console.onMouseWheel(x, y)) {
		return;
	}
	int posX, posY;
	SDL_GetMouseState(&posX, &posY);
	_root->invokeWheel(posX, posY, x, -y, getModifierKeys());
}

void UIApp::onMouseButtonPress(int32_t x, int32_t y, uint8_t button, uint8_t clicks) {
	if (_console.onMouseButtonPress(x, y, button)) {
		return;
	}
	const tb::MODIFIER_KEYS modKeys = getModifierKeys();

	tb::BUTTON_TYPE type = tb::BUTTON_TYPE::TB_UNKNOWN;
	if (button == SDL_BUTTON_LEFT) {
		type = tb::TB_LEFT;
	} else if (button == SDL_BUTTON_RIGHT) {
		type = tb::TB_RIGHT;
	} else if (button == SDL_BUTTON_MIDDLE) {
		type = tb::TB_MIDDLE;
	}

	_root->invokePointerDown(x, y, clicks, modKeys, type);
}

tb::MODIFIER_KEYS UIApp::getModifierKeys() const {
	return mapModifier(0, SDL_GetModState());
}

void UIApp::onMouseButtonRelease(int32_t x, int32_t y, uint8_t button) {
	if (_console.isActive()) {
		return;
	}
	const tb::MODIFIER_KEYS modKeys = getModifierKeys();
	tb::BUTTON_TYPE type = tb::BUTTON_TYPE::TB_UNKNOWN;
	if (button == SDL_BUTTON_LEFT) {
		type = tb::TB_LEFT;
	} else if (button == SDL_BUTTON_RIGHT) {
		type = tb::TB_RIGHT;
	} else if (button == SDL_BUTTON_MIDDLE) {
		type = tb::TB_MIDDLE;
	}
	if (button == SDL_BUTTON_RIGHT) {
		_root->invokePointerMove(x, y, modKeys, type);
		tb::TBWidget* hover = tb::TBWidget::hovered_widget;
		if (hover != nullptr) {
			hover->convertFromRoot(x, y);
			tb::TBWidgetEvent ev(tb::EVENT_TYPE_CONTEXT_MENU, x, y, type, modKeys);
			if (!hover->invokeEvent(ev)) {
				_root->invokePointerUp(x, y, modKeys, type);
			}
		} else {
			_root->invokePointerUp(x, y, modKeys, type);
		}
	} else {
		_root->invokePointerUp(x, y, modKeys, type);
	}
}

bool UIApp::onTextInput(const std::string& text) {
	if (_console.onTextInput(text)) {
		return true;
	}
	const char *c = text.c_str();
	for (;;) {
		const int key = core::utf8::next(&c);
		if (key == -1) {
			return true;
		}
		_root->invokeKey(key, tb::TB_KEY_UNDEFINED, tb::TB_MODIFIER_NONE, true);
		_root->invokeKey(key, tb::TB_KEY_UNDEFINED, tb::TB_MODIFIER_NONE, false);
	}
	return true;
}

bool UIApp::onKeyPress(int32_t key, int16_t modifier) {
	if (_console.onKeyPress(key, modifier)) {
		return true;
	}

	if (Super::onKeyPress(key, modifier)) {
		return true;
	}

	return invokeKey(mapKey(key), mapSpecialKey(key), mapModifier(key, modifier), true);
}

bool UIApp::onKeyRelease(int32_t key, int16_t modifier) {
	if (_console.isActive()) {
		return true;
	}
	Super::onKeyRelease(key, modifier);
	tb::MODIFIER_KEYS mod = mapModifier(0, modifier);
	mod |= mapModifier(key, 0);
	if (key == SDLK_MENU && tb::TBWidget::focused_widget) {
		tb::TBWidgetEvent ev(tb::EVENT_TYPE_CONTEXT_MENU, 0, 0, tb::TB_UNKNOWN, mod);
		if (tb::TBWidget::focused_widget->invokeEvent(ev)) {
			return true;
		}
	}
	return invokeKey(mapKey(key), mapSpecialKey(key), mod, false);
}

void UIApp::onWindowResize() {
	Super::onWindowResize();
	_renderer.onWindowResize(dimension());
	_root->setRect(tb::TBRect(0, 0, dimension().x, dimension().y));
}

core::AppState UIApp::onConstruct() {
	const core::AppState state = Super::onConstruct();
	core::Command::registerCommand("cl_ui_debug", [&] (const core::CmdArgs& args) {
#ifdef DEBUG
		tb::ShowDebugInfoSettingsWindow(_root);
#endif
	}).setHelp("Show ui debug information - only available in debug builds");

	_renderUI = core::Var::get(cfg::ClientRenderUI, "true");
	_lastDirectory = core::Var::get("cl_ui_lastdirectory", core::App::getInstance()->filesystem()->homePath().c_str());

	_console.construct();

	return state;
}

void UIApp::onWidgetFocusChanged(tb::TBWidget *widget, bool focused) {
	if (focused && widget->isOfType<tb::TBEditField>()) {
		SDL_StartTextInput();
	} else {
		SDL_StopTextInput();
	}
}

void UIApp::afterRootWidget() {
	const math::Rect<int> rect(0, 0, _dimension.x, _dimension.y);
	_console.render(rect, _deltaFrameMillis);
}

core::AppState UIApp::onInit() {
	const core::AppState state = Super::onInit();
	video::checkError();
	if (state != core::AppState::Running) {
		return state;
	}
	if (!tb::tb_core_init(&_renderer)) {
		Log::error(_logId, "failed to initialize the ui");
		return core::AppState::InitFailure;
	}

	tb::TBWidgetListener::addGlobalListener(this);
	_uiInitialized = true;

	if (!tb::g_tb_lng->load("ui/lang/en.tb.txt")) {
		Log::warn(_logId, "could not load the translation ui/lang/en.tb.txt");
	}

	if (_applicationSkin.empty()) {
		const std::string skin = "ui/skin/" + _appname + "-skin.tb.txt";
		if (filesystem()->exists(skin)) {
			_applicationSkin = skin;
		}
	}

	tb::TBWidgetsAnimationManager::init();

	if (!tb::g_tb_skin->load("ui/skin/skin.tb.txt", _applicationSkin.empty() ? nullptr : _applicationSkin.c_str())) {
		Log::error(_logId, "could not load the skin at ui/skin/skin.tb.txt and/or %s",
				_applicationSkin.empty() ? "none" : _applicationSkin.c_str());
		return core::AppState::InitFailure;
	}

	if (!_renderer.init(dimension())) {
		Log::error(_logId, "could not init ui renderer");
		return core::AppState::InitFailure;
	}

	initFonts();
	tb::TBFontFace *font = getFont(14, true);
	if (font == nullptr) {
		Log::error(_logId, "could not create the font face");
		return core::AppState::InitFailure;
	}

	_root = new tb::TBWidget();
	_root->setRect(tb::TBRect(0, 0, _dimension.x, _dimension.y));
	_root->setSkinBg(TBIDC("background"));
	_root->setGravity(tb::WIDGET_GRAVITY_ALL);

	_console.init();

	return state;
}

void UIApp::addChild(Window* window) {
	_root->addChild(window);
}

tb::TBWidget* UIApp::getWidget(const char *name) {
	return _root->getWidgetByID(tb::TBID(name));
}

tb::TBWidget* UIApp::getWidgetAt(int x, int y, bool includeChildren) {
	return _root->getWidgetAt(x, y, includeChildren);
}

void UIApp::doLayout() {
	_root->invalidateLayout(tb::TBWidget::INVALIDATE_LAYOUT_RECURSIVE);
}

core::AppState UIApp::onRunning() {
	core::AppState state = Super::onRunning();
	_console.update(_deltaFrameMillis);

	_lastShowTextY = 5;

	if (!_console.isActive()) {
		_root->invokePointerMove(_mousePos.x, _mousePos.y, getModifierKeys(), tb::TB_UNKNOWN);
	}

	const bool running = state == core::AppState::Running;
	if (running) {
		{
			core_trace_scoped(UIAppBeforeUI);
			beforeUI();
		}

		const bool renderUI = _renderUI->boolVal();
		if (renderUI) {
			core_trace_scoped(UIAppUpdateUI);
			tb::TBAnimationManager::update();
			_root->invokeProcessStates();
			_root->invokeProcess();

			_renderer.beginPaint(_dimension.x, _dimension.y);
			_root->invokePaint(tb::TBWidget::PaintProps());

			enqueueShowStr(5, core::Color::White, "FPS: %d", _fps);
		}
		{
			core_trace_scoped(UIAppAfterUI);
			afterRootWidget();
		}
		if (renderUI) {
			core_trace_scoped(UIAppEndPaint);
			_renderer.endPaint();
			// If animations are running, reinvalidate immediately
			if (tb::TBAnimationManager::hasAnimationsRunning()) {
				_root->invalidate();
			}
		}
		double next_fire_time = tb::TBMessageHandler::getNextMessageFireTime();
		double now = tb::TBSystem::getTimeMS();
		if (next_fire_time == TB_NOT_SOON || (next_fire_time - now) <= 1.0) {
			tb::TBMessageHandler::processMessages();
		}
	}
	return state;
}

core::AppState UIApp::onCleanup() {
	tb::TBAnimationManager::abortAllAnimations();
	if (_uiInitialized) {
		tb::TBWidgetListener::removeGlobalListener(this);
		tb::TBWidgetsAnimationManager::shutdown();
		_uiInitialized = false;
	}

	tb::tb_core_shutdown();

	Log::debug("shutdown ui widgets");
	_root->die();
	_root = nullptr;

	_console.shutdown();

	_renderer.shutdown();

	return Super::onCleanup();
}

}
}
