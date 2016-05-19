#include "Console.h"
#include "core/App.h"
#include "io/Filesystem.h"
#include "core/Command.h"
#include "core/Common.h"
#include "core/Color.h"
#include "core/Tokenizer.h"

namespace ui {

namespace {
static const char* historyFilename = "history";
static const std::string consolePrompt = "> ";
static const std::string consoleCursor = "_";
static const int consoleMarginLeft = 5;
static const int consoleMarginLeftBehindPrompt = 13;
static const tb::TBColor consoleBgColor(127, 127, 127, 150);
static const char colorMark = '^';

static const tb::TBColor colors[MAX_COLORS] = {
	tb::TBColor(255, 255, 255),
	tb::TBColor(0, 0, 0),
	tb::TBColor(127, 127, 127),
	tb::TBColor(0, 0, 255),
	tb::TBColor(0, 255, 0),
	tb::TBColor(255, 255, 0),
	tb::TBColor(255, 0, 0),
};
static_assert(SDL_arraysize(colors) == MAX_COLORS, "Color count doesn't match");

static const ConsoleColor priorityColors[SDL_NUM_LOG_PRIORITIES] = {
	GRAY,
	GRAY,
	GREEN,
	WHITE,
	YELLOW,
	RED,
	RED
};
static_assert(SDL_arraysize(priorityColors) == SDL_NUM_LOG_PRIORITIES, "Priority count doesn't match");
}

std::string getColor(ConsoleColor color) {
	core_assert(color >= 0 && color <= (int)SDL_arraysize(colors));
	std::string s;
	s += colorMark;
	s += std::to_string((int)color);
	return s;
}

static inline bool isColor(const char *cstr) {
	static const char maxColor = MAX_COLORS + '0';
	return cstr[0] == colorMark && cstr[1] >= '0' && cstr[1] <= maxColor;
}

static inline void skipColor(const char **cstr) {
	static_assert((int)MAX_COLORS < 10, "max colors must not exceed one ascii char for encoding");
	*cstr += 2;
}

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
	core::Command::registerCommand("clear", [&] (const core::CmdArgs& args) { clear(); });
	core::Command::registerCommand("logerror", [&] (const core::CmdArgs& args) {
		if (args.empty()) {
			return;
		}
		Log::error("%s", args[0].c_str());
	});
	core::Command::registerCommand("loginfo", [&] (const core::CmdArgs& args) {
		if (args.empty()) {
			return;
		}
		Log::info("%s", args[0].c_str());
	});
	core::Command::registerCommand("logdebug", [&] (const core::CmdArgs& args) {
		if (args.empty()) {
			return;
		}
		Log::debug("%s", args[0].c_str());
	});
	core::Command::registerCommand("logwarn", [&] (const core::CmdArgs& args) {
		if (args.empty()) {
			return;
		}
		Log::warn("%s", args[0].c_str());
	});
	const io::FilesystemPtr& fs = core::App::getInstance()->filesystem();
	const std::string& content = fs->load(historyFilename);
	core::string::splitString(content, _history, "\n");
	_historyPos = _history.size();
	Log::info("Loaded %i history entries", _historyPos);

	return true;
}

void Console::shutdown() {
	std::string content;
	for (const std::string& s : _history) {
		content += s;
		content += '\n';
	}

	const io::FilesystemPtr& fs = core::App::getInstance()->filesystem();
	if (!fs->write(historyFilename, content)) {
		Log::warn("Failed to write the history");
	} else {
		Log::debug("Wrote the history");
	}
}

