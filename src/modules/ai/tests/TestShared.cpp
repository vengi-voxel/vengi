#include "TestShared.h"

void TestSuite::SetUp() {
	core::AbstractTest::SetUp();
	ai::randomSeed(0);
}

void TestSuite::TearDown() {
	core::AbstractTest::TearDown();
}
