/**
 * @file
 */

#include "Pipe.h"
#include "app/I18N.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "io/Stream.h"
#include <stdlib.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "io/Filesystem.h"
#endif

namespace app {

Pipe::Pipe() {
#if defined(_WIN32)
	_pipe = INVALID_HANDLE_VALUE;
#endif
}

Pipe::~Pipe() {
	shutdown();
}

void Pipe::construct() {
	core::VarDef varAppPipe(cfg::AppPipe, false, 0u, N_("Enable named pipe for input commands"));
	_corePipe = core::Var::registerVar(varAppPipe);
}

bool Pipe::init() {
	if (!_corePipe->boolVal()) {
		return true;
	}

	const core::String &fullAppname = app::App::getInstance()->fullAppname();
	const core::String pipeName = fullAppname + "-input";

#if defined(_WIN32)
	_pipeName = "\\\\.\\pipe\\" + pipeName;

	_pipe = CreateNamedPipeA(_pipeName.c_str(), PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
							 PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, 1024, 1024, 0, nullptr);

	if (_pipe == INVALID_HANDLE_VALUE) {
		Log::error("Failed to create named pipe %s", _pipeName.c_str());
		return false;
	}

	_connectEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	if (!_connectEvent) {
		CloseHandle(_pipe);
		_pipe = INVALID_HANDLE_VALUE;
		return false;
	}

	OVERLAPPED ov = {};
	ov.hEvent = (HANDLE)_connectEvent;

	BOOL ok = ConnectNamedPipe(_pipe, &ov);
	if (!ok) {
		DWORD err = GetLastError();
		if (err == ERROR_IO_PENDING) {
			_connected = false; // expected, non-blocking
		} else if (err == ERROR_PIPE_CONNECTED) {
			_connected = true;
			SetEvent((HANDLE)_connectEvent);
		} else {
			Log::error("ConnectNamedPipe failed (%lu)", err);
			return false;
		}
	}

#else
	const core::String &homeDir = app::App::getInstance()->filesystem()->homePath();
	_pipeName = core::string::path(homeDir, "." + pipeName);

	if (mkfifo(_pipeName.c_str(), 0666) == -1) {
		if (errno != EEXIST) {
			Log::error("Failed to create named pipe %s: %s", _pipeName.c_str(), strerror(errno));
			return false;
		}
	}

	_pipe = open(_pipeName.c_str(), O_RDWR | O_NONBLOCK);
	if (_pipe == -1) {
		Log::error("Failed to open named pipe %s: %s", _pipeName.c_str(), strerror(errno));
		return false;
	}
#endif

	Log::info("Opened pipe %s", _pipeName.c_str());
	return true;
}

void Pipe::shutdown() {
#if defined(_WIN32)
	if (_pipe != INVALID_HANDLE_VALUE) {
		CloseHandle((HANDLE)_pipe);
		_pipe = INVALID_HANDLE_VALUE;
	}
	if (_connectEvent) {
		CloseHandle((HANDLE)_connectEvent);
		_connectEvent = nullptr;
	}
	_connected = false;
#else
	if (_pipe != -1) {
		close(_pipe);
		_pipe = -1;
	}
	if (!_pipeName.empty()) {
		unlink(_pipeName.c_str());
	}
#endif
}

int Pipe::read(io::WriteStream &stream) {
#if defined(_WIN32)
	if (_pipe == INVALID_HANDLE_VALUE) {
		return 0;
	}

	// Poll for connection (non-blocking)
	if (!_connected) {
		DWORD res = WaitForSingleObject((HANDLE)_connectEvent, 0);
		if (res != WAIT_OBJECT_0) {
			return 0;
		}
		_connected = true;
	}

	DWORD bytesAvailable = 0;
	if (!PeekNamedPipe(_pipe, nullptr, 0, nullptr, &bytesAvailable, nullptr)) {
		if (GetLastError() == ERROR_BROKEN_PIPE) {
			// Client disconnected - reset
			DisconnectNamedPipe(_pipe);
			ResetEvent((HANDLE)_connectEvent);

			OVERLAPPED ov = {};
			ov.hEvent = (HANDLE)_connectEvent;
			ConnectNamedPipe(_pipe, &ov);

			_connected = false;
			return 0;
		}
		return -1;
	}

	if (bytesAvailable == 0) {
		return 0;
	}

	char buffer[1024];
	DWORD bytesRead = 0;
	if (!ReadFile(_pipe, buffer, sizeof(buffer), &bytesRead, nullptr)) {
		if (GetLastError() == ERROR_NO_DATA) {
			return 0;
		}
		return -1;
	}

	if (bytesRead > 0) {
		if (stream.write(buffer, bytesRead) == -1) {
			return -1;
		}
		return (int)bytesRead;
	}

#else
	if (_pipe == -1) {
		return 0;
	}

	char buffer[1024];
	ssize_t bytes = ::read(_pipe, buffer, sizeof(buffer));
	if (bytes == -1) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return 0;
		}
		return -1;
	}
	if (bytes > 0) {
		if (stream.write(buffer, bytes) == -1) {
			return -1;
		}
		return (int)bytes;
	}
#endif

	return 0;
}

} // namespace app
