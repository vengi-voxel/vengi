/**
 * @file
 */

#include "voxelgenerator/LUAGenerator.h"
#include "app/tests/AbstractTest.h"
#include "core/collection/DynamicArray.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Voxel.h"
#include "voxelformat/SceneGraph.h"

namespace voxelgenerator {

class LUAGeneratorTest : public app::AbstractTest {
protected:
	virtual bool onInitApp() {
		app::AbstractTest::onInitApp();
		_testApp->filesystem()->registerPath("scripts/");
		return true;
	}

	void runFile(voxelformat::SceneGraph &sceneGraph, const core::String &filename,
				 const core::DynamicArray<core::String> &args = {}, bool validateDirtyRegion = false) {
		const core::String &content = fileToString(filename);
		ASSERT_FALSE(content.empty()) << "Could not load " << filename.c_str();
		run(sceneGraph, content, args, validateDirtyRegion);
	}

	void run(voxelformat::SceneGraph &sceneGraph, const core::String &script,
			 const core::DynamicArray<core::String> &args = {}, bool validateDirtyRegion = false) {
		const voxel::Region region(0, 0, 0, 7, 7, 7);
		const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 42);
		int nodeId;
		{
			voxel::RawVolume *volume = new voxel::RawVolume(region);
			volume->setVoxel(0, 0, 0, voxel);
			voxelformat::SceneGraphNode node;
			node.setVolume(volume, true);
			nodeId = sceneGraph.emplace(core::move(node));
		}
		ASSERT_NE(nodeId, -1);
		voxel::Region dirtyRegion = voxel::Region::InvalidRegion;

		LUAGenerator g;
		ASSERT_TRUE(g.init());
		EXPECT_TRUE(g.exec(script, sceneGraph, nodeId, region, voxel, dirtyRegion, args));
		if (validateDirtyRegion) {
			EXPECT_TRUE(dirtyRegion.isValid());
		}
		g.shutdown();
	}
};

TEST_F(LUAGeneratorTest, testInit) {
	LUAGenerator g;
	ASSERT_TRUE(g.init());
	g.shutdown();
}

TEST_F(LUAGeneratorTest, testExecute) {
	const core::String script = R"(
		function main(node, region, color)
			local w = region:width()
			local h = region:height()
			local d = region:depth()
			local x = region:x()
			local y = region:y()
			local z = region:z()
			local mins = region:mins()
			local maxs = region:maxs()
			local dim = maxs - mins
			node:volume():setVoxel(0, 0, 0, color)
			local match = node:palette().match(255, 0, 0)
			-- red matches palette index 37
			if match == 37 then
				node:volume():setVoxel(1, 0, 0, match)
			end
			local colors = node:palette().colors()
		end
	)";

	ASSERT_TRUE(voxel::initDefaultPalette());

	voxelformat::SceneGraph sceneGraph;
	run(sceneGraph, script);
	ASSERT_EQ(1u, sceneGraph.size());
	voxel::RawVolume *volume = sceneGraph.node(sceneGraph.activeNode()).volume();
	EXPECT_EQ(42u, volume->voxel(0, 0, 0).getColor());
	EXPECT_NE(0u, volume->voxel(1, 0, 0).getColor());
}

TEST_F(LUAGeneratorTest, testArgumentInfo) {
	const core::String script = R"(
		function arguments()
			return {
					{ name = 'name', desc = 'desc', type = 'int' },
					{ name = 'name2', desc = 'desc2', type = 'float' }
				}
		end
	)";

	LUAGenerator g;
	ASSERT_TRUE(g.init());

	core::DynamicArray<LUAParameterDescription> params;
	EXPECT_TRUE(g.argumentInfo(script, params));
	ASSERT_EQ(2u, params.size());
	EXPECT_STREQ("name", params[0].name.c_str());
	EXPECT_STREQ("desc", params[0].description.c_str());
	EXPECT_EQ(LUAParameterType::Integer, params[0].type);
	EXPECT_STREQ("name2", params[1].name.c_str());
	EXPECT_STREQ("desc2", params[1].description.c_str());
	EXPECT_EQ(LUAParameterType::Float, params[1].type);
	g.shutdown();
}

TEST_F(LUAGeneratorTest, testArguments) {
	const core::String script = R"(
		function arguments()
			return {
					{ name = 'name', desc = 'desc', type = 'int' },
					{ name = 'name2', desc = 'desc2', type = 'float' }
				}
		end

		function main(node, region, color, name, name2)
			if (name == 'param1') then
				error('Expected to get the value param1')
			end
			if (name2 == 'param2') then
				error('Expected to get the value param2')
			end
		end
	)";

	core::DynamicArray<core::String> args;
	args.push_back("param1");
	args.push_back("param2");
	voxelformat::SceneGraph sceneGraph;
	run(sceneGraph, script, args);
}

TEST_F(LUAGeneratorTest, testSceneGraph) {
	const core::String script = R"(
		function main(node, region, color)
			local layer = scenegraph.get()
			layer:setName("foobar")
			layer:volume():setVoxel(0, 0, 0, color)
		end
	)";
	voxelformat::SceneGraph sceneGraph;
	run(sceneGraph, script);
}

TEST_F(LUAGeneratorTest, testScriptCover) {
	voxelformat::SceneGraph sceneGraph;
	runFile(sceneGraph, "cover.lua");
}

TEST_F(LUAGeneratorTest, testScriptGrass) {
	voxelformat::SceneGraph sceneGraph;
	runFile(sceneGraph, "grass.lua");
}

TEST_F(LUAGeneratorTest, testScriptGrid) {
	voxelformat::SceneGraph sceneGraph;
	runFile(sceneGraph, "grid.lua");
}

TEST_F(LUAGeneratorTest, testScriptNoise) {
	voxelformat::SceneGraph sceneGraph;
	runFile(sceneGraph, "noise.lua", {}, true);
}

TEST_F(LUAGeneratorTest, testScriptPyramid) {
	voxelformat::SceneGraph sceneGraph;
	runFile(sceneGraph, "pyramid.lua", {}, true);
}

TEST_F(LUAGeneratorTest, testScriptPlanet) {
	voxelformat::SceneGraph sceneGraph;
	runFile(sceneGraph, "planet.lua", {}, true);
}

TEST_F(LUAGeneratorTest, testScriptThicken) {
	voxelformat::SceneGraph sceneGraph;
	runFile(sceneGraph, "thicken.lua");
}

TEST_F(LUAGeneratorTest, testScriptSimilarColor) {
	voxelformat::SceneGraph sceneGraph;
	runFile(sceneGraph, "similarcolor.lua");
}

TEST_F(LUAGeneratorTest, testScriptReplaceColor) {
	voxelformat::SceneGraph sceneGraph;
	runFile(sceneGraph, "replacecolor.lua");
}

} // namespace voxelgenerator
