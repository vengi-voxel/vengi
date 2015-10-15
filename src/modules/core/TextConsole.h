#pragma once

#include "engine/common/IConsole.h"
#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include <vector>

#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#include <signal.h>
#endif

class TextConsole: public IConsole {
protected:
	struct ConsoleEntry {
		ConsoleEntry (int _color, bool _bold, const std::string& _text) :
				color(_color), bold(_bold), text(_text)
		{
		}
		const int color;
		const bool bold;
		const std::string text;
	};

#ifdef HAVE_NCURSES_H
	WINDOW *_stdwin;
	int _createWidth, _createHeight; // the dimensions that the window was created with
	int _scrollPos;
#endif

	int _lastUpdate;
	typedef std::vector<ConsoleEntry*> Entries;
	typedef Entries::const_iterator EntriesIter;
	Entries _entries;

	virtual void resetColor () const;
	virtual void setColor (int color) const;
	virtual void renderHook ();
public:
	TextConsole ();
	virtual ~TextConsole ();

	virtual void logInfo (const std::string& string) override;
	virtual void logError (const std::string& string) override;
	virtual void logDebug (const std::string& string) override;
	virtual bool onKeyPress (int32_t key, int16_t modifier) override;
	virtual void update (uint32_t deltaTime) override;
	virtual void render () override;
	virtual void init (IFrontend *frontend) override;
	virtual void cursorDelete (bool moveCursor = true) override;
};
