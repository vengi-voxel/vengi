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

TEST_F(LUAApiTest, testImagePixelAccess) {
	const core::String script = R"(
		function main(node, region, color)
			local stream = g_io.sysopen("test-heightmap.png")
			local image = g_import.image("test-heightmap.png", stream)
			local w = image:width()
			local h = image:height()
			if w <= 0 then
				error('Expected width > 0, got ' .. w)
			end
			if h <= 0 then
				error('Expected height > 0, got ' .. h)
			end
			local r, g, b, a = image:rgba(0, 0)
			if r < 0 or r > 255 then
				error('Invalid red value: ' .. r)
			end
			if g < 0 or g > 255 then
				error('Invalid green value: ' .. g)
			end
			if b < 0 or b > 255 then
				error('Invalid blue value: ' .. b)
			end
			if a < 0 or a > 255 then
				error('Invalid alpha value: ' .. a)
			end
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

TEST_F(LUAApiTest, testNodePivot) {
	const core::String script = R"(
		function main(node, region, color)
			node:setPivot(0.5, 0.5, 0.5)
			local p = node:pivot()
			if math.abs(p.x - 0.5) > 0.001 then error('pivot x') end
			if math.abs(p.y - 0.5) > 0.001 then error('pivot y') end
			if math.abs(p.z - 0.5) > 0.001 then error('pivot z') end
		end
	)";
	scenegraph::SceneGraph sceneGraph;
	run(sceneGraph, script);
}

TEST_F(LUAApiTest, testNodeSetPivotCompensation) {
	const core::String script = R"(
		function main(node, region, color)
			-- initial pivot is (0,0,0), local translation is (0,0,0)
			local kf = node:keyFrameForFrame(0)
			local t0 = kf:localTranslation()
			if math.abs(t0.x) > 0.001 or math.abs(t0.y) > 0.001 or math.abs(t0.z) > 0.001 then
				error('initial translation should be 0,0,0')
			end

			-- set pivot to center: (0.5, 0.5, 0.5)
			-- region is 8x8x8 so delta * size = (0.5*8, 0.5*8, 0.5*8) = (4, 4, 4)
			node:setPivot(0.5, 0.5, 0.5)

			-- translation should be compensated by (4, 4, 4)
			local kf2 = node:keyFrameForFrame(0)
			local t1 = kf2:localTranslation()
			if math.abs(t1.x - 4.0) > 0.001 then error('compensated x: ' .. t1.x) end
			if math.abs(t1.y - 4.0) > 0.001 then error('compensated y: ' .. t1.y) end
			if math.abs(t1.z - 4.0) > 0.001 then error('compensated z: ' .. t1.z) end
		end
	)";
	scenegraph::SceneGraph sceneGraph;
	run(sceneGraph, script);
}

TEST_F(LUAApiTest, testNodeNumKeyFrames) {
	const core::String script = R"(
		function main(node, region, color)
			local n = node:numKeyFrames()
			if n ~= 1 then error('expected 1 keyframe, got ' .. n) end
			node:addKeyFrame(10)
			n = node:numKeyFrames()
			if n ~= 2 then error('expected 2 keyframes, got ' .. n) end
		end
	)";
	scenegraph::SceneGraph sceneGraph;
	run(sceneGraph, script);
}

TEST_F(LUAApiTest, testNodeChildren) {
	const core::String script = R"(
		function main(node, region, color)
			local children = node:children()
			-- belt node has head as child (see test setup)
			if #children ~= 1 then error('expected 1 child, got ' .. #children) end
		end
	)";
	scenegraph::SceneGraph sceneGraph;
	run(sceneGraph, script);
}

TEST_F(LUAApiTest, testNodeRegion) {
	const core::String script = R"(
		function main(node, region, color)
			local r = node:region()
			if r:width() ~= 8 then error('expected width 8, got ' .. r:width()) end
			if r:height() ~= 8 then error('expected height 8, got ' .. r:height()) end
			if r:depth() ~= 8 then error('expected depth 8, got ' .. r:depth()) end
		end
	)";
	scenegraph::SceneGraph sceneGraph;
	run(sceneGraph, script);
}

