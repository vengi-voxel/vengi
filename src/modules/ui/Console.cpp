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

	switch (key) {
	case SDLK_ESCAPE:
		toggle();
		break;
	case SDLK_RETURN:
	case SDLK_KP_ENTER:
		executeCommandLine();
		break;
	case SDLK_BACKSPACE:
		cursorDelete();
		break;
	case SDLK_DELETE:
		cursorDelete(false);
		break;
	case SDLK_INSERT:
		_overwrite ^= true;
		break;
	case SDLK_LEFT:
		cursorLeft();
		break;
	case SDLK_RIGHT:
		cursorRight();
		break;
	case SDLK_UP:
		cursorUp();
		break;
	case SDLK_DOWN:
		cursorDown();
		break;
	case SDLK_PAGEUP:
		scrollUp();
		break;
	case SDLK_PAGEDOWN:
		scrollDown();
		break;
	case SDLK_TAB:
		autoComplete();
		break;
	}

	return true;
}

void Console::executeCommandLine() {
	_messages.push_back(_commandLine);
	if (_commandLine.empty()) {
		return;
	}
	_history.push_back(_commandLine);
	_historyPos = _history.size();

	std::vector<std::string> commands;
	core::string::splitString(_commandLine, commands, ";");

	for (const std::string& command : commands) {
		std::vector<std::string> tokens;
		core::string::splitString(command, tokens);
		std::string cmd = core::string::eraseAllSpaces(tokens[0]);
		tokens.erase(tokens.begin());
		if (!core::Command::execute(cmd)) {
			const core::VarPtr& c = core::Var::get(cmd, "", core::CV_NOTCREATEEMPTY);
			if (c) {
				if (tokens.empty()) {
					if (c->strVal().empty())
						Log::info("%s: no value set", cmd.c_str());
					else
						Log::info("%s: %s", cmd.c_str(), c->strVal().c_str());
				} else {
					c->setVal(core::string::eraseAllSpaces(tokens[0]));
				}
			} else {
				Log::info("unknown config variable %s", cmd.c_str());
			}
		} else {
			core::Command::execute(cmd, tokens);
		}
	}
	_commandLine.clear();
	_cursorPos = 0;
}

bool Console::onMouseWheel(int32_t x, int32_t y) {
	if (!_consoleActive) {
		return false;
	}

	if (y > 0) {
		scrollUp();
	} else {
		scrollDown();
	}

	return true;
}

bool Console::onTextInput(const std::string& text) {
	if (!_consoleActive) {
		return false;
	}

	if (_overwrite && _cursorPos < _commandLine.size()) {
		cursorDelete();
	}
	_commandLine.insert(_commandLine.begin() + _cursorPos, text.begin(), text.end());
	_cursorPos += text.size();

	return true;
}

void Console::cursorLeft() {
	if (_cursorPos > 0) {
		_cursorPos--;
	}
}

void Console::cursorUp() {
	if (_historyPos <= 0) {
		return;
	}

	--_historyPos;
	_commandLine = _history[_historyPos];
	_cursorPos = _commandLine.size();
}

void Console::cursorDown() {
	++_historyPos;

	const size_t entries = _history.size();
	if (_historyPos >= entries) {
		_historyPos = entries;
		_commandLine = "";
		_cursorPos = 0;
		return;
	}
	_commandLine = _history[_historyPos];
	_cursorPos = _commandLine.size();
}

void Console::scrollUp() {
	const int scrollableLines = _messages.size() - _maxLines;
	if (scrollableLines <= 0) {
		return;
	}
	if (_scrollPos < scrollableLines) {
		++_scrollPos;
	}
}

void Console::scrollDown() {
	if (_scrollPos <= 0) {
		return;
	}
	--_scrollPos;
}

void Console::cursorRight() {
	const int size = _commandLine.size();
	if (_cursorPos < size) {
		_cursorPos++;
	}
}

void Console::autoComplete() {
	if (_commandLine.empty()) {
		return;
	}
	std::vector<std::string> matches;
	std::vector<std::string> strings;
	std::string match = "";
	core::string::splitString(_commandLine, strings);
	core_assert(!strings.empty());
	if (strings.size() == 1) {
		core::Command::visit([&] (const core::Command& cmd) {
			const std::string tmp(strings[0] + "*");
			if (core::string::matches(tmp, cmd.name())) {
				matches.push_back(cmd.name());
			}
		});
	}
	if (matches.empty()) {
		core::Var::visit([&] (const core::VarPtr& var) {
			const std::string tmp(strings[0] + "*");
			if (core::string::matches(tmp, var->name())) {
				matches.push_back(var->name());
			}
		});
	}

	if (matches.size() == 1) {
		if (strings.size() == 1) {
			_commandLine = matches.front();
		} else {
			const int start = _commandLine.size() - match.size();
			_commandLine.replace(start, _commandLine.size() - 1, matches.front());
		}
		_cursorPos = _commandLine.size();
	} else if (matches.empty()) {
		Log::info("no matches found for %s", _commandLine.c_str());
	} else {
		for (std::vector<std::string>::const_iterator i = matches.begin(); i != matches.end(); ++i) {
			Log::info("%s", (*i).c_str());
		}
	}
}

void Console::cursorDelete(bool moveCursor) {
	if (_commandLine.empty()) {
		return;
	}
	const int size = _commandLine.size();
	if (_cursorPos > size - 1) {
		cursorLeft();
	}
	_commandLine.erase(_cursorPos, 1);
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
	if (_consoleActive) {
		SDL_StartTextInput();
	} else {
		SDL_StopTextInput();
	}
	return _consoleActive;
}

void Console::render(const tb::TBRect &rect) {
	_frame++;
	if ((_frame % 10) == 0) {
		_cursorBlink ^= true;
	}

	static const tb::TBColor consoleFontColor(255, 255, 255);
	static const tb::TBColor consoleBgColor(127, 127, 127, 150);
	if (!_consoleActive) {
		return;
	}

	tb::g_renderer->DrawRectFill(rect, consoleBgColor);

	const int lineHeight = _font->GetFontDescription().GetSize();
	_maxLines = rect.h / lineHeight;
	int maxY = _messages.size() * lineHeight;
	const int startY = std::min(rect.y + rect.h, maxY) - lineHeight;
	int y = startY - lineHeight;
	MessagesIter i = _messages.rbegin();
	std::advance(i, _scrollPos);
	for (; i != _messages.rend(); ++i) {
		tb::TBStr str(i->c_str());
		if (y - lineHeight < 0) {
			break;
		}
		_font->DrawString(5, y, consoleFontColor, str);
		y -= lineHeight;
	}

	_font->DrawString(5, startY, consoleFontColor, "] ");
	const tb::TBStr cmdLine(_commandLine.c_str());
	_font->DrawString(15, startY, consoleFontColor, cmdLine);
	if (_cursorBlink) {
		const int l = _font->GetStringWidth(_commandLine.c_str(), _cursorPos);
		_font->DrawString(15 + l, startY, consoleFontColor, "_");
	}
}

}