bool Console::onKeyPress(int32_t key, int16_t modifier) {
	if (!_consoleActive) {
		return false;
	}

	if (modifier & KMOD_ALT) {
		if (key == SDLK_BACKSPACE) {
			cursorDeleteWord();
		} else if (key == SDLK_LEFT) {
			cursorWordLeft();
		} else if (key == SDLK_RIGHT) {
			cursorWordRight();
		}
		return true;
	}

	if (modifier & KMOD_CTRL) {
		if (key == SDLK_a) {
			_cursorPos = 0;
		} else if (key == SDLK_e) {
			_cursorPos = _commandLine.size();
		} else if (key == SDLK_c) {
			_messages.push_back(consolePrompt + _commandLine);
			clearCommandLine();
		} else if (key == SDLK_d) {
			toggle();
		} else if (key == SDLK_l) {
			clear();
		} else if (key == SDLK_w) {
			cursorDeleteWord();
		} else if (key == SDLK_LEFT) {
			cursorWordLeft();
		} else if (key == SDLK_RIGHT) {
			cursorWordRight();
		}
		return true;
	}

	if (modifier & KMOD_SHIFT) {
		if (key == SDLK_TAB) {
			toggle();
		} else if (key == SDLK_HOME) {
			_scrollPos = _messages.size() - _maxLines + 1;
		} else if (key == SDLK_END) {
			_scrollPos = 0;
		} else if (key == SDLK_PAGEUP) {
			scrollPageUp();
		} else if (key == SDLK_PAGEDOWN) {
			scrollPageDown();
		}
		return true;
	}

	switch (key) {
	case SDLK_ESCAPE:
		toggle();
		break;
	case SDLK_HOME:
		_cursorPos = 0;
		break;
	case SDLK_END:
		_cursorPos = _commandLine.size();
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
		scrollPageUp();
		break;
	case SDLK_PAGEDOWN:
		scrollPageDown();
		break;
	case SDLK_TAB:
		autoComplete();
		break;
	}

	return true;
}

void Console::executeCommandLine() {
	_messages.push_back(consolePrompt + _commandLine);
	_scrollPos = 0;
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
		if (core::Command::execute(cmd, tokens)) {
			continue;
		}
		const core::VarPtr& c = core::Var::get(cmd, "", core::CV_NOTCREATEEMPTY);
		if (!c) {
			Log::info("unknown config variable %s", cmd.c_str());
			continue;
		}
		if (tokens.empty()) {
			if (c->strVal().empty())
				Log::info("%s: no value set", cmd.c_str());
			else
				Log::info("%s: %s", cmd.c_str(), c->strVal().c_str());
		} else {
			c->setVal(core::string::eraseAllSpaces(tokens[0]));
		}
	}
	clearCommandLine();
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

	if (_overwrite && _cursorPos < int(_commandLine.size())) {
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

void Console::cursorRight() {
	const int size = _commandLine.size();
	if (_cursorPos < size) {
		_cursorPos++;
	}
}

void Console::cursorWordLeft() {
	auto prevWordEnd = _commandLine.find_last_of(" ", std::max(0, _cursorPos - 1));
	if (std::string::npos == prevWordEnd) {
		_cursorPos = 0;
		return;
	}
	_cursorPos = prevWordEnd;
}

void Console::cursorWordRight() {
	const int spaceOffset = _commandLine[_cursorPos] == ' ' ? 1 : 0;
	const std::string& partialCommandLine = _commandLine.substr(_cursorPos + spaceOffset);
	const size_t nextWordEnd = partialCommandLine.find_first_of(" ");
	if (std::string::npos == nextWordEnd) {
		_cursorPos = _commandLine.size();
		return;
	}
	_cursorPos = std::min(_commandLine.size(), _cursorPos + nextWordEnd + spaceOffset);
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
		clearCommandLine();
		return;
	}
	_commandLine = _history[_historyPos];
	_cursorPos = _commandLine.size();
}

void Console::scrollUp(const int lines) {
	const int scrollableLines = _messages.size() - _maxLines;
	if (scrollableLines <= 0) {
		return;
	}
	if (_scrollPos <= scrollableLines) {
		_scrollPos += std::min(lines, scrollableLines - _scrollPos + 1);
	}
}

void Console::scrollDown(const int lines) {
	if (_scrollPos <= 0) {
		return;
	}
	_scrollPos = std::max(_scrollPos - lines, 0);
}

void Console::scrollPageUp() {
	// scroll one page minus one line minus prompt
	scrollUp(_maxLines - 2);
}


void Console::scrollPageDown() {
	// scroll one page minus one line minus prompt
	scrollDown(_maxLines - 2);
}

