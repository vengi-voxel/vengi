/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/Common.h"
#include <glm/vec4.hpp>

namespace core {

TEST(CoreTest, testVecSize) {
	std::vector<glm::vec4> vec4;
	ASSERT_EQ(0u, core::vectorSize(vec4));
	vec4.push_back(glm::vec4());
	ASSERT_EQ(4 * sizeof(glm::vec4::value_type), core::vectorSize(vec4));
	vec4.push_back(glm::vec4());
	ASSERT_EQ(8 * sizeof(glm::vec4::value_type), core::vectorSize(vec4));
}

}
