/**
 * @file
 */

#include "http/Request.h"
#include "app/tests/AbstractTest.h"
#include "engine-config.h"
#include "io/BufferedReadWriteStream.h"
#include <gtest/gtest.h>
#include <json.hpp>

namespace http {

class RequestTest : public app::AbstractTest {};

TEST_F(RequestTest, DISABLED_testGetRequest) {
	if (!Request::supported()) {
		GTEST_SKIP() << "No http support available";
	}
	io::BufferedReadWriteStream stream;
	Request request("https://httpbin.org/get", http::RequestType::GET);
	const core::String userAgent = app::App::getInstance()->fullAppname() + "/" PROJECT_VERSION;
	request.setUserAgent(userAgent);
	ASSERT_TRUE(request.execute(stream));
	ASSERT_NE(0, stream.size());
	stream.seek(0);
	core::String response;
	stream.readString((int)stream.size(), response);
	nlohmann::json jsonResponse = nlohmann::json::parse(response, nullptr, false, true);
	ASSERT_TRUE(jsonResponse.contains("headers")) << response;
	ASSERT_FALSE(jsonResponse["headers"].contains("Content-Length")) << response;
	ASSERT_FALSE(jsonResponse["headers"].contains("Content-Type")) << response;
	ASSERT_TRUE(jsonResponse["headers"].contains("User-Agent")) << response;
	EXPECT_STREQ(userAgent.c_str(), jsonResponse["headers"]["User-Agent"].get<std::string>().c_str()) << response;
}

TEST_F(RequestTest, DISABLED_testPostRequest) {
	if (!Request::supported()) {
		GTEST_SKIP() << "No http support available";
	}
	io::BufferedReadWriteStream stream;
	Request request("https://httpbin.org/post", http::RequestType::POST);
	const core::String userAgent = app::App::getInstance()->fullAppname() + "/" PROJECT_VERSION;
	request.setUserAgent(userAgent);
	request.addHeader("Content-Type", "application/json");
	request.setBody("{}");
	ASSERT_TRUE(request.execute(stream));
	ASSERT_NE(0, stream.size());
	stream.seek(0);
	core::String response;
	stream.readString((int)stream.size(), response);
	nlohmann::json jsonResponse = nlohmann::json::parse(response, nullptr, false, true);
	ASSERT_TRUE(jsonResponse.contains("headers")) << response;
	ASSERT_TRUE(jsonResponse["headers"].contains("User-Agent")) << response;
	EXPECT_STREQ(userAgent.c_str(), jsonResponse["headers"]["User-Agent"].get<std::string>().c_str()) << response;
	ASSERT_TRUE(jsonResponse["headers"].contains("Content-Type")) << response;
	EXPECT_EQ("application/json", jsonResponse["headers"]["Content-Type"].get<std::string>()) << response;
	ASSERT_TRUE(jsonResponse["headers"].contains("Content-Length")) << response;
	EXPECT_EQ("2", jsonResponse["headers"]["Content-Length"].get<std::string>()) << response;
}

} // namespace http
