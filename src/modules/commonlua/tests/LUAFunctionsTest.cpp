/**
 * @file
 */

#include "commonlua/LUAFunctions.h"
#include "app/tests/AbstractTest.h"
#include "http/Client.h"
#include <glm/vector_relational.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/string_cast.hpp>

namespace lua {

class LUAFunctionsTest : public app::AbstractTest {
public:
	template<typename T>
	void test(const char *script, const T& expected) {
		LUA lua;
		clua_mathregister(lua);
		clua_streamregister(lua);
		clua_httpregister(lua);
		ASSERT_TRUE(lua.load(script)) << lua.error();
		ASSERT_TRUE(lua.execute("test", 1)) << lua.error();
		const T& v = clua_tovec<T>(lua, -1);
		EXPECT_TRUE(glm::all(glm::equal(expected, v))) << glm::to_string(v) << " vs " << glm::to_string(expected);
	}

	void testExec(const char *script) {
		LUA lua;
		clua_mathregister(lua);
		clua_streamregister(lua);
		clua_httpregister(lua);
		ASSERT_TRUE(lua.load(script)) << lua.error();
		ASSERT_TRUE(lua.execute("test", 1)) << lua.error();
	}
};

TEST_F(LUAFunctionsTest, testVectorCtor) {
	const char *script = R"(
		function test()
			return g_ivec3.new(0, 1, 0)
		end
	)";
	test(script, glm::ivec3(0, 1, 0));
}

TEST_F(LUAFunctionsTest, testAsyncHttpFailureAndAbandonedRequest) {
	if (!http::Request::supported()) {
		GTEST_SKIP();
	}
	LUA lua;
	clua_streamregister(lua);
	clua_httpregister(lua);
	ASSERT_TRUE(lua.load(R"(
		request = g_http.getAsync("http://127.0.0.1:1/")
		assert(not request:ready())
		assert(request:response() == nil)
		local abandoned = g_http.postAsync("http://127.0.0.1:1/", "body")
		abandoned = nil
		collectgarbage("collect")
		function ready() return request:ready() end
		function response()
			local stream, headers, status, success = request:response()
			assert(stream and type(headers) == "table")
			assert(status == 0 and not success)
		end
	)")) << lua.error();
	bool ready = false;
	for (int i = 0; i < 2000 && !ready; ++i) {
		http::update();
		ASSERT_TRUE(lua.execute("ready", 1));
		ready = lua_toboolean(lua, -1);
		lua.pop();
		wait(5);
	}
	ASSERT_TRUE(ready);
	ASSERT_TRUE(lua.execute("response")) << lua.error();
}

TEST_F(LUAFunctionsTest, testClosingLuaCancelsHttp) {
	{
		LUA lua;
		clua_httpregister(lua);
		ASSERT_TRUE(lua.load(R"(request = g_http.getAsync("http://127.0.0.1:1/"))"));
	}
	// Closing the state collects the request. No callback may access its userdata.
	for (int i = 0; i < 10; ++i) {
		http::update();
		wait(2);
	}
}

TEST_F(LUAFunctionsTest, testAsyncHttpBinaryResponse) {
	const char *url = SDL_getenv("VENGI_HTTP_TEST_URL");
	if (!url || !http::Request::supported()) {
		GTEST_SKIP();
	}
	LUA lua;
	clua_httpregister(lua);
	clua_streamregister(lua);
	lua_pushstring(lua, url);
	lua_setglobal(lua, "url");
	ASSERT_TRUE(lua.load(R"(
		request = g_http.postAsync(url, "a\0b" .. string.char(255), {["X-Test"] = "lua-http"})
		function ready() return request:ready() end
		function response()
			local stream, headers, status, success = request:response()
			assert(success and status == 404)
			assert(headers["X-Test"] == "lua-http")
			assert(stream:readUInt8() == 97)
			assert(stream:readUInt8() == 0)
			assert(stream:readUInt8() == 98)
			assert(stream:readUInt8() == 255)
		end
	)")) << lua.error();
	bool ready = false;
	for (int i = 0; i < 2000 && !ready; ++i) {
		http::update();
		ASSERT_TRUE(lua.execute("ready", 1));
		ready = lua_toboolean(lua, -1);
		lua.pop();
		wait(5);
	}
	ASSERT_TRUE(ready);
	ASSERT_TRUE(lua.execute("response")) << lua.error();
}

// disabled because it requires network access
TEST_F(LUAFunctionsTest, DISABLED_testHTTPGet) {
	testExec(R"(
		function test()
			local str = g_http.get("https://httpbin.org/get"):readString()
			print(str)
		end
	)");
}

// disabled because it requires network access
TEST_F(LUAFunctionsTest, DISABLED_testHTTPGetHeaders) {
	testExec(R"(
		function test()
			local headers = {}
			headers["User-Agent"] = "Mozilla/5.0"
			local stream, responseHeaders = g_http.get("https://httpbin.org/get")
			local str = stream:readString()
			print(str)
			for k, v in pairs(responseHeaders) do
				print("key: " .. k .. ", value: " .. v)
			end
		end
	)");
}

TEST_F(LUAFunctionsTest, testVectorDistance) {
	const char *script = R"(
		function test()
			local v1 = g_vec3.new(0, 1, 0)
			local v2 = g_vec3.new(0, 2, 0)
			return v1:distance(v2)
		end
	)";
	LUA lua;
	clua_vecregister<glm::vec3>(lua.state());
	ASSERT_TRUE(lua.load(script)) << lua.error();
	ASSERT_TRUE(lua.execute("test", 1)) << lua.error();
	const float distance = lua_tonumber(lua, -1);
	ASSERT_FLOAT_EQ(1.0f, distance);
}

TEST_F(LUAFunctionsTest, testVectorDistanceGlobal) {
	const char *script = R"(
		function test()
			local v1 = g_vec3.new(0, 1, 0)
			local v2 = g_vec3.new(0, 2, 0)
			return g_vec3.distance(v1, v2)
		end
	)";
	LUA lua;
	clua_vecregister<glm::vec3>(lua.state());
	ASSERT_TRUE(lua.load(script)) << lua.error();
	ASSERT_TRUE(lua.execute("test", 1)) << lua.error();
	const float distance = lua_tonumber(lua, -1);
	ASSERT_FLOAT_EQ(1.0f, distance);
}

TEST_F(LUAFunctionsTest, testVectorAddition) {
	const char *script = R"(
		function test()
			return g_ivec3.new(0, 1, 0) + g_ivec3.new(0, 1, 1)
		end
	)";
	test(script, glm::ivec3(0, 2, 1));
}

TEST_F(LUAFunctionsTest, testVectorAdditionNonVector) {
	const char *script = R"(
		function test()
			return g_ivec3.new(0, 1, 0) + 1
		end
	)";
	test(script, glm::ivec3(1, 2, 1));
}