TEST_F(LUAApiTest, testSceneGraphAnimations) {
	const core::String script = R"(
		function main(node, region, color)
			local anims = g_scenegraph.animations()
			if #anims < 1 then error('expected at least 1 animation') end
			g_scenegraph.addAnimation("extra")
			anims = g_scenegraph.animations()
			local found = false
			for _, name in ipairs(anims) do
				if name == "extra" then found = true end
			end
			if not found then error('animation extra not found') end
		end
	)";
	scenegraph::SceneGraph sceneGraph;
	run(sceneGraph, script);
}

TEST_F(LUAApiTest, testQuatSlerp) {
	const core::String script = R"(
		function main(node, region, color)
			local a = g_quat.new()
			local b = g_quat.rotateX(1.0)
			local mid = g_quat.slerp(a, b, 0.5)
			if mid.w == nil then error('slerp failed') end
		end
	)";
	scenegraph::SceneGraph sceneGraph;
	run(sceneGraph, script);
}

TEST_F(LUAApiTest, testQuatConjugate) {
	const core::String script = R"(
		function main(node, region, color)
			local q = g_quat.rotateY(1.0)
			local c = g_quat.conjugate(q)
			-- conjugate negates xyz, keeps w
			if math.abs(c.w - q.w) > 0.001 then error('conjugate w differs') end
			if math.abs(c.y + q.y) > 0.001 then error('conjugate y not negated') end
		end
	)";
	scenegraph::SceneGraph sceneGraph;
	run(sceneGraph, script);
}

TEST_F(LUAApiTest, testQuatFromAxisAngle) {
	const core::String script = R"(
		function main(node, region, color)
			local axis = g_ivec3.new(0, 1, 0)
			local q = g_quat.fromAxisAngle(axis, math.pi / 2.0)
			if q.w == nil then error('fromAxisAngle failed') end
			-- 90 degree rotation around Y: w should be ~0.707
			if math.abs(q.w - 0.707) > 0.01 then error('unexpected w: ' .. q.w) end
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

TEST_F(LUAApiTest, testScriptClouds) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "clouds.lua");
}

TEST_F(LUAApiTest, testScriptHouse) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "house.lua", {"6", "6", "1", "3", "gable", "0", "1", "1", "2", "1", "2", "false", "1", "1", "2", "3", "4", "5", "6", "7", "42"});
}

TEST_F(LUAApiTest, testScriptPaletteBrighten) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "palette-brighten.lua");
}

// long runtime - thus disabled
TEST_F(LUAApiTest, DISABLED_testScriptAceOfSpadesRainbowTowers) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "aos_rainbow-towers.lua");
}

// long runtime - thus disabled
TEST_F(LUAApiTest, DISABLED_testScriptAceOfSpadesForestRiver) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "aos_forest-river.lua");
}

// long runtime - thus disabled
TEST_F(LUAApiTest, DISABLED_testScriptAceOfSpadesIncompleteFortress) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "aos_incomplete-fortress.lua");
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

TEST_F(LUAApiTest, testTreeCube) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "tree_cube.lua", {"10", "2", "10", "10", "10", "1", "2"});
}

TEST_F(LUAApiTest, testTreeCubeSideCubes) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "tree_cubesidecubes.lua", {"10", "2", "10", "10", "10", "1", "2"});
}

TEST_F(LUAApiTest, testTreeDome) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "tree_dome.lua", {"10", "2", "10", "10", "10", "1", "2"});
}

TEST_F(LUAApiTest, testTreeDomeHanging) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "tree_domehanging.lua", {"10", "2", "10", "10", "10", "10", "5", "10", "1", "1", "2"});
}

TEST_F(LUAApiTest, testTreeCone) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "tree_cone.lua", {"10", "2", "10", "10", "10", "1", "2"});
}

TEST_F(LUAApiTest, testTreeEllipsis) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "tree_ellipsis.lua", {"10", "2", "10", "10", "10", "1", "2"});
}

TEST_F(LUAApiTest, testTreeBranchEllipsis) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "tree_branchellipsis.lua", {"10", "2", "10", "10", "10", "5", "5", "1", "2"});
}

TEST_F(LUAApiTest, testTreePalm) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "tree_palm.lua", {"10", "2", "0", "0", "5", "0.9", "10", "10", "5", "1.0", "5", "0.9", "2", "1", "2"});
}

