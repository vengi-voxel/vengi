/**
 * @file
 */

#include "voxelgenerator/LSystem.h"
#include "app/tests/AbstractTest.h"

namespace voxelgenerator {
namespace lsystem {

class LSystemTests : public app::AbstractTest {};

TEST_F(LSystemTests, testParse) {
	const core::String rulesString = R"(
		{
			F
			F+[!+F-F-FL]-[!-F+F+FL]>[!F<F<FL]<[!<F>F>FL]
		}
		{
			F
			F+[!+F-F-FL]-[!-F+F+FL]>[!F<F<FL]<[!<F>F>FL]
		}
	)";
	core::DynamicArray<Rule> rules;
	ASSERT_TRUE(parseRules(rulesString, rules));
	ASSERT_EQ(2u, rules.size());
}

}
}
