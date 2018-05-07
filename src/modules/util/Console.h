#pragma once

#include <SDL.h>
#include <vector>
#include <string>
#include <thread>
#include "core/Var.h"
#include "core/IComponent.h"
#include "math/Rect.h"
#include "collection/ConcurrentQueue.h"

namespace util {

enum ConsoleColor {
	WHITE, BLACK, GRAY, BLUE, GREEN, YELLOW, RED, MAX_COLORS
};

extern std::string getColor(ConsoleColor color);

class Console : public core::IComponent {
protected:
	typedef std::vector<std::string> Messages;
	typedef Messages::const_reverse_iterator MessagesIter;
	Messages _messages;
	core::ConcurrentQueue<std::string> _messageQueue;
	Messages _history;
	uint32_t _historyPos = 0;
	const std::thread::id _mainThread;
	bool _consoleActive = false;
	SDL_LogOutputFunction _logFunction = nullptr;
	core::VarPtr _autoEnable;
	std::string _commandLine;
	// commandline character will get overwritten if this is true
	bool _overwrite = false;
	bool _cursorBlink = false;
	int _frame = 0;
	int _cursorPos = 0;
	int _scrollPos = 0;
	int _maxLines = 0;

	static std::string removeAnsiColors(const char* message);
	static void logConsole(void *userdata, int category, SDL_LogPriority priority, const char *message);

	// cursor move on the commandline
	void cursorLeft();
	void cursorRight();
	void cursorWordLeft();
	void cursorWordRight();

	// history 'scroll' methods
	void cursorUp();
	void cursorDown();

	void scrollUp(const int lines = 1);
	void scrollDown(const int lines = 1);
	void scrollPageUp();
	void scrollPageDown();

	void executeCommandLine();

	// removed the character under the cursor position
	void cursorDelete(bool moveCursor = true);
	void cursorDeleteWord();

	bool insertClipboard();
	void insertText(const std::string& text);
	void drawString(int x, int y, const std::string& str, int len);

	virtual void beforeRender(const math::Rect<int> &rect) {}
	virtual void afterRender(const math::Rect<int> &rect) {}
	virtual int lineHeight() = 0;
	virtual glm::ivec2 stringSize(const char *c, int length) = 0;
	virtual void drawString(int x, int y, const glm::ivec4& color, const char* str, int len) = 0;

public:
	Console();
	virtual ~Console() {}
	virtual void construct();
	virtual bool init() override;
	virtual void shutdown() override;
	virtual bool toggle();
	void update(uint64_t dt);
	void clear();
	void clearCommandLine();
	void render(const math::Rect<int> &rect, long deltaFrame);
	bool isActive() const;
	bool onTextInput(const std::string& text);
	bool onKeyPress(int32_t key, int16_t modifier);
	bool onMouseWheel(int32_t x, int32_t y);
	bool onMouseButtonPress(int32_t x, int32_t y, uint8_t button);

	void autoComplete();

	const std::string& commandLine() const;
};

inline bool Console::isActive() const {
	return _consoleActive;
}

inline const std::string& Console::commandLine() const {
	return _commandLine;
}

}
