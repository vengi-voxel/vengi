/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "http/HttpHeader.h"

namespace http {

class HttpHeaderTest : public core::AbstractTest {
};

TEST_F(HttpHeaderTest, testSingle) {
	HeaderMap headers;
	headers.put("Foo", "Bar");
	char buf[1024];
	EXPECT_TRUE(buildHeaderBuffer(buf, sizeof(buf), headers));
	EXPECT_STREQ("Foo: Bar\r\n", buf);
}

TEST_F(HttpHeaderTest, testMultiple) {
	HeaderMap headers;
	headers.put("Foo", "Bar");
	headers.put("Foo1", "Bar1");
	headers.put("Foo2", "Bar2");
	char buf[1024];
	EXPECT_TRUE(buildHeaderBuffer(buf, sizeof(buf), headers));
	EXPECT_STREQ("Foo1: Bar1\r\n" "Foo2: Bar2\r\n" "Foo: Bar\r\n", buf);

}

}
