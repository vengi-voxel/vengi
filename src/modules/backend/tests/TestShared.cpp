#include "TestShared.h"
#include "backend/entity/ai/common/Random.h"

void TestSuite::SetUp() {
	app::AbstractTest::SetUp();
	backend::randomSeed(0);
}

void TestSuite::TearDown() {
	app::AbstractTest::TearDown();
}