TEST_F(LUAFunctionsTest, testQuaternionNew) {
	const char *script = R"(
		function test()
			return g_quat.new(0, 0, 0, 1)
		end
	)";
	const glm::quat expected = glm::quat::wxyz(1.0f, 0.0f, 0.0f, 0.0f);
	LUA lua;
	clua_mathregister(lua);
	ASSERT_TRUE(lua.load(script)) << lua.error();
	ASSERT_TRUE(lua.execute("test", 1)) << lua.error();
	const glm::quat& v = clua_toquat(lua, -1);
	EXPECT_TRUE(glm::all(glm::equal(expected, v))) << glm::to_string(v) << " vs " << glm::to_string(expected);
}

TEST_F(LUAFunctionsTest, testQuaternionRotateX) {
	const char *script = R"(
		function test()
			return g_quat.rotateX(0)
		end
	)";
	const glm::quat expected = glm::quat::wxyz(1.0f, 0.0f, 0.0f, 0.0f);
	LUA lua;
	clua_mathregister(lua);
	ASSERT_TRUE(lua.load(script)) << lua.error();
	ASSERT_TRUE(lua.execute("test", 1)) << lua.error();
	const glm::quat& v = clua_toquat(lua, -1);
	EXPECT_TRUE(glm::all(glm::equal(expected, v))) << glm::to_string(v) << " vs " << glm::to_string(expected);
}

