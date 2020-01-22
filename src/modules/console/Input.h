/**
 * @file
 */

#pragma once

#include <uv.h>

namespace console {

/**
 * @brief Non-blocking console input reading e.g. for dedicated server command line
 */
class Input {
private:
	char _input[256];
	uv_tty_t _tty;

	static void onAllocBuffer(uv_handle_t *handle, size_t suggestedSize, uv_buf_t *buf);
	static void onRead(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);

public:
	bool init(uv_loop_t* loop);
	void shutdown();
};

}
