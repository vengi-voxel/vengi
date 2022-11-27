/**
 * @file
 */

#pragma once

#include "app/tests/AbstractTest.h"
#include "voxel/MaterialColor.h"
#include "math/Random.h"
#include "voxel/Region.h"
#include <glm/geometric.hpp>

namespace voxel {

class AbstractVoxelTest: public app::AbstractTest {
protected:
	math::Random _random;
	long _seed = 0;
	const voxel::Region _region { glm::ivec3(0), glm::ivec3(63) };

public:
	void SetUp() override {
		app::AbstractTest::SetUp();
		_random.setSeed(_seed);
	}

	void TearDown() override {
		app::AbstractTest::TearDown();
	}
};

}
