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
	const glm::ivec3* v = clua_get<glm::ivec3>(lua.state(), -1);
	ASSERT_EQ(0, v->x);
	ASSERT_EQ(1, v->y);
	ASSERT_EQ(0, v->z);
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
	const glm::ivec3* v = clua_get<glm::ivec3>(lua.state(), -1);
	ASSERT_EQ(0, v->x);
	ASSERT_EQ(2, v->y);
	ASSERT_EQ(1, v->z);
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
	const glm::ivec3* v = clua_get<glm::ivec3>(lua.state(), -1);
	ASSERT_EQ(1, v->x);
	ASSERT_EQ(2, v->y);
	ASSERT_EQ(3, v->z);
}

}
