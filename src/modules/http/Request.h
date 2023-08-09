/**
 * @file
 */

#include "io/Stream.h"
namespace http {

class Request {
public:
	static bool supported();
	bool request(const core::String &url, io::WriteStream &stream);
};

} // namespace http
