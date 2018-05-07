/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "attrib/ContainerProvider.h"

const char *TestLoadingSuccess = R"(
function init()
	local test1 = attrib.createContainer("test1")
	test1:absolute("ATTACKRANGE", 2.0)
	test1:register()
end
)";

const char *TestLoadingUnknownType = R"(
function init()
	local test1 = attrib.createContainer("test1")
	test1:absolute("FOO", 2.0)
	test1:register()
end
)";

namespace attrib {

class ContainerProviderTest: public core::AbstractTest {
};

TEST_F(ContainerProviderTest, testLoadingSuccess) {
	ContainerProvider p;
	ASSERT_TRUE(p.init(TestLoadingSuccess)) << p.error();
}

TEST_F(ContainerProviderTest, testLoadingUnknownType) {
	ContainerProvider p;
	ASSERT_FALSE(p.init(TestLoadingUnknownType)) << p.error();
}

}
