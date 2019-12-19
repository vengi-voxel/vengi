#include "TestShared.h"

class ParserTest: public TestSuite {
};

TEST_F(ParserTest, testParseConditionSimple) {
	ai::ConditionParser parser(_registry, "HasEnemies");
	const ai::ConditionPtr& c = parser.getCondition();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testParseConditionNot) {
	ai::ConditionParser parser(_registry, "Not(HasEnemies)");
	const ai::ConditionPtr& c = parser.getCondition();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testParseConditionAndNot) {
	ai::ConditionParser parser(_registry, "And(Not(HasEnemies),True)");
	const ai::ConditionPtr& c = parser.getCondition();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testParseConditionAndNotInnerParam) {
	ai::ConditionParser parser(_registry, "And(Not(HasEnemies{3}),True)");
	const ai::ConditionPtr& c = parser.getCondition();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testParseConditionAndNotInnerOuterParam) {
	ai::ConditionParser parser(_registry, "And(Not{3}(HasEnemies{3}),True)");
	const ai::ConditionPtr& c = parser.getCondition();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testParseConditionAndWithNot) {
	ai::ConditionParser parser(_registry, "And(Not(HasEnemies),Not(HasEnemies))");
	const ai::ConditionPtr& c = parser.getCondition();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testParseConditionParmEverywhere) {
	ai::ConditionParser parser(_registry, "And{1}(Not{3}(HasEnemies{3}),True{1})");
	const ai::ConditionPtr& c = parser.getCondition();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testParseFail) {
	ai::ConditionParser parser(_registry, "And(Not(HasEnemies{3},True)");
	const ai::ConditionPtr& c = parser.getCondition();
	ASSERT_FALSE(c) << parser.getError();
}

TEST_F(ParserTest, testParseConditionNodeMultipleParamsAsChild) {
	ai::ConditionParser parser(_registry, "Not(IsCloseToGroup{1,10})");
	const ai::ConditionPtr& c = parser.getCondition();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testParseTreeNode) {
	ai::TreeNodeParser parser(_registry, "Invert{1}");
	const ai::TreeNodePtr& c = parser.getTreeNode();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testParseTreeNodeMultipleParams) {
	ai::TreeNodeParser parser(_registry, "Invert{1,1000}");
	const ai::TreeNodePtr& c = parser.getTreeNode();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testParseIdleNode) {
	ai::TreeNodeParser parser(_registry, "Idle{1000}");
	const ai::TreeNodePtr& c = parser.getTreeNode();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testParseIdleNodeNoParam) {
	ai::TreeNodeParser parser(_registry, "Idle");
	const ai::TreeNodePtr& c = parser.getTreeNode();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testParseUnknown) {
	ai::TreeNodeParser parser(_registry, "Unknown");
	const ai::TreeNodePtr& c = parser.getTreeNode();
	ASSERT_EQ(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testFilterMissingFilterType) {
	ai::ConditionParser parser(_registry, "Filter");
	const ai::ConditionPtr& c = parser.getCondition();
	ASSERT_EQ("missing details for Filter condition", parser.getError());
	ASSERT_EQ(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testFilter) {
	ai::ConditionParser parser(_registry, "Filter(SelectEmpty)");
	const ai::ConditionPtr& c = parser.getCondition();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testMultipleFilter) {
	ai::ConditionParser parser(_registry, "Filter(SelectEmpty,SelectHighestAggro)");
	const ai::ConditionPtr& c = parser.getCondition();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testSteer) {
	ai::TreeNodeParser parser(_registry, "Steer{0.6,0.4}(GroupFlee{2},Wander{1})");
	const ai::TreeNodePtr& c = parser.getTreeNode();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testSteerGroupLeader) {
	ai::TreeNodeParser parser(_registry, "Steer{0.6,0.4}(GroupFlee{2},SelectionSeek)");
	const ai::TreeNodePtr& c = parser.getTreeNode();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testSteerWithoutParam) {
	ai::TreeNodeParser parser(_registry, "Steer(GroupFlee{2})");
	const ai::TreeNodePtr& c = parser.getTreeNode();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testSteerWanderWithoutAnyParam) {
	ai::TreeNodeParser parser(_registry, "Steer(Wander)");
	const ai::TreeNodePtr& c = parser.getTreeNode();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testFilterInAnd) {
	ai::ConditionParser parser(_registry, "And(Filter(SelectEmpty,SelectHighestAggro),True)");
	const ai::ConditionPtr& c = parser.getCondition();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testInnerFiltersUnion) {
	ai::ConditionParser parser(_registry, "Filter(Union(SelectEmpty,SelectHighestAggro))");
	const ai::ConditionPtr& c = parser.getCondition();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testInnerFiltersIntersection) {
	ai::ConditionParser parser(_registry, "Filter(Intersection(SelectEmpty,SelectHighestAggro,SelectZone))");
	const ai::ConditionPtr& c = parser.getCondition();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testInnerFiltersCombination) {
	ai::ConditionParser parser(_registry, "Filter(Intersection(Last(SelectEmpty),SelectHighestAggro,Random{1}(SelectZone)))");
	const ai::ConditionPtr& c = parser.getCondition();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testMultipleFilterInAnd) {
	ai::ConditionParser parser(_registry, "And(Filter(SelectEmpty,SelectHighestAggro),True,And(Filter(SelectEmpty,SelectHighestAggro),True))");
	const ai::ConditionPtr& c = parser.getCondition();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}
