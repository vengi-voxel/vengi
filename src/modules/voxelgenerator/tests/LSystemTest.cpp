/**
 * @file
 */

#include "voxelgenerator/LSystem.h"
#include "app/tests/AbstractTest.h"
#include "color/RGBA.h"
#include "palette/Palette.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxelutil/VolumeVisitor.h"

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

static void generate(const LSystemConfig &config, voxel::RawVolume &volume, LSystemExecutionState &exec) {
	LSystemState state;
	prepareState(config, state);
	voxel::RawVolumeWrapper wrapper(&volume);
	const voxel::Voxel wood = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	while (step(wrapper, wood, state, exec)) {
	}
}

TEST_F(LSystemTests, testTemplates) {
	const core::DynamicArray<LSystemTemplate> &templates = defaultTemplates();
	ASSERT_FALSE(templates.empty());
	for (const LSystemTemplate &t : templates) {
		SCOPED_TRACE(t.name.c_str());
		LSystemState state;
		prepareState(t.config, state);
		ASSERT_FALSE(state.sentence.empty());
		int depth = 0;
		for (char c : state.sentence) {
			if (c == '[') {
				++depth;
			} else if (c == ']') {
				ASSERT_GT(depth, 0);
				--depth;
			}
		}
		ASSERT_EQ(depth, 0);
		// Exercise the real preset, with a distinct foliage index.
		LSystemConfig config = t.config;
		config.leafColor = 2;
		voxel::RawVolume volume(voxel::Region(-128, -16, -128, 128, 255, 128));
		LSystemExecutionState exec;
		generate(config, volume, exec);
		ASSERT_TRUE(exec.stack.empty());
		ASSERT_GT(voxelutil::countVoxels(volume), 0);
		if (t.name == "Pine Tree" || t.name == "Conifer-like" || t.name == "Natural Branching Tree") {
			int wood = 0;
			int leaves = 0;
			voxelutil::visitVolume(volume, [&](int, int, int, const voxel::Voxel &v) {
				wood += v.getColor() == 1;
				leaves += v.getColor() == 2;
			});
			ASSERT_GT(wood, 0);
			ASSERT_GT(leaves, 0);
		}
	}
}

TEST_F(LSystemTests, testBambooLeafPositions) {
	for (const auto &t : defaultTemplates()) {
		if (t.name != "Bamboo") {
			continue;
		}
		auto config = t.config;
		config.iterations = 1;
		config.leafRadius = 0.0f;
		LSystemState state;
		prepareState(config, state);
		voxel::RawVolume volume(voxel::Region(-32, 32));
		voxel::RawVolumeWrapper wrapper(&volume);
		LSystemExecutionState exec;
		core::DynamicArray<glm::vec3> leafPositions;
		while (exec.index < state.sentence.size()) {
			if (state.sentence[exec.index] == 'L') {
				leafPositions.push_back(exec.step.pos);
			}
			step(wrapper, voxel::createVoxel(voxel::VoxelType::Generic, 1), state, exec);
		}
		ASSERT_EQ(leafPositions.size(), 4u);
		for (size_t i = 0; i < leafPositions.size(); ++i) {
			EXPECT_GT(glm::length(glm::vec2(leafPositions[i].x, leafPositions[i].z)), 0.5f);
			for (size_t j = 0; j < i; ++j) {
				EXPECT_GT(glm::distance(leafPositions[i], leafPositions[j]), 0.5f);
			}
		}
		return;
	}
	FAIL() << "Missing Bamboo template";
}

TEST_F(LSystemTests, testFernHasDepth) {
	for (const auto &t : defaultTemplates()) {
		if (t.name != "Fern 3D") {
			continue;
		}
		auto config = t.config;
		config.width = 1.0f;
		config.leafRadius = 0.0f;
		voxel::RawVolume volume(voxel::Region(-64, -8, -64, 64, 128, 64));
		LSystemExecutionState exec;
		generate(config, volume, exec);
		int outsidePlane = 0;
		voxelutil::visitVolume(volume, [&](int, int, int z, const voxel::Voxel &) {
			outsidePlane += z != 0;
		});
		EXPECT_GT(outsidePlane, 0);
		return;
	}
	FAIL() << "Missing Fern 3D template";
}

TEST_F(LSystemTests, testFractionalLength) {
	LSystemConfig config;
	config.axiom = "FFb";
	config.length = 1.5f;
	voxel::RawVolume volume(voxel::Region(-8, 8));
	LSystemExecutionState exec;
	generate(config, volume, exec);
	EXPECT_FLOAT_EQ(exec.step.pos.y, 1.5f);
	for (int y = 0; y <= 3; ++y) {
		EXPECT_FALSE(voxel::isAir(volume.voxel(0, y, 0).getMaterial()));
	}
}

