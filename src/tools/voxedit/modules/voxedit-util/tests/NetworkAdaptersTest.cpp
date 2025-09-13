/**
 * @file
 */

#include "../network/NetworkAdapters.h"
#include <gtest/gtest.h>

namespace voxedit {
namespace network {

TEST(NetworkAdaptersTest, ListInterfaces) {
	auto interfaces = getNetworkAdapters();
	ASSERT_FALSE(interfaces.empty());
}

} // namespace network
} // namespace voxedit
