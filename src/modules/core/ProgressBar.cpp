/**
 * @file
 */

#include "core/ProgressBar.h"
#include "core/StringUtil.h"
#include "core/concurrent/Lock.h"
#include <SDL3/SDL_stdinc.h>
#include <stdio.h>

#if defined(_WIN32) || defined(__CYGWIN__)
#include <io.h>
#include <stdio.h>
#else
#include <unistd.h>
#endif

namespace core {

namespace {
ProgressBar::Mode g_mode = ProgressBar::Mode::Never;

bool shouldPrint(bool tty) {
	if (g_mode == ProgressBar::Mode::Never) {
		return false;
	}
	if (g_mode == ProgressBar::Mode::Always) {
		return true;
	}
	return tty;
}
} // namespace

void ProgressBar::setMode(Mode mode) {
	g_mode = mode;
}

ProgressBar::Mode ProgressBar::mode() {
	return g_mode;
}

bool ProgressBar::isStderrTTY() {
#if defined(_WIN32) || defined(__CYGWIN__)
	return _isatty(_fileno(stderr)) != 0;
#elif defined(__EMSCRIPTEN__)
	return false;
#else
	return isatty(fileno(stderr)) != 0;
#endif
}

void ProgressBar::setTextUnlocked(const char *text) {
	if (text == nullptr) {
		_text[0] = '\0';
	} else if (SDL_strcmp(_text, text) != 0) {
		core::string::strncpyz(text, SDL_strlen(text), _text, sizeof(_text));
		// Allow the same percent to print again under a new label (e.g. mesh_6 -> mesh_7).
		_lastPct = -1;
	}
}

void ProgressBar::setProgressUnlocked(float value, bool tty) {
	value = clamp01(value);
	const int pct = (int)(value * 100.0f + 0.5f);

	// Completing a range often cascades several setProgress(1) calls (nested ranges +
	// load/save wrappers). Keep _lastPct at 100 so identical completions are silent.
	if (pct == _lastPct) {
		return;
	}

	const char *name = _text[0] != '\0' ? _text : "progress";
	char line[256];
	if (format(line, sizeof(line), name, value) < 0) {
		SDL_snprintf(line, sizeof(line), "%3d%% %s", pct, name);
	}

	if (tty) {
		fprintf(stderr, "\r%-79s", line);
		if (value >= 1.0f) {
			fputc('\n', stderr);
			_lineActive = false;
		} else {
			_lineActive = true;
		}
		fflush(stderr);
	} else {
		fprintf(stderr, "%s\n", line);
		fflush(stderr);
	}
	_lastPct = pct;
}

void ProgressBar::setProgress(float value) {
	const bool tty = isStderrTTY();
	if (!shouldPrint(tty)) {
		return;
	}
	ScopedLock lock(_lock);
	setProgressUnlocked(value, tty);
}

void ProgressBar::setText(const char *text) {
	ScopedLock lock(_lock);
	setTextUnlocked(text);
}

int ProgressBar::format(char *buf, size_t buflen, const char *name, float progress01, int barWidth) {
	if (buf == nullptr || buflen == 0 || name == nullptr || barWidth <= 0) {
		return -1;
	}
	progress01 = clamp01(progress01);
	const int pct = (int)(progress01 * 100.0f + 0.5f);
	int filled = (int)(progress01 * (float)barWidth + 0.5f);
	if (filled > barWidth) {
		filled = barWidth;
	}

	size_t needed = (size_t)barWidth + 16 + SDL_strlen(name);
	if (buflen < needed) {
		return -1;
	}

	char *p = buf;
	*p++ = '[';
	for (int i = 0; i < barWidth; ++i) {
		*p++ = (i < filled) ? '#' : '-';
	}
	*p++ = ']';
	*p++ = ' ';
	SDL_snprintf(p, buflen - (size_t)(p - buf), "%3d%% %s", pct, name);
	return (int)SDL_strlen(buf);
}

void ProgressBar::print(const char *name, int cur, int max) {
	if (name == nullptr || max <= 0) {
		return;
	}
	if (cur < 0) {
		cur = 0;
	}
	if (cur > max) {
		cur = max;
	}
	const bool tty = isStderrTTY();
	if (!shouldPrint(tty)) {
		return;
	}
	ScopedLock lock(_lock);
	setTextUnlocked(name);
	setProgressUnlocked((float)cur / (float)max, tty);
}

} // namespace core