TEST_F(LSystemTests, testDiagonalFractionalSegment) {
	LSystemConfig config;
	config.axiom = "+F";
	config.angle = glm::radians(45.0f);
	config.length = 2.5f;
	voxel::RawVolume volume(voxel::Region(-8, 8));
	LSystemExecutionState exec;
	generate(config, volume, exec);
	EXPECT_NEAR(exec.step.pos.x, -glm::sqrt(0.5f) * 2.5f, 0.0001f);
	EXPECT_NEAR(exec.step.pos.y, glm::sqrt(0.5f) * 2.5f, 0.0001f);
	for (int i = 0; i <= 2; ++i) {
		EXPECT_FALSE(voxel::isAir(volume.voxel(-i, i, 0).getMaterial()));
	}
}

TEST_F(LSystemTests, testTaperReachesSingleVoxelWidth) {
	LSystemConfig config;
	config.axiom = "!!!!!F";
	config.width = 2.0f;
	voxel::RawVolume volume(voxel::Region(-8, 8));
	LSystemExecutionState exec;
	generate(config, volume, exec);
	EXPECT_FLOAT_EQ(exec.step.width, 1.0f);
	EXPECT_EQ(voxelutil::countVoxels(volume), 2);
}

TEST_F(LSystemTests, testBranchRestoresLengthAndWidth) {
	LSystemConfig config;
	config.axiom = "F[!+F]F";
	config.length = 4.0f;
	config.lengthScale = 0.5f;
	config.minLength = 1.5f;
	config.width = 4.0f;
	voxel::RawVolume volume(voxel::Region(-16, 16));
	LSystemExecutionState exec;
	generate(config, volume, exec);
	EXPECT_FLOAT_EQ(exec.step.pos.y, 6.0f);
	EXPECT_FLOAT_EQ(exec.step.pos.x, 0.0f);
	EXPECT_FLOAT_EQ(exec.step.width, 4.0f);
	EXPECT_FLOAT_EQ(exec.step.length, 1.5f);
	EXPECT_TRUE(exec.stack.empty());
}

TEST_F(LSystemTests, testLeafColorDoesNotChangeWood) {
	LSystemConfig config;
	config.axiom = "LFF";
	config.length = 4.0f;
	config.leafRadius = 1.0f;
	config.leafColor = 2;
	voxel::RawVolume volume(voxel::Region(-16, 16));
	LSystemExecutionState exec;
	generate(config, volume, exec);
	EXPECT_EQ(volume.voxel(0, 7, 0).getColor(), 1);
	EXPECT_EQ(exec.step.voxel.getColor(), 1);
	int leaves = 0;
	voxelutil::visitVolume(volume, [&](int, int, int, const voxel::Voxel &v) {
		leaves += v.getColor() == 2;
	});
	EXPECT_GT(leaves, 0);
}

TEST_F(LSystemTests, testSeededGeneration) {
	LSystemConfig config;
	config.axiom = "F[+FL][-FL]>FL";
	config.length = 5.0f;
	config.angleVariation = glm::radians(15.0f);
	config.leafRadius = 2.0f;
	config.leafColor = 2;
	voxel::RawVolume first(voxel::Region(-24, 24));
	voxel::RawVolume second(first.region());
	voxel::RawVolume different(first.region());
	LSystemExecutionState firstExec, secondExec, differentExec;
	generate(config, first, firstExec);
	generate(config, second, secondExec);
	++config.seed;
	generate(config, different, differentExec);
	int differences = 0;
	for (int z = -24; z <= 24; ++z) {
		for (int y = -24; y <= 24; ++y) {
			for (int x = -24; x <= 24; ++x) {
				ASSERT_EQ(first.voxel(x, y, z), second.voxel(x, y, z));
				differences += !(first.voxel(x, y, z) == different.voxel(x, y, z));
			}
		}
	}
	EXPECT_GT(differences, 0);
}

TEST_F(LSystemTests, testConstantLengthByDefault) {
	LSystemConfig config;
	config.axiom = "F[+F]F";
	config.length = 0.25f;
	voxel::RawVolume volume(voxel::Region(-8, 8));
	LSystemExecutionState exec;
	generate(config, volume, exec);
	EXPECT_FLOAT_EQ(exec.step.length, 0.25f);
	EXPECT_FLOAT_EQ(exec.step.pos.y, 0.5f);
}

