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
 * StdStreamBuf buf(stream);
 * std::ostream stream(&buf);
 * @endcode
 */
class StdStreamBuf : public std::streambuf {
private:
	io::WriteStream &_stream;

public:
	StdStreamBuf(io::WriteStream &stream) : _stream(stream) {
	}

	/**
	 *  @brief  Consumes data from the buffer; writes to the
	 *          controlled sequence.
	 *  @param  __c  An additional character to consume.
	 *  @return  eof() to indicate failure, something else (usually
	 *           @a __c, or not_eof())
	 *
	 *  Informally, this function is called when the output buffer
	 *  is full (or does not exist, as buffering need not actually
	 *  be done).  If a buffer exists, it is @a consumed, with
	 *  <em>some effect</em> on the controlled sequence.
	 *  (Typically, the buffer is written out to the sequence
	 *  verbatim.)  In either case, the character @a c is also
	 *  written out, if @a __c is not @c eof().
	 *
	 *  For a formal definition of this function, see a good text
	 *  such as Langer & Kreft, or [27.5.2.4.5]/3-7.
	 *
	 *  A functioning output streambuf can be created by overriding only
	 *  this function (no buffer area will be used).
	 *
	 *  @note  Base class version does nothing, returns eof().
	 */
	virtual int_type overflow(int_type c) {
		if (_stream.write(&c, sizeof(c)) != sizeof(c)) {
			return traits_type::eof();
		}
		return c;
	}
};

} // namespace io