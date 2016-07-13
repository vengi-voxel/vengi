/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/Command.h"

namespace core {

TEST(CommandTest, testExecute) {
	ASSERT_EQ(0, Command::execute("test"));
	Command::registerCommand2("test", [] () {});
	ASSERT_EQ(1, Command::execute("test"));
	ASSERT_EQ(3, Command::execute("test;test parameter; test"));
	Command::unregisterCommand("test");
	ASSERT_EQ(0, Command::execute("test"));
}

}
