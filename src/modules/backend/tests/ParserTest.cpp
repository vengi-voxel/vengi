#include "TestShared.h"
#include "backend/entity/ai/condition/ConditionParser.h"
#include "backend/entity/ai/tree/TreeNodeParser.h"

namespace backend {

class ParserTest: public TestSuite {
};

TEST_F(ParserTest, testParseConditionSimple) {
	ConditionParser parser(_registry, "HasEnemies");
	const ConditionPtr& c = parser.getCondition();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testParseConditionNot) {
	ConditionParser parser(_registry, "Not(HasEnemies)");
	const ConditionPtr& c = parser.getCondition();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testParseConditionAndNot) {
	ConditionParser parser(_registry, "And(Not(HasEnemies),True)");
	const ConditionPtr& c = parser.getCondition();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testParseConditionAndNotInnerParam) {
	ConditionParser parser(_registry, "And(Not(HasEnemies{3}),True)");
	const ConditionPtr& c = parser.getCondition();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testParseConditionAndNotInnerOuterParam) {
	ConditionParser parser(_registry, "And(Not{3}(HasEnemies{3}),True)");
	const ConditionPtr& c = parser.getCondition();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testParseConditionAndWithNot) {
	ConditionParser parser(_registry, "And(Not(HasEnemies),Not(HasEnemies))");
	const ConditionPtr& c = parser.getCondition();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testParseConditionParmEverywhere) {
	ConditionParser parser(_registry, "And{1}(Not{3}(HasEnemies{3}),True{1})");
	const ConditionPtr& c = parser.getCondition();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testParseFail) {
	ConditionParser parser(_registry, "And(Not(HasEnemies{3},True)");
	const ConditionPtr& c = parser.getCondition();
	ASSERT_FALSE(c) << parser.getError();
}

TEST_F(ParserTest, testParseConditionNodeMultipleParamsAsChild) {
	ConditionParser parser(_registry, "Not(IsCloseToGroup{1,10})");
	const ConditionPtr& c = parser.getCondition();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testParseTreeNode) {
	TreeNodeParser parser(_registry, "Invert{1}");
	const TreeNodePtr& c = parser.getTreeNode();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testParseTreeNodeMultipleParams) {
	TreeNodeParser parser(_registry, "Invert{1,1000}");
	const TreeNodePtr& c = parser.getTreeNode();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testParseIdleNode) {
	TreeNodeParser parser(_registry, "Idle{1000}");
	const TreeNodePtr& c = parser.getTreeNode();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testParseIdleNodeNoParam) {
	TreeNodeParser parser(_registry, "Idle");
	const TreeNodePtr& c = parser.getTreeNode();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testParseUnknown) {
	TreeNodeParser parser(_registry, "Unknown");
	const TreeNodePtr& c = parser.getTreeNode();
	ASSERT_EQ(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testFilterMissingFilterType) {
	ConditionParser parser(_registry, "Filter");
	const ConditionPtr& c = parser.getCondition();
	ASSERT_EQ("missing details for Filter condition", parser.getError());
	ASSERT_EQ(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testFilter) {
	ConditionParser parser(_registry, "Filter(SelectEmpty)");
	const ConditionPtr& c = parser.getCondition();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testMultipleFilter) {
	ConditionParser parser(_registry, "Filter(SelectEmpty,SelectHighestAggro)");
	const ConditionPtr& c = parser.getCondition();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testSteer) {
	TreeNodeParser parser(_registry, "Steer{0.6,0.4}(GroupFlee{2},Wander{1})");
	const TreeNodePtr& c = parser.getTreeNode();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testSteerGroupLeader) {
	TreeNodeParser parser(_registry, "Steer{0.6,0.4}(GroupFlee{2},SelectionSeek)");
	const TreeNodePtr& c = parser.getTreeNode();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testSteerWithoutParam) {
	TreeNodeParser parser(_registry, "Steer(GroupFlee{2})");
	const TreeNodePtr& c = parser.getTreeNode();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testSteerWanderWithoutAnyParam) {
	TreeNodeParser parser(_registry, "Steer(Wander)");
	const TreeNodePtr& c = parser.getTreeNode();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testFilterInAnd) {
	ConditionParser parser(_registry, "And(Filter(SelectEmpty,SelectHighestAggro),True)");
	const ConditionPtr& c = parser.getCondition();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testInnerFiltersUnion) {
	ConditionParser parser(_registry, "Filter(Union(SelectEmpty,SelectHighestAggro))");
	const ConditionPtr& c = parser.getCondition();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testInnerFiltersIntersection) {
	ConditionParser parser(_registry, "Filter(Intersection(SelectEmpty,SelectHighestAggro,SelectZone))");
	const ConditionPtr& c = parser.getCondition();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testInnerFiltersCombination) {
	ConditionParser parser(_registry, "Filter(Intersection(Last(SelectEmpty),SelectHighestAggro,Random{1}(SelectZone)))");
	const ConditionPtr& c = parser.getCondition();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

TEST_F(ParserTest, testMultipleFilterInAnd) {
	ConditionParser parser(_registry, "And(Filter(SelectEmpty,SelectHighestAggro),True,And(Filter(SelectEmpty,SelectHighestAggro),True))");
	const ConditionPtr& c = parser.getCondition();
	ASSERT_NE(nullptr, c.get()) << parser.getError();
}

}
