/**
 * @file
 */

#pragma once

#include "app/tests/AbstractTest.h"
#include "math/Random.h"
#include "voxelformat/FormatConfig.h"
#include "voxel/Region.h"
#include "math/Vec3i.h"

namespace voxel {

class AbstractVoxelTest: public app::AbstractTest {
protected:
	math::Random _random;
	unsigned int _seed = 0;
	const voxel::Region _region { math::Vec3i(0), math::Vec3i(63) };

public:
	AbstractVoxelTest(size_t threadPoolSize = 1) : app::AbstractTest(threadPoolSize) {
	}

	void SetUp() override {
		app::AbstractTest::SetUp();
		voxelformat::FormatConfig::init();
		_random.setSeed(_seed);
	}

	void TearDown() override {
		app::AbstractTest::TearDown();
	}
};

}
