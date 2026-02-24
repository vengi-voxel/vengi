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
	const VarPtr& v = Var::registerVar(VarDef("test", "nonsense", nullptr, nullptr));
	EXPECT_EQ("nonsense", v->strVal());
	v->setVal("1");
	EXPECT_EQ("1", v->strVal());
	EXPECT_EQ(1, v->intVal());
}

TEST_F(VarTest, testFlags) {
	const VarPtr& v = Var::registerVar(VarDef("test", "nonsense", nullptr, nullptr, CV_READONLY));
	EXPECT_EQ(CV_READONLY, v->getFlags());
}

TEST_F(VarTest, testFlagsOverride) {
	const VarPtr& v = Var::registerVar(VarDef("test", "nonsense", nullptr, nullptr));
	Var::registerVar(VarDef("test", "nonsense", nullptr, nullptr, CV_READONLY));
	EXPECT_EQ(CV_READONLY, v->getFlags());
}

TEST_F(VarTest, testDirty) {
	const VarPtr& v = Var::registerVar(VarDef("test", "nonsense", nullptr, nullptr));
	v->setVal("reasonable");
	EXPECT_TRUE(v->isDirty());
	v->markClean();
	EXPECT_FALSE(v->isDirty());
}

TEST_F(VarTest, testPriorityWithoutEnvironmentVariable) {
	// onConstruct
	Var::registerVar(VarDef("test", "initialvalue", nullptr, nullptr));
	EXPECT_EQ(Var::findVar("test")->strVal(), "initialvalue");

	// onConstruct argument parsing
	Var::registerVar(VarDef("test", "commandline", nullptr, nullptr, CV_FROMCOMMANDLINE));
	EXPECT_EQ(Var::findVar("test")->strVal(), "commandline") << "Commandline should have the highest priority";

	// load appname.vars
	Var::registerVar(VarDef("test", "file", nullptr, nullptr, CV_FROMFILE));
	EXPECT_EQ(Var::findVar("test")->strVal(), "commandline") << "Expected to get the value from the commandline";

	Var::registerVar(VarDef("test", "no", nullptr, nullptr, CV_FROMFILE));
	EXPECT_EQ(Var::findVar("test")->strVal(), "commandline") << "Expected to get the value from the commandline";

	Var::registerVar(VarDef("test", "no", nullptr, nullptr, CV_FROMENV));
	EXPECT_EQ(Var::findVar("test")->strVal(), "commandline") << "Expected to get the value from the commandline";

	Var::registerVar(VarDef("test", "no", nullptr, nullptr));
	EXPECT_EQ(Var::findVar("test")->strVal(), "commandline") << "Expected to get the value from the commandline";

	Var::registerVar(VarDef("test", "", nullptr, nullptr));
	EXPECT_EQ(Var::findVar("test")->strVal(), "commandline") << "Expected to get the value from the commandline";

	Var::registerVar(VarDef("test", "", nullptr, nullptr))->setVal("custom");
	EXPECT_EQ(Var::findVar("test")->strVal(), "custom") << "Expected to get the value from the manual set call";
}

TEST_F(VarTest, testPriorityFromFile) {
	// onConstruct
	Var::registerVar(VarDef("test", "initialvalue", nullptr, nullptr));
	EXPECT_EQ(Var::findVar("test")->strVal(), "initialvalue");

	// load appname.vars
	Var::registerVar(VarDef("test", "file", nullptr, nullptr, CV_FROMFILE));
	EXPECT_EQ(Var::findVar("test")->strVal(), "file") << "Expected to get the value from the file";
}

TEST_F(VarTest, testPriorityFromEnv) {
	SDL_setenv("test", "env", 1);

	// onConstruct
	Var::registerVar(VarDef("test", "initialvalue", nullptr, nullptr));
	EXPECT_EQ(Var::findVar("test")->strVal(), "env") << "Expected to get the value from the env";

	// load appname.vars
	Var::registerVar(VarDef("test", "file", nullptr, nullptr, CV_FROMFILE));
	EXPECT_EQ(Var::findVar("test")->strVal(), "env") << "Expected to still have the value from env";
	SDL_setenv("test", "", 1);
}

TEST_F(VarTest, testPriorityEnvOverrideFromCmd) {
	SDL_setenv("test", "env", 1);

	// onConstruct
	Var::registerVar(VarDef("test", "initialvalue", nullptr, nullptr));
	EXPECT_EQ(Var::findVar("test")->strVal(), "env") << "Expected to get the value from the env";

	// onConstruct argument parsing
	Var::registerVar(VarDef("test", "commandline", nullptr, nullptr, CV_FROMCOMMANDLINE));
	EXPECT_EQ(Var::findVar("test")->strVal(), "commandline") << "Commandline should have the highest priority";
	SDL_setenv("test", "", 1);
}

