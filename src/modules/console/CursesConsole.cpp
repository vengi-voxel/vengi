/**
 * @file
 */

#include "CursesConsole.h"
#include "core/App.h"
#include "core/Log.h"
#include <SDL_stdinc.h>

#include "engine-config.h"

#ifdef CURSES_HAVE_NCURSES_H
#include <ncurses.h>
#include <signal.h>
#endif

namespace console {

CursesConsole::CursesConsole() :
		Super() {
#ifdef CURSES_HAVE_NCURSES_H
	_useOriginalLogFunction = false;
#endif
	_consoleMarginLeft = 1;
	_consoleMarginLeftBehindPrompt = 1;
	_consoleActive = true;
}

void CursesConsole::update(uint32_t deltaTime) {
	Super::update(deltaTime);
#ifdef CURSES_HAVE_NCURSES_H
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
	render(rect, (long)deltaTime);
#endif
}

void CursesConsole::drawString(int x, int y, const glm::ivec4& color, const char* str, int len) {
#ifdef CURSES_HAVE_NCURSES_H
	mvaddnstr(y, x, str, len);
	clrtoeol();
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
	clrtoeol();
	refresh();
#endif
}

void CursesConsole::beforeRender(const math::Rect<int> &) {
}

bool CursesConsole::init() {
#ifdef CURSES_HAVE_NCURSES_H
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
		init_pair(1, COLOR_RED, -1);
		init_pair(2, COLOR_GREEN, -1);
		init_pair(3, COLOR_YELLOW, -1);
		init_pair(4, COLOR_BLUE, -1);
		init_pair(5, COLOR_CYAN, -1);
		init_pair(6, COLOR_MAGENTA, -1);
		init_pair(7, -1, -1);
	}
#endif
	return true;
}

void CursesConsole::shutdown() {
	SDL_LogSetOutputFunction(_logFunction, _logUserData);
#ifdef CURSES_HAVE_NCURSES_H
	refresh();
	endwin();
	for (const auto& e : _messages) {
		Log::info("%s", e.c_str());
	}
#endif
	Super::shutdown();
}

}
