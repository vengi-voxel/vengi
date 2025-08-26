/**
 * @file
 */

#include "voxelgenerator/LUAApi.h"
#include "app/tests/AbstractTest.h"
#include "core/collection/DynamicArray.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxelformat/FormatConfig.h"

namespace voxelgenerator {

const size_t InitialSceneGraphModelSize = 2u;

class LUAApiTest : public app::AbstractTest {
protected:
	const voxel::Region _region{0, 0, 0, 7, 7, 7};
	const glm::ivec3 voxels[6] {
		glm::ivec3(0, 0, 0),
		glm::ivec3(0, 1, 0),
		glm::ivec3(0, 2, 0),
		glm::ivec3(2, 0, 0),
		glm::ivec3(2, 1, 0),
		glm::ivec3(2, 2, 0)
	};

	virtual bool onInitApp() {
		app::AbstractTest::onInitApp();
		return _testApp->filesystem()->registerPath("scripts/");
	}

	void runFile(scenegraph::SceneGraph &sceneGraph, const core::String &filename,
				 const core::DynamicArray<core::String> &args = {}, bool validateDirtyRegion = false) {
		const core::String &content = fileToString(filename);
		ASSERT_FALSE(content.empty()) << "Could not load " << filename.c_str();
		run(sceneGraph, content, args, validateDirtyRegion);
	}

	void run(scenegraph::SceneGraph &sceneGraph, const core::String &script,
			 const core::DynamicArray<core::String> &args = {}, bool validateDirtyRegion = false) {
		const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 42);
		int nodeId;
		{
			voxel::RawVolume *volume = new voxel::RawVolume(_region);
			for (int i = 0; i < lengthof(voxels); ++i) {
				volume->setVoxel(voxels[i], voxel);
			}
			scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
			node.setVolume(volume, true);
			node.setName("belt");
			nodeId = sceneGraph.emplace(core::move(node));
		}
		ASSERT_NE(nodeId, InvalidNodeId);
		{
			scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
			node.setVolume(new voxel::RawVolume(_region), true);
			node.setName("head");
			sceneGraph.emplace(core::move(node), nodeId);
		}

		LUAApi g(_testApp->filesystem());
		ASSERT_TRUE(g.init());
		EXPECT_TRUE(g.exec(script, sceneGraph, nodeId, _region, voxel, args));
		while (g.scriptStillRunning()) {
			_testApp->onFrame();
			if (_testApp->shouldQuit()) {
				break;
			}
			const ScriptState state = g.update(0.0001);
			EXPECT_NE(ScriptState::Error, state);
			EXPECT_NE(ScriptState::Inactive, state);
		}
		if (validateDirtyRegion) {
			const voxel::Region &dirtyRegion = g.dirtyRegion();
			EXPECT_TRUE(dirtyRegion.isValid());
		}
		g.shutdown();
	}
};

TEST_F(LUAApiTest, testInit) {
	LUAApi g(_testApp->filesystem());
	ASSERT_TRUE(g.init());
	g.shutdown();
}

TEST_F(LUAApiTest, testExecute) {
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
			local match = node:palette():match(255, 0, 0)
			-- red matches palette index 37
			if match == 37 then
				node:volume():setVoxel(1, 0, 0, match)
			end
			local colors = node:palette():colors()
			local newpal = g_palette.new()
			newpal:load("built-in:minecraft")
			node:setPalette(newpal)
		end
	)";

	scenegraph::SceneGraph sceneGraph;
	run(sceneGraph, script);
	ASSERT_EQ(InitialSceneGraphModelSize, sceneGraph.size());
	voxel::RawVolume *volume = sceneGraph.node(sceneGraph.activeNode()).volume();
	EXPECT_EQ(42u, volume->voxel(0, 0, 0).getColor());
	EXPECT_NE(0u, volume->voxel(1, 0, 0).getColor());
}

TEST_F(LUAApiTest, testYield) {
	const core::String script = R"(
		function main(node, region, color)
			for i = 1, 5 do
				g_log.debug("Lua: Running step " .. i)
				coroutine.yield()
			end
		end
	)";

	scenegraph::SceneGraph sceneGraph;
	run(sceneGraph, script);
}

TEST_F(LUAApiTest, testArgumentInfo) {
	const core::String script = R"(
		function arguments()
			return {
					{ name = 'name', desc = 'desc', type = 'int' },
					{ name = 'name2', desc = 'desc2', type = 'float' }
				}
		end
	)";

	LUAApi g(_testApp->filesystem());
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

