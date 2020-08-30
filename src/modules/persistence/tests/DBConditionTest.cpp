/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "persistence/DBCondition.h"

namespace persistence {

class DBConditionTest : public core::AbstractTest {
};

TEST_F(DBConditionTest, testDBCondition) {
	const DBCondition c1("field1", persistence::FieldType::MAX, "value1", Comparator::Bigger);
	int parameterCount = 0;
	ASSERT_EQ("\"field1\" > $1", c1.statement(parameterCount));
	ASSERT_EQ(1, parameterCount);
}

TEST_F(DBConditionTest, testDBConditionMultiple) {
	const DBCondition c1("field1", persistence::FieldType::MAX, "value1", Comparator::Bigger);
	const DBCondition c2("field2", persistence::FieldType::MAX, "value2", Comparator::Equal);
	DBConditionMultiple m(true, {&c1, &c2});
	int parameterCount = 0;
	ASSERT_EQ("\"field1\" > $1 AND \"field2\" = $2", m.statement(parameterCount));
	ASSERT_EQ(2, parameterCount);
}

TEST_F(DBConditionTest, testDBConditionMultipleStacked) {
	const DBCondition c1_1("field1", persistence::FieldType::MAX, "value1", Comparator::Bigger);
	const DBCondition c1_2("field2", persistence::FieldType::MAX, "value2", Comparator::Equal);
	DBConditionMultiple m1(true, {&c1_1, &c1_2});

	const DBCondition c2_1("field1", persistence::FieldType::MAX, "value1", Comparator::Lesser);
	const DBCondition c2_2("field2", persistence::FieldType::MAX, "value2", Comparator::BiggerOrEqual);
	DBConditionMultiple m2(true, {&c2_1, &c2_2});

	DBConditionMultiple mall(true, {&m1, &m2});

	int parameterCount = 0;
	ASSERT_EQ("\"field1\" > $1 AND \"field2\" = $2 AND \"field1\" < $3 AND \"field2\" >= $4", mall.statement(parameterCount));
	ASSERT_EQ(4, parameterCount);
}

}
