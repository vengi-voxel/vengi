/**
 * @file
 */

#include "TTY.h"
#include "core/Log.h"
#include "command/CommandHandler.h"
#include <string.h>

namespace console {

bool TTY::init(uv_loop_t* loop) {
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

void TTY::shutdown() {
	uv_stream_t* stream = reinterpret_cast<uv_stream_t*>(&_tty);
	uv_read_stop(stream);
	uv_close((uv_handle_t*)stream, nullptr);
}

void TTY::onAllocBuffer(uv_handle_t *handle, size_t suggestedSize, uv_buf_t *buf) {
	TTY* self = static_cast<TTY*>(handle->data);
	buf->len = sizeof(self->_input);
	buf->base = self->_input;
}

void TTY::onRead(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
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
