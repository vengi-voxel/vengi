/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "core/JSON.h"

namespace core {

class JSONTest: public AbstractTest {
};

TEST_F(JSONTest, testParse) {
	const std::string jsonStr = R"({ "key": "value", "key2": 42 })";
	auto j = json::parse(jsonStr);
	ASSERT_EQ(42, j["key2"]);
}

TEST_F(JSONTest, testParseVector) {
	const std::string jsonStr = R"([0, 1])";
	auto j = json::parse(jsonStr);
	const glm::ivec2 v = j;
	ASSERT_EQ(0, v.x);
	ASSERT_EQ(1, v.y);
}

}
