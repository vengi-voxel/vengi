/**
 * @file
 */

#include "http/HttpCacheStream.h"
#include "app/App.h"
#include "app/tests/AbstractTest.h"
#include "http/Request.h"
#include "io/FilesystemArchive.h"
#include <gtest/gtest.h>

namespace http {

class HttpCacheStreamTest : public app::AbstractTest {};

// disabled because it requires network access
TEST_F(HttpCacheStreamTest, DISABLED_testGetRequest) {
	if (!Request::supported()) {
		GTEST_SKIP() << "No http support available";
	}

	const core::String filename = "testGetRequest.json";

	if (io::filesystem()->exists(filename)) {
		ASSERT_TRUE(io::Filesystem::sysRemoveFile(io::filesystem()->homeWritePath(filename)));
	}
	{
		http::HttpCacheStream stream(io::openFilesystemArchive(_testApp->filesystem()), filename,
									 "https://httpbin.org/get");
		ASSERT_TRUE(stream.valid());
		ASSERT_TRUE(stream.isNewInCache());
		ASSERT_GT(stream.size(), 0);
	}
	{
		http::HttpCacheStream stream(io::openFilesystemArchive(_testApp->filesystem()), filename,
									 "https://httpbin.org/get");
		ASSERT_TRUE(stream.valid());
		ASSERT_FALSE(stream.isNewInCache());
		ASSERT_GT(stream.size(), 0);
	}
}

} // namespace http