TEST_F(VarTest, testHistory) {
	const VarPtr& v = Var::registerVar(VarDef("test", "nonsense", nullptr, nullptr));
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
	const VarPtr& v = Var::registerVar(VarDef("test", "nonsense", nullptr, nullptr));
	for (int i = 0; i < 120; ++i) {
		v->setVal(core::String::format("reasonable%i", i));
	}
	EXPECT_EQ("reasonable119", v->strVal());
}

TEST_F(VarTest, testIntMinMax) {
	const VarPtr& v = Var::registerVar(VarDef("test", 5, 0, 10, nullptr, nullptr));
	EXPECT_TRUE(v->hasMinMax());
	EXPECT_FLOAT_EQ(0.0f, v->minValue());
	EXPECT_FLOAT_EQ(10.0f, v->maxValue());
	EXPECT_EQ(5, v->intVal());
	EXPECT_TRUE(v->setVal(0));
	EXPECT_EQ(0, v->intVal());
	EXPECT_TRUE(v->setVal(10));
	EXPECT_EQ(10, v->intVal());
	EXPECT_FALSE(v->setVal(-1));
	EXPECT_EQ(10, v->intVal());
	EXPECT_FALSE(v->setVal(11));
	EXPECT_EQ(10, v->intVal());
}

TEST_F(VarTest, testFloatMinMax) {
	const VarPtr& v = Var::registerVar(VarDef("test", 5.0f, 0.0f, 10.0f, nullptr, nullptr));
	EXPECT_TRUE(v->hasMinMax());
	EXPECT_FLOAT_EQ(0.0f, v->minValue());
	EXPECT_FLOAT_EQ(10.0f, v->maxValue());
	EXPECT_FLOAT_EQ(5.0f, v->floatVal());
	EXPECT_TRUE(v->setVal(0.0f));
	EXPECT_FLOAT_EQ(0.0f, v->floatVal());
	EXPECT_TRUE(v->setVal(10.0f));
	EXPECT_FLOAT_EQ(10.0f, v->floatVal());
	EXPECT_FALSE(v->setVal(-0.1f));
	EXPECT_FLOAT_EQ(10.0f, v->floatVal());
	EXPECT_FALSE(v->setVal(10.1f));
	EXPECT_FLOAT_EQ(10.0f, v->floatVal());
}

TEST_F(VarTest, testNoMinMax) {
	const VarPtr& v = Var::registerVar(VarDef("test", 5, nullptr, nullptr));
	EXPECT_FALSE(v->hasMinMax());
	EXPECT_TRUE(v->setVal(1000));
	EXPECT_TRUE(v->setVal(-1000));
}

TEST_F(VarTest, testMinMaxReRegister) {
	const VarPtr& v = Var::registerVar(VarDef("test", 5, nullptr, nullptr));
	EXPECT_FALSE(v->hasMinMax());
	Var::registerVar(VarDef("test", 5, 0, 10, nullptr, nullptr));
	EXPECT_TRUE(v->hasMinMax());
	EXPECT_FALSE(v->setVal(11));
}

TEST_F(VarTest, testEnumValidValues) {
	const core::DynamicArray<core::String> validValues = {"low", "medium", "high"};
	const VarPtr& v = Var::registerVar(VarDef("test", "medium", validValues, nullptr, nullptr));
	EXPECT_EQ("medium", v->strVal());
	EXPECT_EQ(3u, v->validValues().size());
	EXPECT_TRUE(v->setVal("low"));
	EXPECT_EQ("low", v->strVal());
	EXPECT_TRUE(v->setVal("high"));
	EXPECT_EQ("high", v->strVal());
	EXPECT_FALSE(v->setVal("invalid"));
	EXPECT_EQ("high", v->strVal());
}

TEST_F(VarTest, testEnumEmptyValueAllowed) {
	const core::DynamicArray<core::String> validValues = {"a", "b"};
	const VarPtr& v = Var::registerVar(VarDef("test", "a", validValues, nullptr, nullptr));
	EXPECT_TRUE(v->setVal(""));
}

TEST_F(VarTest, testEnumReRegister) {
	const VarPtr& v = Var::registerVar(VarDef("test", "a", nullptr, nullptr));
	EXPECT_TRUE(v->validValues().empty());
	const core::DynamicArray<core::String> validValues = {"a", "b", "c"};
	Var::registerVar(VarDef("test", "a", validValues, nullptr, nullptr));
	EXPECT_EQ(3u, v->validValues().size());
	EXPECT_FALSE(v->setVal("d"));
	EXPECT_TRUE(v->setVal("c"));
}

}
