/**
 * @file
 */

#include "Base64Stream.h"
#include "io/Stream.h"

namespace io {

class Base64WriteStream : public io::WriteStream, public Base64Stream {
protected:
	io::WriteStream &_stream;
	int _bytes = 0;
	uint8_t _buf[4];

	static CORE_FORCE_INLINE bool encode(const uint8_t src[3], io::WriteStream &out, int bytes) {
		uint8_t dst[4];
		dst[0] = (src[0] & 0xfc) >> 2;
		dst[1] = ((src[0] & 0x03) << 4) + ((src[1] & 0xf0) >> 4);
		dst[2] = ((src[1] & 0x0f) << 2) + ((src[2] & 0xc0) >> 6);
		dst[3] = src[2] & 0x3f;

		for (int i = 0; i < bytes; ++i) {
			if (!out.writeUInt8(LUT[dst[i]])) {
				return false;
			}
		}
		return true;
	}
public:
	Base64WriteStream(io::WriteStream &stream);
	~Base64WriteStream() override;

	int write(const void *buf, size_t size) override;
	bool flush() override;
};

} // namespace io
