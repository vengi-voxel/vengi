/**
 * @file
 */

#pragma once

#include "Stream.h"
#include <streambuf>

namespace io {

/**
 * @brief If you need an std::ostream, use this wrapper
 * @code
 * io::WriteStreamXX stream;
 * StdOStreamBuf buf(stream);
 * std::ostream stream(&buf);
 * @endcode
 */
class StdOStreamBuf : public std::streambuf {
private:
	io::WriteStream &_stream;

public:
	StdOStreamBuf(io::WriteStream &stream) : _stream(stream) {
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
		if (_stream.writeUInt8(c) != 1) {
			return traits_type::eof();
		}
		return c;
	}
};

class StdIStreamBuf : public std::streambuf {
private:
	io::ReadStream &_stream;
	char_type _char;

public:
	StdIStreamBuf(io::ReadStream &stream) : _stream(stream) {
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
			if (_stream.read(&_char, sizeof(_char)) == -1) {
				return traits_type::eof();
			}

			this->setg(&_char, &_char, &_char + 1);
		}
		return this->gptr() == this->egptr() ? traits_type::eof() : traits_type::to_int_type(*this->gptr());
	}
};

} // namespace io
