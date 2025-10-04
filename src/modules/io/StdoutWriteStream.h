/**
 * @file
 */

#pragma once

#include "io/Stream.h"
#include <stdio.h>

namespace io {

class StdoutWriteStream : public WriteStream {
public:
	~StdoutWriteStream() override {
		flush();
	}

	int write(const void *buf, size_t size) override {
		return (int)fwrite(buf, 1, size, stdout);
	}

	bool flush() override {
		return fflush(stdout) == 0;
	}
};

} // namespace io
