#pragma once
#include "io/Stream.h"
#include <istream>
#include <streambuf>
// MIT License
//
// Copyright (c) 2024 Ben McLean
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
namespace io {

class SeekableReadStreamBuf : public std::streambuf {
private:
	SeekableReadStream &_stream;
	static const int bufferSize = 4096;
	char _buffer[bufferSize];

public:
	explicit SeekableReadStreamBuf(SeekableReadStream &stream) : _stream(stream) {
		setg(_buffer, _buffer, _buffer);
	}

	explicit SeekableReadStreamBuf(SeekableReadStream *stream) : _stream(*stream) {
		setg(_buffer, _buffer, _buffer);
	}

protected:
	int_type underflow() override {
		if (gptr() < egptr()) {
			return traits_type::to_int_type(*gptr());
		}

		int bytesRead = _stream.read(_buffer, bufferSize);
		if (bytesRead <= 0) {
			return traits_type::eof();
		}

		setg(_buffer, _buffer, _buffer + bytesRead);
		return traits_type::to_int_type(*gptr());
	}

	pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which) override {
		if (!(which & std::ios_base::in)) {
			return pos_type(off_type(-1));
		}

		int whence;
		switch (dir) {
		case std::ios_base::beg:
			whence = SEEK_SET;
			break;
		case std::ios_base::cur:
			off -= (egptr() - gptr());
			whence = SEEK_CUR;
			break;
		case std::ios_base::end:
			whence = SEEK_END;
			break;
		default:
			return pos_type(off_type(-1));
		}

		int64_t newPos = _stream.seek(off, whence);
		if (newPos < 0) {
			return pos_type(off_type(-1));
		}

		setg(_buffer, _buffer, _buffer);
		return pos_type(newPos);
	}

	pos_type seekpos(pos_type pos, std::ios_base::openmode which) override {
		return seekoff(off_type(pos), std::ios_base::beg, which);
	}
};

class SeekableReadStreamAdapter : public std::istream {
private:
	SeekableReadStreamBuf _buf;

public:
	explicit SeekableReadStreamAdapter(SeekableReadStream &stream) : std::istream(nullptr), _buf(stream) {
		rdbuf(&_buf);
	}

	explicit SeekableReadStreamAdapter(SeekableReadStream *stream) : std::istream(nullptr), _buf(stream) {
		rdbuf(&_buf);
	}
};

} // namespace io
