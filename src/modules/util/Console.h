/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "core/Log.h"
#include "core/String.h"
#include "core/collection/ConcurrentQueue.h"
#include "core/collection/DynamicArray.h"

namespace command {
struct CommandExecutionListener;
}

namespace util {

class Console : public core::IComponent {
protected:
	struct Message {
		Log::Level priority;
		core::String message;
		Message(Log::Level _priority, const core::String &_message) : priority(_priority), message(_message) {
		}
	};
	using Messages = core::DynamicArray<Message>;
	Messages _messages;

	const char *_historyFilename = "history";
	core::String _consolePrompt = "> ";
	core::String _consoleCursor = "_";

	void printHistory();

	/**
	 * @brief Data structure to store a log entry call from a different thread.
	 */
	struct LogLine {
		LogLine(int _category = 0, Log::Level _priority = Log::Level::None, const char *_message = nullptr)
			: category(_category), priority(_priority), message(_message ? core_strdup(_message) : nullptr) {
		}
		~LogLine() {
			core_free(message);
		}
		LogLine(LogLine &&o) noexcept {
			category = o.category;
			priority = o.priority;
			message = o.message;
			o.message = nullptr;
		}
		LogLine(const LogLine &o) noexcept {
			category = o.category;
			priority = o.priority;
			message = o.message != nullptr ? core_strdup(o.message) : nullptr;
		}
		int category;
		Log::Level priority;
		char *message;

		LogLine &operator=(LogLine &&o) noexcept {
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

		LogLine &operator=(const LogLine &o) {
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
	// other threads are logging into this queue
	core::ConcurrentQueue<LogLine> _messageQueue;
	core::DynamicArray<core::String> _history;
	uint32_t _historyPos = 0;
	const unsigned long _mainThread;
	void *_logFunction = nullptr;
	void *_logUserData = nullptr;
	core::String _commandLine;
	bool _useOriginalLogFunction = true;

	static core::String removeAnsiColors(const char *message);
	static void logConsole(void *userdata, int category, Log::Level priority, const char *message);
	virtual void addLogLine(int category, Log::Level priority, const char *message);

	void replaceLastParameter(const core::String &param);

	void executeCommandLine(command::CommandExecutionListener *listener);

	bool insertClipboard();

	virtual void drawString(const Message& msg) = 0;

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

	const core::String &commandLine() const;
};

inline const core::String &Console::commandLine() const {
	return _commandLine;
}

} // namespace util