TEST_F(LUAApiTest, testTreeFir) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "tree_fir.lua", {"10", "2", "5", "5", "2", "5.0", "1", "2", "0.5", "1", "2"});
}

TEST_F(LUAApiTest, testTreeBonsai) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "tree_bonsai.lua");
}

TEST_F(LUAApiTest, testPotPlant) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "potplant.lua");
}

TEST_F(LUAApiTest, testPotPlantFern) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "potplant.lua", {"fern", "12", "10", "3", "round", "4", "6", "5", "2", "4", "3", "1", "42"});
}

TEST_F(LUAApiTest, testPotPlantCactus) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "potplant.lua", {"cactus", "15", "8", "4", "square", "4", "6", "3", "2", "4", "3", "1", "42"});
}

TEST_F(LUAApiTest, testPotPlantFlower) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "potplant.lua", {"flower", "10", "8", "3", "tall", "4", "6", "7", "2", "4", "3", "1", "42"});
}

TEST_F(LUAApiTest, testPotPlantBush) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "potplant.lua", {"bush", "10", "10", "4", "bowl", "4", "6", "5", "2", "4", "3", "1", "42"});
}

TEST_F(LUAApiTest, testTreePine) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "tree_pine.lua", {"10", "2", "10", "10", "10", "2", "1", "2", "2", "1", "2"});
}

TEST_F(LUAApiTest, testTreeBlackWillow) {
	scenegraph::SceneGraph sceneGraph;
	runFile(sceneGraph, "tree_blackwillow.lua");
}

TEST_F(LUAApiTest, testPaletteExtendedBindings) {
	const core::String script = R"(
		function main(node, region, color)
			local pal = node:palette()
			local name = pal:name()
			local hash = pal:hash()
			local size = pal:size()
			assert(size > 0, "palette size should be > 0")

			-- test hasColor, tryAdd, colorName, setColorName
			pal:setColor(0, 255, 0, 0, 255)
			local hasRed = pal:hasColor(255, 0, 0)
			assert(hasRed, "palette should have red")

			pal:setColorName(0, "MyRed")
			local cname = pal:colorName(0)
			assert(cname == "MyRed", "color name should be MyRed")

			-- test palette name
			pal:setName("TestPalette")
			assert(pal:name() == "TestPalette", "palette name should be TestPalette")

			-- test hasAlpha, hasEmit, hasMaterials
			local alpha = pal:hasAlpha(0)
			local emit = pal:hasEmit(0)
			local mats = pal:hasMaterials()
			local freeSlot = pal:hasFreeSlot()

			-- test exchange and copy
			pal:setColor(1, 0, 255, 0, 255)
			pal:exchange(0, 1)
			local r, g, b, a = pal:rgba(0)
			assert(r == 0 and g == 255 and b == 0, "exchange should swap colors")
			pal:copy(0, 2)

			-- test brighter/darker/warmer/colder
			pal:brighter(0.1)
			pal:darker(0.1)
			pal:warmer(5)
			pal:colder(5)
			pal:changeIntensity(1.0)

			-- test contrastStretching and whiteBalance
			pal:contrastStretching()
			pal:whiteBalance()

			-- test setSize and fill
			pal:setSize(10)
			assert(pal:size() == 10, "size should be 10")
			pal:fill()

			-- test new palette
			local newpal = g_palette.new()
			newpal:setSize(4)
			newpal:setColor(0, 255, 0, 0, 255)
			newpal:setColor(1, 0, 255, 0, 255)
			newpal:setColor(2, 0, 0, 255, 255)
			newpal:setColor(3, 128, 128, 128, 255)
			local added, idx = newpal:tryAdd(64, 64, 64)
		end
	)";
	scenegraph::SceneGraph sceneGraph;
	run(sceneGraph, script);
}

