/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/FourCC.h"
#include <glm/fwd.hpp>
#include <glm/vec4.hpp>
#include <list>
#include <vector>

namespace core {

TEST(CoreTest, testFourCC) {
	const uint32_t fcc = FourCC('a', 'b', 'c', 'd');
	uint8_t buf[4];
	FourCCRev(buf, fcc);
	EXPECT_EQ(buf[0], 'a');
	EXPECT_EQ(buf[1], 'b');
	EXPECT_EQ(buf[2], 'c');
	EXPECT_EQ(buf[3], 'd');
}

}
