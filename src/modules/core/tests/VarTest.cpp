/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/Var.h"

namespace core {

TEST(VarTest, testChange) {
	const VarPtr& v = Var::get("test", "nonsense");
	ASSERT_EQ("nonsense", v->strVal());
	v->setVal("1");
	ASSERT_EQ("1", v->strVal());
	ASSERT_EQ(1, v->intVal());
}

TEST(VarTest, testFlags) {
	const VarPtr& v = Var::get("test", "nonsense", CV_READONLY);
	ASSERT_EQ(CV_READONLY, v->getFlags());
}

TEST(VarTest, testFlagsOverride) {
	const VarPtr& v = Var::get("test", "nonsense");
	Var::get("test", "nonsense", CV_READONLY);
	ASSERT_EQ(CV_READONLY, v->getFlags());
}

TEST(VarTest, testDirty) {
	const VarPtr& v = Var::get("test", "nonsense");
	v->setVal("reasonable");
	ASSERT_TRUE(v->isDirty());
	v->markClean();
	ASSERT_FALSE(v->isDirty());
}

}
