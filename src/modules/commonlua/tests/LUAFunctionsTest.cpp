/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "commonlua/LUAFunctions.h"

namespace lua {

class LUAFunctionsTest : public core::AbstractTest {
};

TEST_F(LUAFunctionsTest, testVectorCtor) {
	LUA lua;
	clua_vecregister<glm::ivec3>(lua.state());
	ASSERT_TRUE(lua.load("function test() return ivec3.new(0, 1, 0) end")) << lua.error();
	ASSERT_TRUE(lua.execute("test", 1)) << lua.error();
	const glm::ivec3* v = clua_get<glm::ivec3>(lua.state(), -1);
	ASSERT_EQ(0, v->x);
	ASSERT_EQ(1, v->y);
	ASSERT_EQ(0, v->z);
}

TEST_F(LUAFunctionsTest, testVectorAddition) {
	LUA lua;
	clua_vecregister<glm::ivec3>(lua.state());
	ASSERT_TRUE(lua.load("function test() return ivec3.new(0, 1, 0) + ivec3.new(0, 1, 1) end")) << lua.error();
	ASSERT_TRUE(lua.execute("test", 1)) << lua.error();
	const glm::ivec3* v = clua_get<glm::ivec3>(lua.state(), -1);
	ASSERT_EQ(0, v->x);
	ASSERT_EQ(2, v->y);
	ASSERT_EQ(1, v->z);
}

}
