/**
 * @file
 */

#include "Shared_generated.h"
#include "attrib/Container.h"
#include "app/tests/AbstractTest.h"
#include "attrib/ContainerProvider.h"
#include "core/Enum.h"
#include "gtest/gtest.h"

const char *TestLoadingSuccess = R"(
function init()
	local test1 = attrib.createContainer("test1")
	test1:addAbsolute("ATTACKRANGE", 2.0)
	test1:addPercentage("ATTACKRANGE", 25.0)
end
)";

const char *TestLoadingUnknownType = R"(
function init()
	local test1 = attrib.createContainer("test2")
	test1:addAbsolute("FOO", 2.0)
end
)";

namespace attrib {

class ContainerProviderTest: public app::AbstractTest {
};

TEST_F(ContainerProviderTest, testLoadingSuccess) {
	ContainerProvider p;
	ASSERT_TRUE(p.init(TestLoadingSuccess)) << p.error();
	const ContainerPtr& c = p.container("test1");
	ASSERT_TRUE(c) << "Could not find container test1";
	EXPECT_DOUBLE_EQ(2.0, c->absolute()[core::enumVal(Type::ATTACKRANGE)]);
	EXPECT_DOUBLE_EQ(25.0, c->percentage()[core::enumVal(Type::ATTACKRANGE)]);
}

TEST_F(ContainerProviderTest, testLoadingUnknownType) {
	ContainerProvider p;
	ASSERT_FALSE(p.init(TestLoadingUnknownType)) << p.error();
	const ContainerPtr& c = p.container("test2");
	ASSERT_TRUE(c) << "Could not find container test2";
}

}
