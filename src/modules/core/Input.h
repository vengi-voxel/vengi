/**
 * @file
 */

#pragma once

#include <SDL_platform.h>

#if defined(__LINUX__) || defined(__MACOSX__)
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/select.h>
#endif

namespace core {

class Input {
private:
	char _input[256];
public:
	Input() {
#if defined(__LINUX__) || defined(__MACOSX__)
		::signal(SIGTTIN, SIG_IGN);
		::signal(SIGTTOU, SIG_IGN);
		::signal(SIGCONT, SIG_IGN);
#ifdef __LINUX__
		::fcntl(STDIN_FILENO, F_SETFL, ::fcntl(STDIN_FILENO, F_GETFL, 0) | O_NONBLOCK);
#endif
		::fcntl(STDIN_FILENO, F_SETFL, ::fcntl(STDIN_FILENO, F_GETFL, 0) | FNDELAY);
#endif
	}

	const char* read() {
#if defined(__LINUX__) || defined(__MACOSX__)
		fd_set fdset;
		FD_ZERO(&fdset);
		FD_SET(STDIN_FILENO, &fdset);

		struct timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;
		if (::select(STDIN_FILENO + 1, &fdset, nullptr, nullptr, &timeout) == -1 || !FD_ISSET(STDIN_FILENO, &fdset))
			return nullptr;

		const int len = ::read(STDIN_FILENO, _input, sizeof(_input));
		if (len <= 1)
			return nullptr;

		_input[len - 1] = '\0';
		return _input;
#else
		return nullptr;
#endif
	}
};

}
