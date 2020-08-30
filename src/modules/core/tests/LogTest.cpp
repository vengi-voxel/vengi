/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/Log.h"

namespace core {

TEST(LogTest, testLogId) {
	const auto logid1 = Log::logid("LogTest1");
	const auto logid2 = Log::logid("LogTest2");
	ASSERT_NE(logid1, logid2);
}

}
