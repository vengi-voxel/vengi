/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/Tokenizer.h"
#include "io/MemoryReadStream.h"
#include "io/Stream.h"

namespace io {

class TokenStream {
private:
	io::MemoryReadStream _memStream;
	io::SeekableReadStream &_stream;
	const core::TokenizerConfig _cfg;
	const char *_separator;

protected:
	/**
	 * @brief skip multiline and singleline comments
	 * @return @c true if a comment was skipped, @c false otherwise
	 */
	bool skipComments(uint8_t &c);
	bool isComment(uint8_t c);
	bool skipUntil(uint8_t &c, const char *end, core::String *content = nullptr);
	bool nextTokenChar(const core::String &token, uint8_t &c, bool skipWhitespace = true);
	bool isSeparator(char c) const;
	bool skipWhitespaces(uint8_t &c);

public:
	TokenStream(io::SeekableReadStream &stream, const core::TokenizerConfig &cfg = {},
				const char *separator = " (){};\n\t")
		: _memStream{nullptr, 0}, _stream(stream), _cfg(cfg), _separator(separator) {
	}
	TokenStream(const char *string, const core::TokenizerConfig &cfg = {}, const char *separator = " (){};\n\t")
		: _memStream{string, strlen(string)}, _stream(_memStream), _cfg(cfg), _separator(separator) {
	}

	bool eos() const {
		return _stream.eos();
	}

	core::String next();
};

} // namespace io
