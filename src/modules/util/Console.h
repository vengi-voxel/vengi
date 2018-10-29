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

	/**
	 * @brief Data structure to store a log entry call from a different thread.
	 */
	struct LogLine {
		LogLine(int _category = 0, SDL_LogPriority _priority = (SDL_LogPriority)0, const char* _message = nullptr) :
				category(_category), priority(_priority), message(_message ? SDL_strdup(_message) : nullptr) {
		}
		~LogLine() {
			SDL_free(message);
			message = (char*)(void*)0xdeadbeef;
		}
		LogLine(LogLine&& o) {
			category = o.category;
			priority = o.priority;
			message = o.message;
			o.message = nullptr;
		}
		int category;
		SDL_LogPriority priority;
		char* message;

		LogLine& operator=(LogLine&& o) {
			if (this != &o) {
				category = o.category;
				priority = o.priority;
				message = o.message;
				o.message = nullptr;
			}
			return *this;
		}

		LogLine& operator=(const LogLine& o) {
			category = o.category;
			priority = o.priority;
			if (message) {
				SDL_free(message);
			}
			message = SDL_strdup(o.message);
			return *this;
		}

		inline bool operator<(const LogLine& logLine) const {
			return category < logLine.category && priority < logLine.priority && message < logLine.message;
		}
	};
	core::ConcurrentQueue<LogLine> _messageQueue;
	Messages _history;
	uint32_t _historyPos = 0;
	const std::thread::id _mainThread;
	bool _consoleActive = false;
	SDL_LogOutputFunction _logFunction = nullptr;
	void *_logUserData = nullptr;
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
	void addLogLine(int category, SDL_LogPriority priority, const char *message);

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
	virtual void construct() override;
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
