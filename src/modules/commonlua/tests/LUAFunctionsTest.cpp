/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "commonlua/LUAFunctions.h"
#include <glm/vector_relational.hpp>

namespace lua {

class LUAFunctionsTest : public app::AbstractTest {
public:
	template<typename T>
	void test(const char *script, const T& expected) {
		LUA lua;
		clua_vecregister<T>(lua.state());
		ASSERT_TRUE(lua.load(script)) << lua.error();
		ASSERT_TRUE(lua.execute("test", 1)) << lua.error();
		const T& v = clua_tovec<T>(lua, -1);
		EXPECT_TRUE(glm::all(glm::equal(expected, v)));
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

}
