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
	ASSERT_TRUE(p.init("testattributes.lua")) << p.error();
}

TEST_F(ContainerProviderTest, testLoadingUnknownType) {
	ContainerProvider p;
	ASSERT_FALSE(p.init("testattributes_fail.lua")) << p.error();
}

}
