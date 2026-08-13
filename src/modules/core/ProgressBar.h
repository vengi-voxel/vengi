/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/IProgress.h"
#include "core/Trace.h"
#include "core/concurrent/Lock.h"
#include <stddef.h>

namespace core {

/**
 * @brief Terminal progress reporting for CLI tools.
 *
 * Writes to stderr so stdout stays free for machine-readable output (--json, etc.).
 * Implements @c IProgress with normalized values in [0, 1].
 */
class ProgressBar : public IProgress {
private:
	char _text[64] = "";
	int _lastPct = -1;
	bool _lineActive = false;
	mutable core_trace_mutex(core::Lock, _lock, "ProgressBar");

	void setTextUnlocked(const char *text);
	void setProgressUnlocked(float value, bool tty);

public:
	enum class Mode {
		Never, /**< No progress output (default) */
		Auto,  /**< Animated bar when stderr is a TTY; silent otherwise */
		Always /**< Force output (newline updates when not a TTY) */
	};

	static void setMode(Mode mode);
	static Mode mode();
	static bool isStderrTTY();

	void setProgress(float value) override;
	void setText(const char *text) override;

	/**
	 * @brief Format a single progress line into @p buf (NUL-terminated).
	 * @param progress01 Normalized progress in [0, 1]
	 * @return Bytes written excluding NUL, or -1 if @p buf is too small / invalid input
	 */
	static int format(char *buf, size_t buflen, const char *name, float progress01, int barWidth = 20);

	/**
	 * @brief Legacy helper: map discrete cur/max into setText + setProgress.
	 */
	void print(const char *name, int cur, int max);
};

} // namespace core