TEST_F(LUAFunctionsTest, testQuatMultiplication) {
	const char *script = R"(
		function test()
			return g_quat.new(0, 0, 0, 1) * g_quat.new(0, 0, 0, 1)
		end
	)";
	testExec(script);
}

TEST_F(LUAFunctionsTest, testVectorMultiplication) {
	const char *script = R"(
		function test()
			return g_ivec3.new(0, 1, 2) * g_ivec3.new(0, 2, 2)
		end
	)";
	test(script, glm::ivec3(0, 2, 4));
}

TEST_F(LUAFunctionsTest, testVectorMultiplicationNonVector) {
	const char *script = R"(
		function test()
			return g_ivec3.new(0, 1, 2) * 2
		end
	)";
	test(script, glm::ivec3(0, 2, 4));
}

TEST_F(LUAFunctionsTest, testVectorDivision) {
	const char *script = R"(
		function test()
			return g_ivec3.new(0, 1, 2) / g_ivec3.new(1, 2, 2)
		end
	)";
	test(script, glm::ivec3(0, 0, 1));
}

TEST_F(LUAFunctionsTest, testVectorDivisionNonVector) {
	const char *script = R"(
		function test()
			return g_ivec3.new(0, 1, 2) / 2
		end
	)";
	test(script, glm::ivec3(0, 0, 1));
}

TEST_F(LUAFunctionsTest, testVectorComponents) {
	const char *script = R"(
		function test()
			local vec = g_ivec3.new(0, 0, 0)
			vec.x = 1
			vec.y = 2
			vec.z = 3
			return vec
		end
	)";
	test(script, glm::ivec3(1, 2, 3));
}

TEST_F(LUAFunctionsTest, testPushVec3) {
	static const char *script = R"(
		function test(vec)
			print(vec:__tostring())
			local val = vec.y
			if val == nil then
				error('vec.y failed')
			end
			if val == 1.0 then
				vec.x = 1
			else
				vec.x = val
			end
			vec.z = 3
			print(vec)
			return vec
		end
	)";
	LUA lua;
	clua_vecregister<glm::vec3>(lua);
	ASSERT_EQ(0, luaL_dostring(lua, script));
	lua_getglobal(lua, "test");
	ASSERT_TRUE(lua_isfunction(lua, -1));
	glm::vec3 inputVec(0.0f, 1.0f, 2.0f);
	ASSERT_EQ(1, clua_push(lua, inputVec));
	ASSERT_EQ(LUA_OK, lua_pcall(lua, 1, 1, 0)) << lua_tostring(lua, -1);
	const glm::vec3& outputVec = clua_tovec<glm::vec3>(lua, -1);
	EXPECT_FLOAT_EQ(1.0f, outputVec.x);
	EXPECT_FLOAT_EQ(1.0f, outputVec.y);
	EXPECT_FLOAT_EQ(3.0f, outputVec.z);
}

TEST_F(LUAFunctionsTest, testIO) {
	const char *script = R"(
		local function FourCC(fourcc)
			return string.unpack("<I", fourcc)
		end

		function test()
			local stream = g_io.open('rgb.qbcl', 'r')
			if stream == nil then
				error('stream is nil')
			end
			local magic = stream:readUInt32()
			if magic ~= FourCC("QBCL") then
				error('magic is not ' .. FourCC("QBCL") .. ' but ' .. magic)
			end
		end
	)";
	testExec(script);
}

TEST_F(LUAFunctionsTest, testBoundedAllocatorStopsLargeAllocations) {
	LUA lua;
	const size_t limit = 2 * 1024 * 1024;
	ASSERT_TRUE(lua.setMemoryLimit(limit));
	EXPECT_GT(lua.memoryUsage(), 0u);
	EXPECT_FALSE(lua.load("local memory=string.rep('x', 3 * 1024 * 1024)"));
	EXPECT_LE(lua.memoryPeak(), limit);
	ASSERT_TRUE(lua.load("local memory=string.rep('x', 1024)"));
	lua.shutdown();
	EXPECT_EQ(0u, lua.memoryUsage());
	EXPECT_FALSE(lua.load("return 1"));
	ASSERT_TRUE(lua.resetState());
	ASSERT_TRUE(lua.load("assert(2 + 2 == 4)"));
}

} // namespace lua