TEST_F(LUAApiTest, testArguments) {
	const core::String script = R"(
		function arguments()
			return {
					{ name = 'name', desc = 'desc', type = 'int' },
					{ name = 'name2', desc = 'desc2', type = 'float' }
				}
		end

		function main(node, region, color, name, name2)
			if name == 'param1' then
				error('Expected to get the value param1')
			end
			if name2 == 'param2' then
				error('Expected to get the value param2')
			end
		end
	)";

	core::DynamicArray<core::String> args;
	args.push_back("param1");
	args.push_back("param2");
	scenegraph::SceneGraph sceneGraph;
	run(sceneGraph, script, args);
}

TEST_F(LUAApiTest, testSceneGraph) {
	const core::String script = R"(
		function main(node, region, color)
			local model = g_scenegraph.get()
			model:setName("foobar")
			model:volume():setVoxel(0, 0, 0, color)
		end
	)";
	scenegraph::SceneGraph sceneGraph;
	run(sceneGraph, script);
}

TEST_F(LUAApiTest, testImageAsPlane) {
	const core::String script = R"(
		function main(node, region, color)
			local pal = g_palette.new()
			pal:load("built-in:minecraft")
			local stream = g_io.sysopen("test-heightmap.png")
			local image = g_import.image("test-heightmap.png", stream)
			g_import.imageAsPlane(image, pal)
		end
	)";
	scenegraph::SceneGraph sceneGraph;
	run(sceneGraph, script);
}

TEST_F(LUAApiTest, testKeyFrames) {
	const core::String script = R"(
		function main(node, region, color)
			local kf = node:keyFrame(0)
			if kf:frame() ~= 0 then
				error('Expected frame 0')
			end
			if not g_scenegraph.addAnimation("test") then
				error('Failed to add animation')
			end
			if not g_scenegraph.setAnimation("test") then
				error('Failed to activate animation')
			end
		end
	)";
	scenegraph::SceneGraph sceneGraph;
	run(sceneGraph, script);
}

TEST_F(LUAApiTest, testSceneGraphNewNode) {
	const core::String script = R"(
		function main(node, region, color)
			local region = g_region.new(0, 0, 0, 1, 1, 1)
			local model = g_scenegraph.new("test", region, false)
			model:setName("foobar")
			model:volume():setVoxel(0, 0, 0, color)
		end
	)";
	scenegraph::SceneGraph sceneGraph;
	run(sceneGraph, script);
}

TEST_F(LUAApiTest, DISABLED_testDownloadAndImport) {
	voxelformat::FormatConfig::init();
	scenegraph::SceneGraph sceneGraph;
	const core::String script = R"(
		function main(node, region, color)
			local stream = g_http.get('https://raw.githubusercontent.com/vengi-voxel/vengi/master/data/tests/rgb.qb')
			if stream == nil then
				error('Failed to download')
			end
			g_import.scene('test.qb', stream)
		end
	)";
	run(sceneGraph, script);
}

TEST_F(LUAApiTest, testScriptCompaction) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "compaction.lua");
}

TEST_F(LUAApiTest, testScriptCover) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "cover.lua");
}

TEST_F(LUAApiTest, testScriptDeleteRGB) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "delete-rgb.lua");
}

TEST_F(LUAApiTest, testScriptErode) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "erode.lua");
}

TEST_F(LUAApiTest, testScriptFillHollow) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "fillhollow.lua");
}

TEST_F(LUAApiTest, testScriptFillz) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "fillz.lua");
}

TEST_F(LUAApiTest, testScriptGradient) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "gradient.lua");
}

TEST_F(LUAApiTest, testScriptGrass) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "grass.lua");
}

TEST_F(LUAApiTest, testScriptGrid) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "grid.lua");
}

TEST_F(LUAApiTest, testScriptMaze) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "maze.lua");
}

TEST_F(LUAApiTest, testScriptMove) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "move.lua", {"1", "1", "1"});
	scenegraph::SceneGraphNode *model = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, model);
	for (int i = 0; i < lengthof(voxels); ++i) {
		EXPECT_TRUE(voxel::isBlocked(model->volume()->voxel(voxels[i] + 1).getMaterial()));
		EXPECT_TRUE(voxel::isAir(model->volume()->voxel(voxels[i]).getMaterial()));
	}
}

