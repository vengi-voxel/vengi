/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/MD5.h"
#include "core/ArrayLength.h"

namespace core {

TEST(MD5Test, test1) {
	const uint8_t buf[] = {'1'};
	ASSERT_EQ("c4ca4238a0b923820dcc509a6f75849b", md5sum(buf, lengthof(buf)));
}

}
