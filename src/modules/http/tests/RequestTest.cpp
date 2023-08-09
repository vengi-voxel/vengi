/**
 * @file
 */

#include "http/Request.h"
#include "app/tests/AbstractTest.h"
#include "io/BufferedReadWriteStream.h"
#include <gtest/gtest.h>

namespace http {

class RequestTest : public app::AbstractTest {};

TEST_F(RequestTest, testRequest) {
	if (!Request::supported()) {
		GTEST_SKIP() << "No http support available";
	}
	io::BufferedReadWriteStream stream;
	Request request;
	ASSERT_TRUE(request.request("https://www.google.com", stream));
	// stream.seek(0);
	// core::String response;
	// stream.readString((int)stream.size(), response);
	ASSERT_NE(0, stream.size());
}

} // namespace http
