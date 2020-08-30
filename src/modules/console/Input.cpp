/**
 * @file
 */

#include "Input.h"
#include "core/Log.h"
#include "command/CommandHandler.h"
#include <string.h>

namespace console {

bool Input::init(uv_loop_t* loop) {
	_tty.data = this;
	int ttyFileDescriptor = 0;
	if (uv_tty_init(loop, &_tty, ttyFileDescriptor, 1) != 0) {
		return false;
	}
	uv_stream_t* stream = reinterpret_cast<uv_stream_t*>(&_tty);
	if (uv_is_readable(stream)) {
		uv_read_start(stream, onAllocBuffer, onRead);
		return false;
	}

	return true;
}

void Input::shutdown() {
	uv_stream_t* stream = reinterpret_cast<uv_stream_t*>(&_tty);
	uv_read_stop(stream);
	uv_close((uv_handle_t*)stream, nullptr);
}

void Input::onAllocBuffer(uv_handle_t *handle, size_t suggestedSize, uv_buf_t *buf) {
	Input* self = static_cast<Input*>(handle->data);
	buf->len = sizeof(self->_input);
	buf->base = self->_input;
}

void Input::onRead(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
	if (nread < 0) {
		return uv_close(reinterpret_cast<uv_handle_t*>(stream), nullptr);
	}
	size_t len;
	for (len = 0u; len < buf->len; ++len) {
		if (buf->base[len] == '\0' || buf->base[len] == '\n' || buf->base[len] == '\r') {
			break;
		}
	}
	if (len == 0u || len == buf->len) {
		return;
	}
	const core::String commandLine(buf->base, len);
	command::executeCommands(commandLine);
}

}
