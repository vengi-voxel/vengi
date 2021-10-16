#include <gtest/gtest.h>
#include "io/FormatDescription.h"

namespace io {

class FormatDescriptionTest: public testing::Test {
};

TEST_F(FormatDescriptionTest, testGetPath) {
	ASSERT_EQ(core::String("*.foo"), getWildcardsFromPattern("Foo (*.foo)"));
	ASSERT_EQ(core::String("*.foo,*.bar"), getWildcardsFromPattern("Foo (*.foo,*.bar)"));
}

}
