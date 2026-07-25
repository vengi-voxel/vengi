/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/Var.h"
#include "util/VarUtil.h"

namespace util {

class VarUtilTest : public testing::Test {
public:
	void TearDown() override {
		core::Var::shutdown();
	}
};

TEST_F(VarUtilTest, testScopedVarChangeByName) {
	const core::VarPtr &v = core::Var::registerVar(core::VarDef("test", "initial", nullptr, nullptr));
	EXPECT_EQ("initial", v->strVal());
	{
		ScopedVarChange scoped("test", "changed");
		EXPECT_EQ("changed", v->strVal());
	}
	EXPECT_EQ("initial", v->strVal());
}

TEST_F(VarUtilTest, testScopedVarChangeByPtr) {
	const core::VarPtr &v = core::Var::registerVar(core::VarDef("test", "initial", nullptr, nullptr));
	EXPECT_EQ("initial", v->strVal());
	{
		ScopedVarChange scoped(v, "changed");
		EXPECT_EQ("changed", v->strVal());
	}
	EXPECT_EQ("initial", v->strVal());
}

TEST_F(VarUtilTest, testScopedVarChangeByPtrInt) {
	const core::VarPtr &v = core::Var::registerVar(core::VarDef("test", "1", nullptr, nullptr));
	{
		ScopedVarChange scoped(v, 42);
		EXPECT_EQ(42, v->intVal());
	}
	EXPECT_EQ(1, v->intVal());
}

} // namespace util
