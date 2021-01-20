/**
 * @file
 */

#include "core/concurrent/Thread.h"
#include <gtest/gtest.h>

namespace core {

TEST(ThreadTest, testNoCaptureLambda) {
	core::Thread thread("test", [](void *data) { return 1; });
	ASSERT_EQ(1, thread.join());
}

}
