#include "Console.h"
#include "core/Command.h"
#include "core/Common.h"
#include "core/Color.h"
#include "core/Tokenizer.h"

namespace ui {

Console::Console() {
	SDL_LogGetOutputFunction(&_logFunction, nullptr);
	SDL_LogSetOutputFunction(logConsole, this);

	_autoEnable = core::Var::get("ui_autoconsole", "false");
}

bool Console::init() {
	tb::TBFontDescription fd;
	fd.SetID(TBIDC("Segoe"));
	fd.SetSize(tb::g_tb_skin->GetDimensionConverter()->DpToPx(20));
	tb::TBFontManager *fontMgr = tb::g_font_manager;

	if (fontMgr->HasFontFace(fd)) {
		_font = fontMgr->GetFontFace(fd);
	} else {
		_font = fontMgr->CreateFontFace(fd);
	}
	core_assert_msg(_font != nullptr, "Could not find the default font - make sure the ui is already configured");
	_font->RenderGlyphs(" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNORSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~•·");

	core::Command::registerCommand("toggleconsole", [&] (const core::CmdArgs& args) { toggle(); });

	return true;
}

bool Console::onKeyPress(int32_t key, int16_t modifier) {
	if (!_consoleActive) {
		return false;
	}

	if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
		_messages.push_back(_commandLine);
		if (core::Command::execute(_commandLine) <= 0) {
			core::Tokenizer t(_commandLine);
			while (t.hasNext()) {
				const std::string& var = t.next();
				const core::VarPtr& varPtr = core::Var::get(var, "", core::CV_NOTCREATEEMPTY);
				if (!varPtr) {
					Log::info("unknown command and cvar");
					break;
				}
				if (!t.hasNext()) {
					Log::info("%s = %s", varPtr->name().c_str(), varPtr->strVal().c_str());
					break;
				}
				const std::string& value = t.next();
				varPtr->setVal(value);
				if (!t.hasNext()) {
					break;
				}
				if (t.next() != ";") {
					break;
				}
			}
		}
		_history.push_back(_commandLine);
		_historyPos = _history.size();
		_commandLine = "";
	} else if (key == SDLK_TAB) {
		// TODO: auto complete command and/or cvar
	} else if (key == SDLK_UP) {
		if (_historyPos > 1) {
			_commandLine = _history[_historyPos - 1];
			--_historyPos;
		} else if (_historyPos == 1) {
			_commandLine = _history[0];
		}
	} else if (key == SDLK_DOWN) {
		if (_historyPos < _history.size()) {
			++_historyPos;
			_commandLine = _history[_historyPos - 1];
		} else {
			_commandLine = "";
		}
	}

	return true;
}

bool Console::onMouseWheel(int32_t x, int32_t y) {
	if (!_consoleActive) {
		return false;
	}

	// TODO: scrolling

	return true;
}

bool Console::onTextInput(const std::string& text) {
	if (!_consoleActive) {
		return false;
	}

	_commandLine.append(text);

	return true;
}

void Console::logConsole(void *userdata, int category, SDL_LogPriority priority, const char *message) {
	Console* console = (Console*)userdata;
	console->_logFunction(userdata, category, priority, message);
	// TODO: add color code for errors
	console->_messages.push_back(message);
	if (priority < SDL_LOG_PRIORITY_ERROR) {
		return;
	}
	if (!console->_consoleActive && console->_autoEnable->boolVal()) {
		console->toggle();
	}
}

bool Console::toggle() {
	_consoleActive ^= true;
	Log::info("toggle game console");
	if (_consoleActive) {
		SDL_StartTextInput();
	} else {
		SDL_StopTextInput();
	}
	return _consoleActive;
}

void Console::render(const tb::TBRect &rect) {
	static const tb::TBColor consoleFontColor(255, 255, 255);
	static const tb::TBColor consoleBgColor(127, 127, 127, 150);
	if (!_consoleActive) {
		return;
	}

	tb::g_renderer->DrawRectFill(rect, consoleBgColor);

	const int lineHeight = _font->GetFontDescription().GetSize();
	int maxY = _messages.size() * lineHeight;
	const int startY = std::min(rect.y + rect.h, maxY) - lineHeight;
	int y = startY - lineHeight;
	for (MessagesIter i = _messages.rbegin(); i != _messages.rend(); ++i) {
		tb::TBStr str(i->c_str());
		if (y - lineHeight < 0) {
			break;
		}
		_font->DrawString(5, y, consoleFontColor, str);
		y -= lineHeight;
	}

	const tb::TBStr cmdLine(_commandLine.c_str());
	_font->DrawString(5, startY, consoleFontColor, "] ");
	_font->DrawString(15, startY, consoleFontColor, cmdLine);
}

}
