/**
 * @file
 */

#include "io/Base64Stream.h"
#include "io/Stream.h"

namespace io {

/**
 * @ingroup IO
 */
class Base64ReadStream : public io::ReadStream, public Base64Stream {
protected:
	io::ReadStream &_stream;
	int _readBufSize = 0;
	uint8_t _readBuf[3];
	int _readBufPos = 0;

	// Constexpr reverse lookup table - computes at compile time
	static constexpr int INVALID_CHAR = -1;

	static CORE_FORCE_INLINE constexpr int reverseLookup(uint8_t c) {
		return (c >= 'A' && c <= 'Z') ? (c - 'A') :
		       (c >= 'a' && c <= 'z') ? (c - 'a' + 26) :
		       (c >= '0' && c <= '9') ? (c - '0' + 52) :
		       (c == '+') ? 62 :
		       (c == '/') ? 63 :
		       INVALID_CHAR;
	}

	static CORE_FORCE_INLINE void decode(const uint8_t src[4], uint8_t dest[3]) {
		dest[0] = (src[0] << 2) | (src[1] >> 4);
		dest[1] = (src[1] << 4) | (src[2] >> 2);
		dest[2] = (src[2] << 6) | src[3];
	}

public:
	Base64ReadStream(io::ReadStream &stream);
	~Base64ReadStream() override = default;

	int read(void *buf, size_t size) override;
	bool eos() const override;
};

} // namespace io
