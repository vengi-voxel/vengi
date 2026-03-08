/**
 * @file
 */

#pragma once

#include "Stream.h"
#include <string.h>
#include <streambuf>

namespace io {

/**
 * @brief If you need an std::ostream, use this wrapper
 * @code
 * io::WriteStreamXX stream;
 * StdOStreamBuf buf(stream);
 * std::ostream stream(&buf);
 * @endcode
 * @ingroup IO
 */
class StdOStreamBuf : public std::streambuf {
private:
	static constexpr size_t BufSize = 4096;
	io::WriteStream &_stream;
	char_type _buf[BufSize];

	bool flushBuffer() {
		const int n = (int)(pptr() - pbase());
		if (n > 0 && _stream.write(pbase(), n) != n) {
			return false;
		}
		setp(_buf, _buf + BufSize);
		return true;
	}

public:
	StdOStreamBuf(io::WriteStream &stream) : _stream(stream) {
		setp(_buf, _buf + BufSize);
	}

	~StdOStreamBuf() override {
		flushBuffer();
	}

	/**
	 *  @brief  Consumes data from the buffer; writes to the
	 *          controlled sequence.
	 *  @param  c  An additional character to consume.
	 *  @return  eof() to indicate failure, something else (usually
	 *           @a c, or not_eof())
	 *
	 *  Informally, this function is called when the output buffer
	 *  is full (or does not exist, as buffering need not actually
	 *  be done).  If a buffer exists, it is @a consumed, with
	 *  <em>some effect</em> on the controlled sequence.
	 *  (Typically, the buffer is written out to the sequence
	 *  verbatim.)  In either case, the character @a c is also
	 *  written out, if @a c is not @c eof().
	 *
	 *  For a formal definition of this function, see a good text
	 *  such as Langer & Kreft, or [27.5.2.4.5]/3-7.
	 *
	 *  A functioning output streambuf can be created by overriding only
	 *  this function (no buffer area will be used).
	 *
	 *  @note  Base class version does nothing, returns eof().
	 */
	int_type overflow(int_type c) override {
		if (!flushBuffer()) {
			return traits_type::eof();
		}
		if (!traits_type::eq_int_type(c, traits_type::eof())) {
			*pptr() = traits_type::to_char_type(c);
			pbump(1);
		}
		return c;
	}

	int sync() override {
		return flushBuffer() ? 0 : -1;
	}

	std::streamsize xsputn(const char_type *s, std::streamsize count) override {
		std::streamsize written = 0;
		while (count > 0) {
			const std::streamsize avail = epptr() - pptr();
			if (count <= avail) {
				memcpy(pptr(), s, (size_t)count);
				pbump((int)count);
				written += count;
				break;
			}
			memcpy(pptr(), s, (size_t)avail);
			pbump((int)avail);
			s += avail;
			count -= avail;
			written += avail;
			if (!flushBuffer()) {
				break;
			}
		}
		return written;
	}
};

class StdIStreamBuf : public std::streambuf {
private:
	static constexpr size_t BufSize = 4096;
	io::ReadStream &_stream;
	char_type _buf[BufSize];

public:
	StdIStreamBuf(io::ReadStream &stream) : _stream(stream) {
		setg(_buf, _buf, _buf);
	}

	/**
	 *  @brief  Fetches more data from the controlled sequence.
	 *  @return  The first character from the <em>pending sequence</em>.
	 *
	 *  Informally, this function is called when the input buffer is
	 *  exhausted (or does not exist, as buffering need not actually be
	 *  done).  If a buffer exists, it is @a refilled.  In either case, the
	 *  next available character is returned, or @c traits::eof() to
	 *  indicate a null pending sequence.
	 *
	 *  For a formal definition of the pending sequence, see a good text
	 *  such as Langer & Kreft, or [27.5.2.4.3]/7-14.
	 *
	 *  A functioning input streambuf can be created by overriding only
	 *  this function (no buffer area will be used).  For an example, see
	 *  https://gcc.gnu.org/onlinedocs/libstdc++/manual/streambufs.html
	 *
	 *  @note  Base class version does nothing, returns eof().
	 */
	int_type underflow() override {
		if (gptr() == egptr()) {
			if (_stream.eos()) {
				return traits_type::eof();
			}
			const int n = _stream.read(_buf, BufSize);
			if (n <= 0) {
				return traits_type::eof();
			}
			setg(_buf, _buf, _buf + n);
		}
		return traits_type::to_int_type(*gptr());
	}

	std::streamsize xsgetn(char_type *s, std::streamsize count) override {
		std::streamsize read = 0;
		while (count > 0) {
			const std::streamsize avail = egptr() - gptr();
			if (avail > 0) {
				const std::streamsize n = avail < count ? avail : count;
				memcpy(s, gptr(), (size_t)n);
				gbump((int)n);
				s += n;
				count -= n;
				read += n;
			} else {
				if (underflow() == traits_type::eof()) {
					break;
				}
			}
		}
		return read;
	}
};

} // namespace io