TEST_F(LUAApiTest, testNormalPaletteBindings) {
	const core::String script = R"(
		function main(node, region, color)
			-- test creation
			local npal = g_normalpalette.new()
			npal:load("built-in:tiberiansun")
			assert(npal:size() > 0, "size should be > 0")

			-- test name
			npal:setName("TestNormals")
			assert(npal:name() == "TestNormals", "name should be TestNormals")

			-- test hash
			local hash = npal:hash()

			-- test set/get normal
			npal:setNormal(0, 0.0, 1.0, 0.0)
			local n = npal:normal(0)
			-- check that it's roughly (0, 1, 0)
			assert(n.y > 0.9, "normal y should be close to 1.0")

			-- test closest match
			local idx = npal:match(0.0, 1.0, 0.0)

			-- test tostring
			local str = tostring(npal)

			-- test node normal palette
			node:setNormalPalette(npal)
			assert(node:hasNormalPalette(), "node should have normal palette")
			local npal2 = node:normalPalette()
			assert(npal2:size() > 0, "node normal palette size should be > 0")
		end
	)";
	scenegraph::SceneGraph sceneGraph;
	run(sceneGraph, script);
}

TEST_F(LUAApiTest, testVolumeWrapperNormals) {
	const core::String script = R"(
		function main(node, region, color)
			local volume = node:volume()
			local mins = region:mins()
			local x = mins.x
			local y = mins.y
			local z = mins.z

			-- setVoxel with normal parameter
			volume:setVoxel(x, y, z, 1, 5)
			local c = volume:voxel(x, y, z)
			assert(c == 1, "color should be 1, got " .. tostring(c))
			local n = volume:normal(x, y, z)
			assert(n == 5, "normal should be 5, got " .. tostring(n))

			-- setNormal on existing voxel
			volume:setNormal(x, y, z, 10)
			n = volume:normal(x, y, z)
			assert(n == 10, "normal should be 10 after setNormal, got " .. tostring(n))

			-- setNormal on air voxel should return false
			local airx = x + 100
			local result = volume:setNormal(airx, y, z, 1)
			assert(not result, "setNormal on air should return false")

			-- normal of air voxel should be 0 (NO_NORMAL)
			n = volume:normal(airx, y, z)
			assert(n == 0, "normal of air should be 0, got " .. tostring(n))
		end
	)";
	scenegraph::SceneGraph sceneGraph;
	run(sceneGraph, script);
}

TEST_F(LUAApiTest, testVoxelUtilBindings) {
	const core::String script = R"(
		function main(node, region, color)
			local volume = node:volume()
			local mins = region:mins()
			local maxs = region:maxs()
			local x = mins.x
			local y = mins.y
			local z = mins.z

			-- test fill
			volume:fill(5)
			local c = volume:voxel(x, y, z)
			assert(c == 5, "fill color should be 5, got " .. tostring(c))

			-- test isEmpty after fill
			assert(not volume:isEmpty(), "volume should not be empty after fill")

			-- test clear
			volume:clear()
			c = volume:voxel(x, y, z)
			assert(c == -1, "voxel should be air (-1) after clear, got " .. tostring(c))

			-- test isEmpty after clear
			assert(volume:isEmpty(), "volume should be empty after clear")

			-- test isTouching
			volume:setVoxel(x, y, z, 1)
			local touching = volume:isTouching(x + 1, y, z)
			assert(touching, "position adjacent to voxel should be touching")

			-- test fill with overwrite=false
			volume:fill(10, false)
			c = volume:voxel(x, y, z)
			assert(c == 1, "fill with overwrite=false should not overwrite existing voxel, got " .. tostring(c))
		end
	)";
	scenegraph::SceneGraph sceneGraph;
	run(sceneGraph, script);
}

TEST_F(LUAApiTest, testVolumeMergeBinding) {
	const core::String script = R"(
		function main(node, region, color)
			local volume = node:volume()
			local mins = region:mins()
			local x = mins.x
			local y = mins.y
			local z = mins.z

			-- set a voxel in the source
			volume:setVoxel(x, y, z, 3)

			-- create a second node and merge into it
			local newNode = g_scenegraph.new("merge_target", region)
			local newVolume = newNode:volume()

			-- merge source into dest
			local count = newVolume:merge(volume)
			assert(count > 0, "merge should have copied at least 1 voxel, got " .. tostring(count))
			local c = newVolume:voxel(x, y, z)
			assert(c == 3, "merged voxel color should be 3, got " .. tostring(c))
		end
	)";
	scenegraph::SceneGraph sceneGraph;
	run(sceneGraph, script);
}

