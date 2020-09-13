/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "commonlua/LUAFunctions.h"

namespace lua {

class LUAFunctionsTest : public app::AbstractTest {
};

TEST_F(LUAFunctionsTest, testVectorCtor) {
	static const char *script = R"(
		function test()
			return ivec3.new(0, 1, 0)
		end
	)";
	LUA lua;
	clua_vecregister<glm::ivec3>(lua.state());
	ASSERT_TRUE(lua.load(script)) << lua.error();
	ASSERT_TRUE(lua.execute("test", 1)) << lua.error();
	const glm::ivec3& v = clua_tovec<glm::ivec3>(lua, -1);
	ASSERT_EQ(0, v.x);
	ASSERT_EQ(1, v.y);
	ASSERT_EQ(0, v.z);
}

TEST_F(LUAFunctionsTest, testVectorAddition) {
	static const char *script = R"(
		function test()
			return ivec3.new(0, 1, 0) + ivec3.new(0, 1, 1)
		end
	)";
	LUA lua;
	clua_vecregister<glm::ivec3>(lua.state());
	ASSERT_TRUE(lua.load(script)) << lua.error();
	ASSERT_TRUE(lua.execute("test", 1)) << lua.error();
	const glm::ivec3& v = clua_tovec<glm::ivec3>(lua, -1);
	ASSERT_EQ(0, v.x);
	ASSERT_EQ(2, v.y);
	ASSERT_EQ(1, v.z);
}

TEST_F(LUAFunctionsTest, testVectorComponents) {
	static const char *script = R"(
		function test()
			local vec = ivec3.new(0, 0, 0)
			vec.x = 1
			vec.y = 2
			vec.z = 3
			return vec
		end
	)";
	LUA lua;
	clua_vecregister<glm::ivec3>(lua.state());
	ASSERT_TRUE(lua.load(script)) << lua.error();
	ASSERT_TRUE(lua.execute("test", 1)) << lua.error();
	const glm::ivec3& v = clua_tovec<glm::ivec3>(lua, -1);
	ASSERT_EQ(1, v.x);
	ASSERT_EQ(2, v.y);
	ASSERT_EQ(3, v.z);
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
