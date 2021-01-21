/**
 * @file
 */

#pragma once

#include <SDL_stdinc.h>
#include <SDL_log.h>
#include "core/String.h"
#include "core/Var.h"
#include "core/Assert.h"
#include "core/IComponent.h"
#include "core/collection/DynamicArray.h"
#include "math/Rect.h"
#include "core/collection/ConcurrentQueue.h"
#include <thread>

namespace util {

enum ConsoleColor {
	WHITE, BLACK, GRAY, BLUE, GREEN, YELLOW, RED, MAX_COLORS
};

extern core::String getColor(ConsoleColor color);

class Console : public core::IComponent {
protected:
	typedef core::DynamicArray<core::String> Messages;
	Messages _messages;

	int _consoleMarginLeft = 5;
	int _consoleMarginLeftBehindPrompt = 13;

	const char* _historyFilename = "history";
	core::String _consolePrompt = "> ";
	core::String _consoleCursor = ".";
	char _colorMark = '^';

	core::String getColor(ConsoleColor color);

	bool isColor(const char *cstr);

	void skipColor(const char **cstr);

	/**
	 * @brief Data structure to store a log entry call from a different thread.
	 */
	struct LogLine {
		LogLine(int _category = 0, SDL_LogPriority _priority = (SDL_LogPriority)0, const char* _message = nullptr) :
				category(_category), priority(_priority), message(_message ? SDL_strdup(_message) : nullptr) {
		}
		~LogLine() {
			SDL_free(message);
			message = (char*)(intptr_t)0xdeadbeef;
		}
		LogLine(LogLine&& o) noexcept {
			category = o.category;
			priority = o.priority;
			message = o.message;
			o.message = nullptr;
		}
		int category;
		SDL_LogPriority priority;
		char* message;

		LogLine& operator=(LogLine&& o) noexcept {
			if (this != &o) {
				category = o.category;
				priority = o.priority;
				message = o.message;
				o.message = nullptr;
			}
			return *this;
		}

		LogLine& operator=(const LogLine& o) {
			if (&o == this) {
				return *this;
			}
			category = o.category;
			priority = o.priority;
			if (message) {
				SDL_free(message);
			}
			message = SDL_strdup(o.message);
			return *this;
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
	core::String _commandLine;
	// commandline character will get overwritten if this is true
	bool _overwrite = false;
	bool _cursorBlink = false;
	bool _useOriginalLogFunction = true;
	int _frame = 0;
	int _cursorPos = 0;
	int _scrollPos = 0;
	int _maxLines = 0;
	int _fontSize = 14;

	static core::String removeAnsiColors(const char* message);
	static void logConsole(void *userdata, int category, SDL_LogPriority priority, const char *message);
	void addLogLine(int category, SDL_LogPriority priority, const char *message);

	// cursor move on the commandline
	void cursorLeft();
	void cursorRight();
	void cursorWordLeft();
	void cursorWordRight();

	void replaceLastParameter(const core::String& param);

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
	void insertText(const core::String& text);
	void drawString(int x, int y, const core::String& str, int len);

	virtual void beforeRender(const math::Rect<int> &rect) {}
	virtual void afterRender(const math::Rect<int> &rect) {}
	virtual int lineHeight() = 0;
	virtual glm::ivec2 stringSize(const char *c, int length) = 0;
	virtual void drawString(int x, int y, const glm::ivec4& color, int colorIndex, const char* str, int len) = 0;

public:
	Console();
	virtual ~Console();
	virtual void construct() override;
	virtual bool init() override;
	virtual void shutdown() override;
	virtual bool toggle();
	virtual void update(double deltaFrameSeconds);
	void clear();
	void clearCommandLine();
	void render(const math::Rect<int> &rect, double deltaFrameSeconds);
	bool isActive() const;
	bool onTextInput(const core::String& text);
	bool onKeyPress(int32_t key, int16_t modifier);
	bool onMouseWheel(int32_t x, int32_t y);
	bool onMouseButtonPress(int32_t x, int32_t y, uint8_t button);

	void autoComplete();

	const core::String& commandLine() const;
};

inline bool Console::isActive() const {
	return _consoleActive;
}

inline const core::String& Console::commandLine() const {
	return _commandLine;
}

}
