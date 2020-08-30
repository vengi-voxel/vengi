/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "http/Url.h"

namespace http {

class UrlTest : public app::AbstractTest {
};

TEST_F(UrlTest, testSimple) {
	const Url url("http://www.myhost.de");
	EXPECT_EQ("http", url.schema);
	EXPECT_EQ("www.myhost.de", url.hostname);
	EXPECT_EQ(80, url.port);
	EXPECT_EQ("/", url.path);
}

TEST_F(UrlTest, testPort) {
	const Url url("http://www.myhost.de:8080");
	EXPECT_EQ("http", url.schema);
	EXPECT_EQ("www.myhost.de", url.hostname);
	EXPECT_EQ(8080, url.port);
	EXPECT_EQ("/", url.path);
}

TEST_F(UrlTest, testPath) {
	const Url url("http://www.myhost.de/path");
	EXPECT_EQ("http", url.schema);
	EXPECT_EQ("www.myhost.de", url.hostname);
	EXPECT_EQ(80, url.port);
	EXPECT_EQ("/path", url.path);
}

TEST_F(UrlTest, testPortPath) {
	const Url url("http://www.myhost.de:8080/path");
	EXPECT_EQ("http", url.schema);
	EXPECT_EQ("www.myhost.de", url.hostname);
	EXPECT_EQ(8080, url.port);
	EXPECT_EQ("/path", url.path);
}

TEST_F(UrlTest, testQuery) {
	const Url url("http://www.myhost.de/path?query=1");
	EXPECT_EQ("http", url.schema);
	EXPECT_EQ("www.myhost.de", url.hostname);
	EXPECT_EQ(80, url.port);
	EXPECT_EQ("query=1", url.query);
	EXPECT_EQ("/path", url.path);
}

TEST_F(UrlTest, testPortPathQuery) {
	const Url url("http://www.myhost.de:8080/path?query=1&second=2");
	EXPECT_EQ("http", url.schema);
	EXPECT_EQ("www.myhost.de", url.hostname);
	EXPECT_EQ(8080, url.port);
	EXPECT_EQ("/path", url.path);
	EXPECT_EQ("query=1&second=2", url.query);
}

TEST_F(UrlTest, testUserPassword) {
	const Url url("http://foo:bar@www.myhost.de");
	EXPECT_EQ("http", url.schema);
	EXPECT_EQ("www.myhost.de", url.hostname);
	EXPECT_EQ(80, url.port);
	EXPECT_EQ("/", url.path);
	EXPECT_EQ("foo", url.username);
	EXPECT_EQ("bar", url.password);
}

TEST_F(UrlTest, testPortPathQueryUsernamePassword) {
	const Url url("http://foo:bar@www.myhost.de:8080/path?query=1&second=2");
	EXPECT_EQ("http", url.schema);
	EXPECT_EQ("www.myhost.de", url.hostname);
	EXPECT_EQ(8080, url.port);
	EXPECT_EQ("/path", url.path);
	EXPECT_EQ("query=1&second=2", url.query);
	EXPECT_EQ("foo", url.username);
	EXPECT_EQ("bar", url.password);
}

}
