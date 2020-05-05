/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "voxelgenerator/LSystem.h"

namespace voxelgenerator {
namespace lsystem {

TEST(LSystemTests, testParse) {
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
	std::vector<Rule> rules;
	ASSERT_TRUE(parseRules(rulesString, rules));
	ASSERT_EQ(2u, rules.size());
}

}
}
