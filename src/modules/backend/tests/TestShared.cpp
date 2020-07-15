#include "TestShared.h"
#include "backend/entity/ai/common/Random.h"

void TestSuite::SetUp() {
	core::AbstractTest::SetUp();
	backend::randomSeed(0);
}

void TestSuite::TearDown() {
	core::AbstractTest::TearDown();
}