TEST_F(LUAApiTest, testVolumeRotateDegrees) {
	const core::String script = R"(
		function main(node, region, color)
			local volume = node:volume()
			local mins = region:mins()
			volume:setVoxel(mins.x, mins.y, mins.z, 1)
			volume:rotateDegrees(90, 0, 0)
		end
	)";
	scenegraph::SceneGraph sceneGraph;
	run(sceneGraph, script);
}

TEST_F(LUAApiTest, testVolumeScaleUp) {
	const core::String script = R"(
		function main(node, region, color)
			local volume = node:volume()
			local mins = region:mins()
			volume:setVoxel(mins.x, mins.y, mins.z, 1)
			volume:scaleUp()
			local r = volume:region()
			local s = r:size()
			-- after scaling up, the dimensions should be doubled
			assert(s.x > 0, "scaleUp should produce a valid volume")
		end
	)";
	scenegraph::SceneGraph sceneGraph;
	run(sceneGraph, script);
}

TEST_F(LUAApiTest, testVolumeScaleDown) {
	const core::String script = R"(
		function main(node, region, color)
			local volume = node:volume()
			-- fill a 2x2x2 area
			local mins = region:mins()
			volume:setVoxel(mins.x, mins.y, mins.z, 1)
			volume:setVoxel(mins.x + 1, mins.y, mins.z, 1)
			volume:setVoxel(mins.x, mins.y + 1, mins.z, 1)
			volume:setVoxel(mins.x + 1, mins.y + 1, mins.z, 1)
			volume:setVoxel(mins.x, mins.y, mins.z + 1, 1)
			volume:setVoxel(mins.x + 1, mins.y, mins.z + 1, 1)
			volume:setVoxel(mins.x, mins.y + 1, mins.z + 1, 1)
			volume:setVoxel(mins.x + 1, mins.y + 1, mins.z + 1, 1)
			volume:scaleDown()
		end
	)";
	scenegraph::SceneGraph sceneGraph;
	run(sceneGraph, script);
}

TEST_F(LUAApiTest, testVolumeScale) {
	const core::String script = R"(
		function main(node, region, color)
			local volume = node:volume()
			local mins = region:mins()
			volume:setVoxel(mins.x, mins.y, mins.z, 1)
			volume:scale(2.0)
			local r = volume:region()
			local s = r:size()
			assert(s.x > 0, "scale should produce a valid volume")
		end
	)";
	scenegraph::SceneGraph sceneGraph;
	run(sceneGraph, script);
}

TEST_F(LUAApiTest, testVolumeRemapToPalette) {
	const core::String script = R"(
		function main(node, region, color)
			local volume = node:volume()
			local mins = region:mins()
			volume:setVoxel(mins.x, mins.y, mins.z, 1)
			local oldPal = node:palette()
			local newPal = g_palette.new()
			newPal:load("built-in:nippon")
			volume:remapToPalette(oldPal, newPal)
		end
	)";
	scenegraph::SceneGraph sceneGraph;
	run(sceneGraph, script);
}

TEST_F(LUAApiTest, testVolumeRenderToImage) {
	const core::String script = R"(
		function main(node, region, color)
			local volume = node:volume()
			local mins = region:mins()
			volume:setVoxel(mins.x, mins.y, mins.z, 1)
			local img = volume:renderToImage("front")
			assert(img ~= nil, "renderToImage should return an image")
			local w = img:width()
			local h = img:height()
			assert(w > 0, "image width should be > 0, got " .. tostring(w))
			assert(h > 0, "image height should be > 0, got " .. tostring(h))
		end
	)";
	scenegraph::SceneGraph sceneGraph;
	run(sceneGraph, script);
}

TEST_F(LUAApiTest, testVolumeRenderIsometricImage) {
	const core::String script = R"(
		function main(node, region, color)
			local volume = node:volume()
			local mins = region:mins()
			volume:setVoxel(mins.x, mins.y, mins.z, 1)
			local img = volume:renderIsometricImage("front")
			assert(img ~= nil, "renderIsometricImage should return an image")
			local w = img:width()
			local h = img:height()
			assert(w > 0, "image width should be > 0, got " .. tostring(w))
			assert(h > 0, "image height should be > 0, got " .. tostring(h))
		end
	)";
	scenegraph::SceneGraph sceneGraph;
	run(sceneGraph, script);
}

} // namespace voxelgenerator
