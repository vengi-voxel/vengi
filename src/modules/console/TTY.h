/**
 * @file
 */

#pragma once

#include <uv.h>

namespace console {

enum class ConsoleKey {
	None,
	Tab,
	CursorLeft,
	CursorRight,
	CursorUp,
	CursorDown,

	Max
};

/**
 * @brief Non-blocking console input reading e.g. for dedicated server command line
 */
class TTY {
private:
	char _input[256] = "";
	uv_tty_t _tty;
	char _cmdline[256] = "";
	size_t _cmdlineSize = 0; // used in raw mode
	bool _cmdlineValid = false;
	ConsoleKey _cmdlineKey = ConsoleKey::None;
	bool _raw = false;

	// used in raw mode (unix)
	int _eraseKey = -1;

	void resetCmdLine();

	static void onAllocBuffer(uv_handle_t *handle, size_t suggestedSize, uv_buf_t *buf);
	static void onRead(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);

	void doRead(ssize_t nread, const uv_buf_t *buf);
	void doReadNormal(ssize_t nread, const uv_buf_t *buf);
	void doReadRaw(ssize_t nread, const uv_buf_t *buf);

	inline void deleteChar();
	void deleteCmdLine();

public:
	bool init(uv_loop_t* loop, bool raw = true);
	void shutdown();

	/**
	 * Copy the input into the given buffer (if input is available)
	 *
	 * @return @c true if the cmdline contains valid input
	 */
	bool swap(char *buf, size_t size);
	ConsoleKey swapConsoleKey();

	void setCmdline(const char* buf, size_t size);

	/**
	 * Print the buffer to stdout
	 */
	void print(const char *buf, size_t size);
};

}
