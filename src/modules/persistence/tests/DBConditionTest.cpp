/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "persistence/DBCondition.h"

namespace persistence {

class DBConditionTest : public core::AbstractTest {
};

TEST_F(DBConditionTest, testDBCondition) {
	const DBCondition c1("field1", "value1", Operator::Bigger);
	int parameterCount = 0;
	ASSERT_EQ("\"field1\" > $1", c1.statement(parameterCount));
}

TEST_F(DBConditionTest, testDBConditionMultiple) {
	const DBCondition c1("field1", "value1", Operator::Bigger);
	const DBCondition c2("field2", "value2", Operator::Equal);
	DBConditionMultiple m(true, {c1, c2});
	int parameterCount = 0;
	ASSERT_EQ("\"field1\" > $1 AND \"field2\" = $2", m.statement(parameterCount));
}

}
