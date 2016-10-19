/**
 * @file
 */

#include "AbstractTest.h"
#include "core/Var.h"

namespace core {

class VarTest: public AbstractTest {
};

TEST_F(VarTest, testChange) {
	const VarPtr& v = Var::get("test", "nonsense");
	ASSERT_EQ("nonsense", v->strVal());
	v->setVal("1");
	ASSERT_EQ("1", v->strVal());
	ASSERT_EQ(1, v->intVal());
}

TEST_F(VarTest, testFlags) {
	const VarPtr& v = Var::get("test", "nonsense", CV_READONLY);
	ASSERT_EQ(CV_READONLY, v->getFlags());
}

TEST_F(VarTest, testFlagsOverride) {
	const VarPtr& v = Var::get("test", "nonsense");
	Var::get("test", "nonsense", CV_READONLY);
	ASSERT_EQ(CV_READONLY, v->getFlags());
}

TEST_F(VarTest, testDirty) {
	const VarPtr& v = Var::get("test", "nonsense");
	v->setVal("reasonable");
	ASSERT_TRUE(v->isDirty());
	v->markClean();
	ASSERT_FALSE(v->isDirty());
}

}