TEST_F(LSystemTests, testEvaluateRegionMatchesGeneration) {
	LSystemConfig config;
	config.axiom = "F[+FL]F";
	config.length = 4.0f;
	config.width = 3.0f;
	config.leafRadius = 2.0f;
	config.leafColor = 2;
	config.position = glm::ivec3(2, 3, 4);
	voxel::RawVolume volume(voxel::Region(-32, 32));
	LSystemExecutionState exec;
	generate(config, volume, exec);

	const voxel::Voxel wood = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	const voxel::Region bounds = evaluateRegion(config, wood);
	ASSERT_TRUE(bounds.isValid());

	voxel::Region occupied = voxel::Region::InvalidRegion;
	voxelutil::visitVolume(volume, [&](int x, int y, int z, const voxel::Voxel &v) {
		if (voxel::isAir(v.getMaterial())) {
			return;
		}
		if (occupied.isValid()) {
			occupied.accumulate(x, y, z);
		} else {
			occupied = voxel::Region(x, y, z, x, y, z);
		}
	});
	EXPECT_EQ(bounds, occupied);
}

TEST_F(LSystemTests, testAdoptToRegionFitsSimpleTree) {
	LSystemConfig config;
	for (const auto &t : defaultTemplates()) {
		if (t.name == "Simple Tree") {
			config = t.config;
			break;
		}
	}
	ASSERT_FALSE(config.axiom.empty());

	const voxel::Voxel wood = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	const voxel::Region target(0, 31);
	const voxel::Region unscaled = evaluateRegion(config, wood);
	ASSERT_TRUE(unscaled.isValid());
	ASSERT_FALSE(target.containsRegion(unscaled));

	ASSERT_TRUE(adoptToRegion(config, target));
	const voxel::Region bounds = evaluateRegion(config, wood);
	ASSERT_TRUE(bounds.isValid());
	EXPECT_TRUE(target.containsRegion(bounds)) << bounds.toString().c_str() << " not in " << target.toString().c_str();
	EXPECT_EQ(bounds.getLowerY(), target.getLowerY());
}

TEST_F(LSystemTests, testAdoptLiftsWidthBelowOrigin) {
	LSystemConfig config;
	config.axiom = "F";
	config.length = 4.0f;
	config.width = 5.0f;
	config.leafRadius = 0.0f;
	config.iterations = 0;
	const voxel::Region target(0, 15);
	ASSERT_TRUE(adoptToRegion(config, target));
	const voxel::Voxel wood = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	const voxel::Region bounds = evaluateRegion(config, wood);
	ASSERT_TRUE(bounds.isValid());
	EXPECT_TRUE(target.containsRegion(bounds));
	EXPECT_EQ(bounds.getLowerY(), target.getLowerY());
	EXPECT_GT(config.position.y, target.getLowerY());
}

TEST_F(LSystemTests, testDefaultLeafColorPicksGreen) {
	palette::Palette palette;
	ASSERT_TRUE(palette.nippon());
	const int idx = defaultLeafColor(palette);
	ASSERT_GE(idx, 0);
	ASSERT_LT(idx, palette.colorCount());
	EXPECT_EQ(idx, palette.getClosestMatch(color::RGBA(60, 140, 35)));
	const color::RGBA c = palette.color(idx);
	EXPECT_GE(c.g, c.r);
	EXPECT_GE(c.g, c.b);
}

TEST_F(LSystemTests, testDefaultLeafColorEmptyPalette) {
	palette::Palette palette;
	EXPECT_EQ(-1, defaultLeafColor(palette));
}

TEST_F(LSystemTests, testLeafColorZeroIsUsed) {
	LSystemConfig config;
	config.axiom = "L";
	config.leafRadius = 1.0f;
	config.leafColor = 0;
	config.iterations = 0;
	voxel::RawVolume volume(voxel::Region(-4, 4));
	LSystemExecutionState exec;
	generate(config, volume, exec);
	int leaves = 0;
	voxelutil::visitVolume(volume, [&](int, int, int, const voxel::Voxel &v) {
		leaves += v.getColor() == 0 && !voxel::isAir(v.getMaterial());
	});
	EXPECT_GT(leaves, 0);
}

TEST_F(LSystemTests, testInvalidRule) {
	const core::DynamicArray<LSystemTemplate> &templates = defaultTemplates();
	LSystemConfig conf;
	conf = templates[0].config;
	conf.rules[1].b += "(1";
	conf.iterations = 2;
	LSystemState state;
	prepareState(conf, state);
	ASSERT_FALSE(state.sentence.empty());
	voxel::Region r(0, 10);
	voxel::RawVolume v(r);
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	LSystemExecutionState execState;
	int steps = 0;
	voxel::RawVolumeWrapper wrapper(&v);
	while (step(wrapper, voxel, state, execState)) {
		++steps;
		if (steps > 1000) {
			break;
		}
	}
	ASSERT_LT(steps, 1000);
}

} // namespace lsystem

} // namespace voxelgenerator
