/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "attrib/ContainerProvider.h"

namespace attrib {

class ContainerProviderTest: public core::AbstractTest {
};

TEST_F(ContainerProviderTest, testLoadingSuccess) {
	ContainerProvider p;
	const std::string& attributes = _testApp->filesystem()->load("testattributes.lua");
	ASSERT_TRUE(p.init(attributes)) << p.error();
}

TEST_F(ContainerProviderTest, testLoadingUnknownType) {
	ContainerProvider p;
	const std::string& attributes = _testApp->filesystem()->load("testattributes_fail.lua");
	ASSERT_FALSE(p.init(attributes)) << p.error();
}

}
