/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/Var.h"
#include <SDL_stdinc.h>
#include <SDL_version.h>

#if SDL_VERSION_ATLEAST(3, 2, 0)
#define SDL_setenv SDL_setenv_unsafe
#endif

namespace core {

class VarTest: public testing::Test {
public:
	void TearDown() override {
		core::Var::shutdown();
	}
};

TEST_F(VarTest, testChange) {
	const VarPtr& v = Var::registerVar("test", "nonsense");
	EXPECT_EQ("nonsense", v->strVal());
	v->setVal("1");
	EXPECT_EQ("1", v->strVal());
	EXPECT_EQ(1, v->intVal());
}

TEST_F(VarTest, testFlags) {
	const VarPtr& v = Var::registerVar("test", "nonsense", CV_READONLY);
	EXPECT_EQ(CV_READONLY, v->getFlags());
}

TEST_F(VarTest, testFlagsOverride) {
	const VarPtr& v = Var::registerVar("test", "nonsense");
	Var::registerVar("test", "nonsense", CV_READONLY);
	EXPECT_EQ(CV_READONLY, v->getFlags());
}

TEST_F(VarTest, testDirty) {
	const VarPtr& v = Var::registerVar("test", "nonsense");
	v->setVal("reasonable");
	EXPECT_TRUE(v->isDirty());
	v->markClean();
	EXPECT_FALSE(v->isDirty());
}

TEST_F(VarTest, testPriorityWithoutEnvironmentVariable) {
	// onConstruct
	Var::registerVar("test", "initialvalue");
	EXPECT_EQ(Var::findVar("test")->strVal(), "initialvalue");

	// onConstruct argument parsing
	Var::registerVar("test", "commandline", CV_FROMCOMMANDLINE);
	EXPECT_EQ(Var::findVar("test")->strVal(), "commandline") << "Commandline should have the highest priority";

	// load appname.vars
	Var::registerVar("test", "file", CV_FROMFILE);
	EXPECT_EQ(Var::findVar("test")->strVal(), "commandline") << "Expected to get the value from the commandline";

	Var::registerVar("test", "no", CV_FROMFILE);
	EXPECT_EQ(Var::findVar("test")->strVal(), "commandline") << "Expected to get the value from the commandline";

	Var::registerVar("test", "no", CV_FROMENV);
	EXPECT_EQ(Var::findVar("test")->strVal(), "commandline") << "Expected to get the value from the commandline";

	Var::registerVar("test", "no");
	EXPECT_EQ(Var::findVar("test")->strVal(), "commandline") << "Expected to get the value from the commandline";

	Var::registerVar("test", "");
	EXPECT_EQ(Var::findVar("test")->strVal(), "commandline") << "Expected to get the value from the commandline";

	Var::registerVar("test", "")->setVal("custom");
	EXPECT_EQ(Var::findVar("test")->strVal(), "custom") << "Expected to get the value from the manual set call";
}

TEST_F(VarTest, testPriorityFromFile) {
	// onConstruct
	Var::registerVar("test", "initialvalue");
	EXPECT_EQ(Var::findVar("test")->strVal(), "initialvalue");

	// load appname.vars
	Var::registerVar("test", "file", CV_FROMFILE);
	EXPECT_EQ(Var::findVar("test")->strVal(), "file") << "Expected to get the value from the file";
}

TEST_F(VarTest, testPriorityFromEnv) {
	SDL_setenv("test", "env", 1);

	// onConstruct
	Var::registerVar("test", "initialvalue");
	EXPECT_EQ(Var::findVar("test")->strVal(), "env") << "Expected to get the value from the env";

	// load appname.vars
	Var::registerVar("test", "file", CV_FROMFILE);
	EXPECT_EQ(Var::findVar("test")->strVal(), "env") << "Expected to still have the value from env";
	SDL_setenv("test", "", 1);
}

TEST_F(VarTest, testPriorityEnvOverrideFromCmd) {
	SDL_setenv("test", "env", 1);

	// onConstruct
	Var::registerVar("test", "initialvalue");
	EXPECT_EQ(Var::findVar("test")->strVal(), "env") << "Expected to get the value from the env";

	// onConstruct argument parsing
	Var::registerVar("test", "commandline", CV_FROMCOMMANDLINE);
	EXPECT_EQ(Var::findVar("test")->strVal(), "commandline") << "Commandline should have the highest priority";
	SDL_setenv("test", "", 1);
}

TEST_F(VarTest, testHistory) {
	const VarPtr& v = Var::registerVar("test", "nonsense");
	EXPECT_EQ("nonsense", v->strVal());
	v->setVal("reasonable");
	EXPECT_EQ(2u, v->getHistorySize());
	EXPECT_EQ(1u, v->getHistoryIndex());
	EXPECT_EQ("reasonable", v->strVal());
	v->useHistory(0);
	EXPECT_EQ(0u, v->getHistoryIndex());
	EXPECT_EQ("nonsense", v->strVal());
}

TEST_F(VarTest, testHistoryCleanup) {
	const VarPtr& v = Var::registerVar("test", "nonsense");
	for (int i = 0; i < 120; ++i) {
		v->setVal(core::String::format("reasonable%i", i));
	}
	EXPECT_EQ("reasonable119", v->strVal());
}

}
