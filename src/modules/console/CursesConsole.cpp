/**
 * @file
 */

#include "CursesConsole.h"
#include "app/App.h"
#include "core/Log.h"
#include "core/GameConfig.h"
#include "command/CommandHandler.h"
#include <SDL_stdinc.h>
#include <SDL_assert.h>

#include "engine-config.h"

#ifdef CURSES_HAVE_NCURSES_H
#include <ncurses.h>
#include <signal.h>
#endif

namespace console {

CursesConsole::CursesConsole() :
		Super() {
	_consoleMarginLeft = 1;
	_consoleMarginLeftBehindPrompt = 1;
	_consoleActive = true;
	uv_loop_init(&_loop);
}

CursesConsole::~CursesConsole() {
	uv_run(&_loop, UV_RUN_NOWAIT);
	core_assert_always(uv_loop_close(&_loop) == 0);
}

void CursesConsole::update(double deltaFrameSeconds) {
	Super::update(deltaFrameSeconds);
	if (_cursesVar->isDirty()) {
		_enableCurses = _cursesVar->boolVal();
		if (_enableCurses && !_cursesActive) {
			initCurses();
		} else if (!_enableCurses && _cursesActive) {
			shutdownCurses();
		}
		_cursesVar->markClean();
	}
#ifdef CURSES_HAVE_NCURSES_H
	if (_cursesActive) {
		int key = getch();
		while (key != ERR) {
			if (key == KEY_ENTER || key == '\n') {
				executeCommandLine();
			} else if (key == KEY_CTAB || key == '\t') {
				autoComplete();
			} else if (key == KEY_RESIZE) {
				clear();
				refresh();
			} else if (key == KEY_BACKSPACE || key == 8 || key == 127) {
				cursorDelete();
			} else if (key == KEY_LEFT) {
				cursorLeft();
			} else if (key == KEY_PPAGE) {
				scrollPageUp();
			} else if (key == KEY_NPAGE) {
				scrollPageDown();
			} else if (key == KEY_HOME) {
				_cursorPos = 0;
			} else if (key == KEY_RIGHT) {
				cursorRight();
			} else if (key == KEY_END) {
				_cursorPos = _commandLine.size();
			} else if (key == KEY_UP) {
				cursorUp();
			} else if (key == KEY_DC) {
				cursorDelete(false);
			} else if (key == KEY_IC) {
				_overwrite ^= true;
			} else if (key == KEY_DOWN) {
				cursorDown();
			} else if (key >= 32 && key < 127) {
				const char buf[] = { (char)key, '\0' };
				insertText(buf);
			}
			key = getch();
		}
		const math::Rect<int> rect(0, 0, COLS - 1, LINES - 1);
		render(rect, deltaFrameSeconds);
		return;
	}
#endif
	uv_run(&_loop, UV_RUN_NOWAIT);
	handleTTYInput();
}

void CursesConsole::handleTTYInput() {
	const ConsoleKey consoleKey = _input.swapConsoleKey();
	char cmdlineBuf[256];
	const bool commandLineExecute = _input.swap(cmdlineBuf, sizeof(cmdlineBuf));
	_commandLine = core::String(cmdlineBuf);
	_cursorPos = _commandLine.size();
	if (consoleKey != ConsoleKey::None) {
		switch (consoleKey) {
		case ConsoleKey::Tab:
			autoComplete();
			_input.setCmdline(_commandLine.c_str(), _commandLine.size());
			break;
		case ConsoleKey::CursorUp:
			cursorUp();
			_input.setCmdline(_commandLine.c_str(), _commandLine.size());
			break;
		case ConsoleKey::CursorDown:
			cursorDown();
			_input.setCmdline(_commandLine.c_str(), _commandLine.size());
			break;
		case ConsoleKey::Abort:
			if (_abortPressCount == _cursorPos) {
				app::App::getInstance()->requestQuit();
			} else {
#ifdef DEBUG
				SDL_TriggerBreakpoint();
#else
				_abortPressCount = _cursorPos;
				Log::info("press once again to abort");
#endif
			}
			break;
		default:
			break;
		}
	}
	if (_abortPressCount != _cursorPos) {
		_abortPressCount = -1;
	}
	if (commandLineExecute) {
		executeCommandLine();
		_abortPressCount = -1;
	}
}

void CursesConsole::drawString(int x, int y, const glm::ivec4& color, int colorIndex, const char* str, int len) {
#ifdef CURSES_HAVE_NCURSES_H
	if (!_cursesActive) {
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "%s\n", str);
		return;
	}
	if (colorIndex >= 0) {
		attron(COLOR_PAIR(colorIndex + 1));
	}
	mvaddnstr(y, x, str, len);
	clrtoeol();
	if (colorIndex >= 0) {
		attroff(COLOR_PAIR(colorIndex + 1));
	}
#else
	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "%s\n", str);
#endif
}

int CursesConsole::lineHeight() {
	return 1;
}

glm::ivec2 CursesConsole::stringSize(const char* s, int length) {
	return glm::ivec2(core_min(length, (int)SDL_strlen(s)), lineHeight());
}

void CursesConsole::afterRender(const math::Rect<int> &rect) {
#ifdef CURSES_HAVE_NCURSES_H
	if (!_cursesActive) {
		return;
	}
	clrtoeol();
	refresh();
#endif
}

void CursesConsole::beforeRender(const math::Rect<int> &) {
}

void CursesConsole::initCurses() {
#ifdef CURSES_HAVE_NCURSES_H
	if (!_enableCurses) {
		return;
	}
	core_assert(_cursesActive == false);
	_cursesActive = true;
	_useOriginalLogFunction = false;
	// Start curses mode
	initscr();
	// We get F1, F2 etc..
	keypad(stdscr, TRUE);
	// Don't echo() while we do getch
	noecho();
	// non-blocking input
	nodelay(stdscr, TRUE);
	// enable the cursor
	curs_set(0);

	if (has_colors()) {
		start_color();
		use_default_colors();
		init_pair(util::WHITE + 1, COLOR_WHITE, -1);
		init_pair(util::BLACK + 1, COLOR_BLACK, -1);
		init_pair(util::GRAY + 1, COLOR_BLACK, -1);
		init_pair(util::BLUE, COLOR_BLUE, -1);
		init_pair(util::GREEN, COLOR_GREEN, -1);
		init_pair(util::YELLOW, COLOR_YELLOW, -1);
		init_pair(util::RED, COLOR_RED, -1);
	}
#endif
}

void CursesConsole::construct() {
	Super::construct();
	_cursesVar = core::Var::get(cfg::ConsoleCurses, "false", "Use curses for the console");
}

bool CursesConsole::init() {
	Super::init();
	_enableCurses = _cursesVar->boolVal();
	_input.init(&_loop);
	initCurses();
	return true;
}

void CursesConsole::shutdownCurses() {
#ifdef CURSES_HAVE_NCURSES_H
	if (!_cursesActive) {
		return;
	}
	_useOriginalLogFunction = true;
	refresh();
	endwin();
	for (const auto& e : _messages) {
		const char *str = e.c_str();
		if (isColor(str)) {
			skipColor(&str);
		}
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "%s", str);
	}
	_cursesActive = false;
#endif
}

void CursesConsole::shutdown() {
	SDL_LogSetOutputFunction(_logFunction, _logUserData);
	shutdownCurses();
	_input.shutdown();
	Super::shutdown();
}

}