TEST_F(LUAApiTest, testScriptNoiseBuiltin) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "noise-builtin.lua", {}, true);
}

// requires a meshy api key https://www.meshy.ai/
TEST_F(LUAApiTest, DISABLED_testScriptMeshy) {
	voxelformat::FormatConfig::init();
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "meshy.lua", {}, false);
}

TEST_F(LUAApiTest, testScriptNoise) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "noise.lua", {}, true);
}

TEST_F(LUAApiTest, testScriptPlanet) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "planet.lua", {}, true);
}

TEST_F(LUAApiTest, testScriptPyramid) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "pyramid.lua", {}, true);
}

TEST_F(LUAApiTest, testScriptReplaceColor) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "replacecolor.lua");
}

TEST_F(LUAApiTest, testScriptFlatten) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "flatten.lua");
}

TEST_F(LUAApiTest, testScriptReplacePalette) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "replacepalette.lua", {"built-in:minecraft"});
}

TEST_F(LUAApiTest, testScriptResize) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "resize.lua");
	scenegraph::SceneGraphNode *model = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, model);
	const voxel::Region &region = model->region();
	EXPECT_EQ(region.getLowerCorner(), _region.getLowerCorner());
	EXPECT_EQ(region.getUpperCorner(), _region.getUpperCorner() + 1);
}

TEST_F(LUAApiTest, testScriptSimilarColor) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "similarcolor.lua");
}

TEST_F(LUAApiTest, testScriptSlice) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "slice.lua", {"1", "1", "1"});
	const uint32_t slices = _region.getWidthInVoxels() * _region.getHeightInVoxels() * _region.getDepthInVoxels();
	EXPECT_EQ(slices + InitialSceneGraphModelSize, sceneGraph.size(scenegraph::SceneGraphNodeType::Model));
}

TEST_F(LUAApiTest, testScriptSplitColor) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "splitcolor.lua");
	EXPECT_EQ(InitialSceneGraphModelSize + 2u, sceneGraph.nodeSize());
}

TEST_F(LUAApiTest, testScriptSplitObjects) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "splitobjects.lua");
}

TEST_F(LUAApiTest, testScriptAnimate) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "animate.lua");
}

TEST_F(LUAApiTest, testScriptThicken) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "thicken.lua");
}

TEST_F(LUAApiTest, testScriptAlign) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "align.lua");
}

TEST_F(LUAApiTest, testScriptMandelbulb) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "mandelbulb.lua");
}

TEST_F(LUAApiTest, testScriptSmooth) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "smooth.lua");
}

TEST_F(LUAApiTest, testScriptRemapColors) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "remapcolors.lua");
}

TEST_F(LUAApiTest, testScriptGameoflife) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "gameoflife.lua");
}

TEST_F(LUAApiTest, testScriptNewelStair) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "newelstair.lua");
}

TEST_F(LUAApiTest, testScriptGenland) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "genland.lua", {"0", "64"});
}

TEST_F(LUAApiTest, testScriptShadow) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "shadow.lua");
}

// long runtime - thus disabled
TEST_F(LUAApiTest, DISABLED_testScriptAceOfSpadesRainbowTowers) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "aos_rainbow_towers.lua");
}

// long runtime - thus disabled
TEST_F(LUAApiTest, DISABLED_testScriptAceOfSpadesForestRiver) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "aos_forest_river.lua");
}

// long runtime - thus disabled
TEST_F(LUAApiTest, DISABLED_testScriptAceOfSpadesIncompleteFortress) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "aos_incomplete_fortress.lua");
}

// long runtime - thus disabled
TEST_F(LUAApiTest, DISABLED_testScriptAceOfSpadesSlope) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "aos_slope.lua");
}

// long runtime - thus disabled
TEST_F(LUAApiTest, DISABLED_testScriptAceOfSpadesCaves) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "aos_caves.lua");
}

TEST_F(LUAApiTest, testScriptImageAsVolume) {
	{
		scenegraph::SceneGraph sceneGraph;
		runFile(sceneGraph, "imageasvolume.lua", {"test-heightmap.png"});
	}
	{
		scenegraph::SceneGraph sceneGraph;
		runFile(sceneGraph, "imageasvolume.lua", {"test-heightmap.png", "test-heightmap-dm.png"});
	}
}

} // namespace voxelgenerator
