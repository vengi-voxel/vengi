#include "TextConsole.h"
#include "engine/common/ConfigManager.h"
#include "engine/common/System.h"
#include "engine/common/Application.h"
#include <SDL.h>
#include <algorithm>
#include <iostream>

namespace {
#ifdef HAVE_NCURSES_H
const int COLOR_DEFAULT = 0;
const int COLOR_ALT = COLOR_RED;
#else
const int COLOR_DEFAULT = 0;
const int COLOR_ALT = 0;
#endif
}

TextConsole::TextConsole () :
		IConsole(), _lastUpdate(0)
{
#ifdef HAVE_NCURSES_H
	_scrollPos = 0;
	_stdwin = nullptr;
	_createWidth = _createHeight = 0;
#endif
}

TextConsole::~TextConsole ()
{
#ifdef HAVE_NCURSES_H
	clrtoeol();
	refresh();
	endwin();
#endif
	for (EntriesIter i = _entries.begin(); i != _entries.end(); ++i) {
		delete *i;
	}
}

void TextConsole::update (uint32_t deltaTime)
{
	_lastUpdate += deltaTime;

#ifdef HAVE_NCURSES_H
	int key = wgetch(_stdwin);
	while (key != ERR) {
		if (key == KEY_ENTER || key == '\n') {
			executeCommandLine();
		} else if (key == KEY_CTAB || key == '\t') {
			autoComplete();
		} else if (key == KEY_BACKSPACE || key == 8 || key == 127) {
			_cursorPos--;
			if (_cursorPos > 0)
				_commandLine.erase(_cursorPos, 1);
			else
				_cursorPos = 0;
		} else if (key == KEY_LEFT) {
			_cursorPos--;
			if (_cursorPos < 0)
				_cursorPos = 0;
		} else if (key == KEY_PPAGE) {
			_scrollPos++;
		} else if (key == KEY_NPAGE) {
			_scrollPos--;
		} else if (key == KEY_HOME) {
			_cursorPos = 0;
		} else if (key == KEY_RIGHT) {
			_cursorPos++;
			if (_cursorPos >= _commandLine.size())
				_cursorPos = _commandLine.size() - 1;
			if (_cursorPos >= COLS - 1)
				_cursorPos = COLS - 1;
		} else if (key == KEY_END) {
			_cursorPos = _commandLine.size() - 1;
		} else if (key == KEY_UP) {
			cursorUp();
		} else if (key == KEY_DOWN) {
			cursorDown();
		} else if (key >= 32 && key < 127) {
			if (_cursorPos >= _commandLine.size())
				_commandLine += (char) key;
			else
				_commandLine[_cursorPos] = (char) key;
			_cursorPos++;
		}
		key = wgetch(_stdwin);
	}
#endif
}

void TextConsole::cursorDelete (bool moveCursor)
{
	IConsole::cursorDelete(moveCursor);
	std::cout << "\b";
	std::cout << " ";
}

void TextConsole::logInfo (const std::string& string)
{
	_entries.push_back(new ConsoleEntry(COLOR_DEFAULT, false, string));
}

void TextConsole::logError (const std::string& string)
{
	_entries.push_back(new ConsoleEntry(COLOR_ALT, true, string));
}

void TextConsole::logDebug (const std::string& string)
{
	if (!Config.isDebug()) {
		return;
	}

	logInfo(string);
}

bool TextConsole::onKeyPress (int32_t key, int16_t modifier)
{
#ifdef HAVE_NCURSES_H
	// curses is handling our key presses in console mode
	return false;
#else
	if (key == SDLK_RETURN) {
		executeCommandLine();
	} else if (key == SDLK_TAB) {
		autoComplete();
	} else if (key >= 32 && key <= 126) {
		const char chr = (char) key;
		_commandLine += chr;
	}
	return true;
#endif
}

void TextConsole::render ()
{
	int timeout = 200;

	if (_lastUpdate < timeout)
		return;

	_lastUpdate = 0;

#ifdef HAVE_NCURSES_H
	bkgdset(' ');
	wclear(_stdwin);

	const int w = COLS - 1;
	const int h = LINES - 1;

	if (w < 3 && h < 3) {
		return;
	}

	box(_stdwin, ACS_VLINE , ACS_HLINE);

	// Draw the header
	setColor(COLOR_GREEN);
	mvaddstr(0, 2, Singleton<Application>::getInstance().getName().c_str());

	const int lines = LINES - 2;
	const int lastLine = _entries.size();
	const int startLine = std::max(0, (int) _entries.size() - _scrollPos - lines);
	int y = 1;
	for (EntriesIter i = _entries.begin() + startLine; i != _entries.end() && i != _entries.begin() + lines; ++i) {
		const ConsoleEntry &e = *(*i);
		int x = 1;
		// color of the first character of the line
		setColor(e.color);
		if (e.bold)
			wattron(_stdwin, A_BOLD);

		for (const char *pos = e.text.c_str(); pos[0] != '\0'; ++pos) {
			if (pos[0] == '\n' || pos[0] == '\r') {
				x++;
			} else if (x < w) {
				mvaddnstr(y, x, pos, 1);
				x++;
			} else {
				y++;
				x = 1;
				mvaddnstr(y, x, "> ", 2);
				x += 2;
				mvaddnstr(y, x, pos, 1);
			}
		}

		if (e.bold)
			wattroff(_stdwin, A_BOLD);
		y++;
	}

	// draw a scroll indicator
	if (_scrollPos != 0) {
		setColor(COLOR_GREEN);
		mvaddnstr(1 + ((lastLine - _scrollPos) * lines / lastLine), w, "O", 1);
	}

	// reset drawing colors
	resetColor();
	const int32_t xPos = COLS - 5;

	for (int x = 2; x < COLS - 1; x++) {
		mvaddstr(LINES - 1, x, " ");
	}

	// TODO: fix rendering for too long strings
	mvaddnstr(LINES - 1, 3, _commandLine.c_str(), xPos);

	wrefresh(_stdwin);

	renderHook();

	// move the cursor to input position
	wmove(_stdwin, LINES - 1, 3 + _cursorPos);
	waddch(_stdwin, '_');

	// Print it on to the real screen
	refresh();
#endif
}

void TextConsole::renderHook()
{
}

void TextConsole::init (IFrontend *frontend)
{
#ifdef HAVE_NCURSES_H
	// Start curses mode
	_stdwin = initscr();
	// Line buffering disabled
	cbreak();
	// We get F1, F2 etc..
	keypad(stdscr, TRUE);
	// Don't echo() while we do getch
	noecho();
	// non-blocking input
	nodelay(_stdwin, TRUE);
	// enable the cursor
	curs_set(0);

	if (has_colors()) {
		start_color();
		// this is ncurses-specific
		use_default_colors();
		// COLOR_PAIR(0) is terminal default
		init_pair(1, COLOR_RED, -1);
		init_pair(2, COLOR_GREEN, -1);
		init_pair(3, COLOR_YELLOW, -1);
		init_pair(4, COLOR_BLUE, -1);
		init_pair(5, COLOR_CYAN, -1);
		init_pair(6, COLOR_MAGENTA, -1);
		init_pair(7, -1, -1);
	}
#endif
}

inline void TextConsole::setColor (int color) const
{
#ifdef HAVE_NCURSES_H
	if (!has_colors()) {
		return;
	}

	color_set(color, nullptr);
#endif
}

inline void TextConsole::resetColor () const
{
	setColor(COLOR_DEFAULT);
}
