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

}
