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

TEST_F(VarTest, testPriorityWithoutEnvironmentVariable) {
	// onConstruct
	Var::get("test", "initialvalue");
	EXPECT_EQ(Var::get("test")->strVal(), "initialvalue");

	// onConstruct argument parsing
	Var::get("test", "commandline", CV_FROMCOMMANDLINE);
	EXPECT_EQ(Var::get("test")->strVal(), "commandline") << "Commandline should have the highest priority";

	// load appname.vars
	Var::get("test", "file", CV_FROMFILE);
	EXPECT_EQ(Var::get("test")->strVal(), "commandline") << "Expected to get the value from the commandline";

	Var::get("test", "no", CV_FROMFILE);
	EXPECT_EQ(Var::get("test")->strVal(), "commandline") << "Expected to get the value from the commandline";

	Var::get("test", "no", CV_FROMENV);
	EXPECT_EQ(Var::get("test")->strVal(), "commandline") << "Expected to get the value from the commandline";

	Var::get("test", "no");
	EXPECT_EQ(Var::get("test")->strVal(), "commandline") << "Expected to get the value from the commandline";

	Var::get("test");
	EXPECT_EQ(Var::get("test")->strVal(), "commandline") << "Expected to get the value from the commandline";

	Var::get("test")->setVal("custom");
	EXPECT_EQ(Var::get("test")->strVal(), "custom") << "Expected to get the value from the manual set call";
}

TEST_F(VarTest, testPriorityFromFile) {
	// onConstruct
	Var::get("test", "initialvalue");
	EXPECT_EQ(Var::get("test")->strVal(), "initialvalue");

	// load appname.vars
	Var::get("test", "file", CV_FROMFILE);
	EXPECT_EQ(Var::get("test")->strVal(), "file") << "Expected to get the value from the file";
}

TEST_F(VarTest, testPriorityFromEnv) {
	SDL_setenv("test", "env", 1);

	// onConstruct
	Var::get("test", "initialvalue");
	EXPECT_EQ(Var::get("test")->strVal(), "env") << "Expected to get the value from the env";

	// load appname.vars
	Var::get("test", "file", CV_FROMFILE);
	EXPECT_EQ(Var::get("test")->strVal(), "env") << "Expected to still have the value from env";
}

TEST_F(VarTest, testPriorityEnvOverrideFromCmd) {
	SDL_setenv("test", "env", 1);

	// onConstruct
	Var::get("test", "initialvalue");
	EXPECT_EQ(Var::get("test")->strVal(), "env") << "Expected to get the value from the env";

	// onConstruct argument parsing
	Var::get("test", "commandline", CV_FROMCOMMANDLINE);
	EXPECT_EQ(Var::get("test")->strVal(), "commandline") << "Commandline should have the highest priority";
}

}
