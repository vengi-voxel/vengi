/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "core/String.h"
#include "core/collection/Array.h"
#include "core/IComponent.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/ConcurrentQueue.h"

namespace util {

enum ConsoleColor {
	WHITE, BLACK, GRAY, BLUE, GREEN, YELLOW, RED, MAX_COLORS
};

class Console : public core::IComponent {
protected:
	typedef core::DynamicArray<core::String> Messages;
	Messages _messages;

	const char* _historyFilename = "history";
	core::String _consolePrompt = "> ";
	core::String _consoleCursor = "_";
	char _colorMark = '^';

	core::Array<ConsoleColor, SDL_NUM_LOG_PRIORITIES> _priorityColors {
		GRAY,
		GRAY,
		GREEN,
		WHITE,
		YELLOW,
		RED,
		RED
	};
	static_assert(7 == SDL_NUM_LOG_PRIORITIES, "Priority count doesn't match");
	static_assert(7 == MAX_COLORS, "Colors count doesn't match");

	core::String getColor(ConsoleColor color);

	/**
	 * @brief Checks whether the given input string is a color string.
	 *
	 * A color string is started with ^ and followed by a number which indicates the color.
	 */
	bool isColor(const char *cstr);

	/**
	 * @brief Set the given pointer to the next character after a color string
	 * @see isColor()
	 */
	void skipColor(const char **cstr);

	void printHistory();

	/**
	 * @brief Data structure to store a log entry call from a different thread.
	 */
	struct LogLine {
		LogLine(int _category = 0, int _priority = 0, const char* _message = nullptr) :
				category(_category), priority(_priority), message(_message ? core_strdup(_message) : nullptr) {
		}
		~LogLine() {
			core_free(message);
		}
		LogLine(LogLine&& o) noexcept {
			category = o.category;
			priority = o.priority;
			message = o.message;
			o.message = nullptr;
		}
		LogLine(const LogLine& o) noexcept {
			category = o.category;
			priority = o.priority;
			message = o.message != nullptr ? core_strdup(o.message) : nullptr;
		}
		int category;
		int priority;
		char* message;

		LogLine& operator=(LogLine&& o) noexcept {
			if (this != &o) {
				category = o.category;
				priority = o.priority;
				if (message) {
					core_free(message);
				}
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
				core_free(message);
			}
			message = core_strdup(o.message);
			return *this;
		}
	};
	core::ConcurrentQueue<LogLine> _messageQueue;
	Messages _history;
	uint32_t _historyPos = 0;
	const unsigned long _mainThread;
	void *_logFunction = nullptr;
	void *_logUserData = nullptr;
	core::String _commandLine;
	bool _useOriginalLogFunction = true;

	static core::String removeAnsiColors(const char* message);
	static void logConsole(void *userdata, int category, int priority, const char *message);
	virtual void addLogLine(int category, int priority, const char *message);

	void replaceLastParameter(const core::String& param);

	void executeCommandLine();

	bool insertClipboard();
	void drawStringColored(const core::String& str, int len);

	virtual void drawString(ConsoleColor color, const char* str, int len) = 0;

public:
	Console();
	virtual ~Console();
	virtual void construct() override;
	virtual bool init() override;
	virtual void shutdown() override;
	virtual void update(double deltaFrameSeconds);
	void clear();
	void clearCommandLine();

	// history 'scroll' methods
	void cursorUp();
	void cursorDown();

	void autoComplete();

	const core::String& commandLine() const;
};

inline const core::String& Console::commandLine() const {
	return _commandLine;
}

}
