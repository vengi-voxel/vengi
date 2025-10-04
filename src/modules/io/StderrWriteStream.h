/**
 * @file
 */

#pragma once

#include "io/Stream.h"
#include <stdio.h>

namespace io {

class StderrWriteStream : public WriteStream {
public:
	~StderrWriteStream() override {
		flush();
	}

	int write(const void *buf, size_t size) override {
		return (int)fwrite(buf, 1, size, stderr);
	}

	bool flush() override {
		return fflush(stderr) == 0;
	}
};

} // namespace io