void Console::autoComplete() {
	std::vector<std::string> matches;
	std::vector<std::string> strings;
	core::string::splitString(_commandLine, strings);
	const std::string search = strings.empty() ? "*" : strings[0] + "*";
	core::Command::visitSorted([&] (const core::Command& cmd) {
		if (core::string::matches(search, cmd.name())) {
			matches.push_back(cmd.name());
		}
	});
	core::Var::visitSorted([&] (const core::VarPtr& var) {
		if (core::string::matches(search, var->name())) {
			matches.push_back(var->name());
		}
	});

	if (matches.empty()) {
		return;
	}

	if (matches.size() == 1) {
		if (strings.size() == 1) {
			_commandLine = matches.front() + " ";
		} else {
			_commandLine.erase(0, strings[0].size());
			_commandLine.insert(0, matches.front());
		}
		_cursorPos = _commandLine.size();
	} else {
		_messages.push_back(consolePrompt + _commandLine);
		std::sort(begin(matches), end(matches), [](auto const &v1, auto const &v2) {
			return v1 < v2;
		});
		for (auto match : matches) {
			Log::info("%s", match.c_str());
		}
	}
}

void Console::cursorDelete(bool moveCursor) {
	if (_commandLine.empty()) {
		return;
	}
	if (moveCursor) {
		if (_cursorPos <= 0) {
			return;
		}
		cursorLeft();
	}
	_commandLine.erase(_cursorPos, 1);
}

void Console::cursorDeleteWord() {
	if (_commandLine.empty()) {
		return;
	}
	if (0 >= _cursorPos) {
		return;
	}
	const int spaceOffset = _commandLine[_cursorPos - 1] == ' ' ? 1 : 0;
	const size_t prevWordStart = _commandLine.find_last_of(" ", _cursorPos - spaceOffset - 1);
	if (std::string::npos == prevWordStart) {
		_commandLine.erase(0, _cursorPos);
		_cursorPos = 0;
		return;
	}
	_commandLine.erase(prevWordStart + 1, _cursorPos - prevWordStart - 1);
	_cursorPos = prevWordStart + 1;
}

void Console::logConsole(void *userdata, int category, SDL_LogPriority priority, const char *message) {
	Console* console = (Console*)userdata;
	const bool hasColor = isColor(message);
	const std::string& color = hasColor ? "" : getColor(priorityColors[priority]);
	console->_messages.push_back(color + message);
	if (hasColor) {
		skipColor(&message);
	}
	console->_logFunction(userdata, category, priority, message);
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

void Console::clear() {
	clearCommandLine();
	_messages.clear();
	_scrollPos = 0;
}

inline void Console::clearCommandLine() {
	_cursorPos = 0;
	_commandLine.clear();
}

void Console::drawString(int x, int y, const std::string& str, int len) {
	const char *cstr = str.c_str();
	tb::TBColor color = colors[WHITE];
	if (isColor(cstr)) {
		skipColor(&cstr);
		const int colorIndex = str[1] - '0';
		if (colorIndex >= 0 && colorIndex < (int)SDL_arraysize(colors)) {
			color = colors[colorIndex];
		}
	}
	_font->DrawString(x, y, color, cstr, len);
}

void Console::render(const tb::TBRect &rect) {
	_frame++;
	if ((_frame % 10) == 0) {
		_cursorBlink ^= true;
	}

	if (!_consoleActive) {
		return;
	}

	tb::g_renderer->DrawRectFill(rect, consoleBgColor);

	const int lineHeight = _font->GetFontDescription().GetSize();
	_maxLines = rect.h / lineHeight;
	int maxY = _messages.size() * lineHeight;
	const int startY = std::min(rect.y + rect.h - lineHeight, maxY);
	MessagesIter i = _messages.rbegin();
	std::advance(i, _scrollPos);
	for (int y = startY - lineHeight; i != _messages.rend(); ++i, y -= lineHeight) {
		if (y < 0) {
			break;
		}
		drawString(consoleMarginLeft, y, *i);
	}

	drawString(consoleMarginLeft, startY, consolePrompt);
	drawString(consoleMarginLeft + consoleMarginLeftBehindPrompt, startY, _commandLine);
	if (_cursorBlink) {
		const int l = _font->GetStringWidth(_commandLine.c_str(), _cursorPos);
		drawString(consoleMarginLeft + consoleMarginLeftBehindPrompt + l, startY, consoleCursor);
	}
}

}
