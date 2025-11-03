/**
 * @file
 */

#include "network/NetworkAdapters.h"
#include <gtest/gtest.h>

namespace network {

TEST(NetworkAdaptersTest, ListInterfaces) {
	auto interfaces = getNetworkAdapters();
	ASSERT_FALSE(interfaces.empty());
}

} // namespace network
