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
	int _reverseLookupTable[256];

	static CORE_FORCE_INLINE bool decode(uint8_t src[4], uint8_t dest[3]) {
		dest[0] = (src[0] << 2) + ((src[1] & 0x30) >> 4);
		dest[1] = ((src[1] & 0xf) << 4) + ((src[2] & 0x3c) >> 2);
		dest[2] = ((src[2] & 0x3) << 6) + src[3];
		return true;
	}

public:
	Base64ReadStream(io::ReadStream &stream);
	~Base64ReadStream() override = default;

	int read(void *buf, size_t size) override;
	bool eos() const override;
};

} // namespace io
