/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "io/Stream.h"

namespace io {

class TokenStream {
private:
	io::ReadStream &_stream;

public:
	TokenStream(io::ReadStream &stream) : _stream(stream) {
	}

	bool eos() const {
		return _stream.eos();
	}

	core::String next();
};

} // namespace io
