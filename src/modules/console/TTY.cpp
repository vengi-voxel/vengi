/**
 * @file
 */

#include "TTY.h"
#include "core/Assert.h"
#include "core/Log.h"
#include <string.h>
#include <uv.h>
#ifndef _WIN32
#include <unistd.h>
#endif

namespace console {

namespace _priv {
static const int stdinFileHandle = 0;
static const int stdoutFileHandle = 1;
}

bool TTY::init(uv_loop_t* loop, bool raw) {
	_tty.data = this;
	if (uv_tty_init(loop, &_tty, _priv::stdinFileHandle, 1) != 0) {
		return false;
	}
	if (raw) {
#ifndef _WIN32
		_raw = true;
		uv_tty_set_mode(&_tty, UV_TTY_MODE_RAW);
		tcgetattr(_priv::stdinFileHandle, &_tty.orig_termios);
		_eraseKey = _tty.orig_termios.c_cc[VERASE];
#endif
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

void TTY::resetCmdLine() {
	_cmdline[0] = '\0';
	_cmdlineValid = false;
	_cmdlineSize = 0;
	_cmdlineKey = ConsoleKey::None;
}

void TTY::onRead(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
	if (nread < 0) {
		return uv_close(reinterpret_cast<uv_handle_t*>(stream), nullptr);
	}
	TTY* self = static_cast<TTY*>(stream->data);
	self->doRead(nread, buf);
}

void TTY::doRead(ssize_t nread, const uv_buf_t *buf) {
	if (_cmdlineValid || _cmdlineKey != ConsoleKey::None) {
		Log::warn("input wasn't fetched");
		resetCmdLine();
	}
	if (_raw) {
		doReadRaw(nread, buf);
	} else {
		doReadNormal(nread, buf);
	}
}

void TTY::doReadRaw(ssize_t nread, const uv_buf_t *buf) {
	if (nread == 3) { // vt100
		if (buf->base[1] == '[') {
			//core_assert(buf->base[1] == 27);
			switch (buf->base[2]) {
			case 'A':
				_cmdlineKey = ConsoleKey::CursorUp;
				break;
			case 'B':
				_cmdlineKey = ConsoleKey::CursorDown;
				break;
			case 'C':
				_cmdlineKey = ConsoleKey::CursorRight;
				break;
			case 'D':
				_cmdlineKey = ConsoleKey::CursorLeft;
				break;
			default:
				_cmdlineKey = ConsoleKey::None;
				break;
			}
		}
	} else if (nread == 1) {
		const char key = *buf->base;
		if (key == _eraseKey || key == 127 || key == 8) {
			if (_cmdlineSize > 0) {
				_cmdline[--_cmdlineSize] = '\0';
				deleteChar();
			}
		} else if (key == '\t') {
			_cmdlineKey = ConsoleKey::Tab;
		} else if (key == '\r' || key == '\n') {
			_cmdline[_cmdlineSize] = '\0';
			_cmdlineValid = true;
			print(buf->base, 1);
		} else if (key >= ' ' && key <= '~') {
			if (_cmdlineSize < sizeof(_cmdline) - 1) {
				_cmdline[_cmdlineSize++] = key;
				_cmdline[_cmdlineSize] = '\0';
				print(buf->base, 1);
			}
		}
	} else {
		Log::warn("Unhandled tty input in raw mode of length %i", (int)nread);
	}
}

void TTY::deleteChar() {
	print("\b \b", 3);
}

void TTY::deleteCmdLine() {
	for (size_t i = 0; i < _cmdlineSize; ++i) {
		deleteChar();
	}
	resetCmdLine();
}

void TTY::doReadNormal(ssize_t nread, const uv_buf_t *buf) {
	size_t len;
	for (len = 0u; len < buf->len; ++len) {
		if (buf->base[len] == '\0' || buf->base[len] == '\n' || buf->base[len] == '\r') {
			break;
		}
	}
	if (len == 0u || len == buf->len) {
		return;
	}
	strncpy(_cmdline, buf->base, len);
	const size_t end = core_min(len + 1, sizeof(_cmdline) - 1);
	_cmdline[end] = '\0';
	_cmdlineValid = true;
}

void TTY::setCmdline(const char* buf, size_t size) {
	deleteCmdLine();
	strncpy(_cmdline, buf, sizeof(_cmdline) - 1);
	_cmdline[sizeof(_cmdline) - 1] = '\0';
	_cmdlineSize = strlen(_cmdline);
	print(_cmdline, _cmdlineSize);
}

ConsoleKey TTY::swapConsoleKey() {
	const ConsoleKey old = _cmdlineKey;
	_cmdlineKey = ConsoleKey::None;
	return old;
}

bool TTY::swap(char *buf, size_t size) {
	const bool old = _cmdlineValid;
	strncpy(buf, _cmdline, size);
	buf[size - 1] = '\0';
	if (old) {
		resetCmdLine();
	}
	return old;
}

void TTY::print(const char *buf, size_t size) {
	for (size_t i = 0; i < size; ++i) {
		write(_priv::stdoutFileHandle, &buf[i], 1);
	}
}

}
