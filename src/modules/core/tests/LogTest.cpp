/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "core/Log.h"

namespace core {

class LogTest : public core::AbstractTest {
};

TEST_F(LogTest, testLogId) {
	const auto logid1 = Log::logid("LogTest1");
	const auto logid2 = Log::logid("LogTest2");
	ASSERT_NE(logid1, logid2);
}

}
