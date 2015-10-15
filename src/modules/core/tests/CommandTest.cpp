#include <gtest/gtest.h>
#include "core/Command.h"

namespace core {

TEST(CommandTest, testExecute) {
	ASSERT_FALSE(Command::execute("test"));
	Command::registerCommand2("test", [] () {});
	ASSERT_TRUE(Command::execute("test"));
	Command::unregisterCommand("test");
	ASSERT_FALSE(Command::execute("test"));
}

}
