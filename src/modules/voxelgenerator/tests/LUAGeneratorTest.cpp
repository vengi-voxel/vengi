/**
 * @file
 */

#include "core/collection/DynamicArray.h"
#include "app/tests/AbstractTest.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Voxel.h"
#include "voxelgenerator/LUAGenerator.h"

namespace voxelgenerator {

class LUAGeneratorTest: public app::AbstractTest {
};

TEST_F(LUAGeneratorTest, testInit) {
	LUAGenerator g;
	ASSERT_TRUE(g.init());
	g.shutdown();
}

TEST_F(LUAGeneratorTest, testExecute) {
	const core::String script = R"(
		--[[
		@tparam volume The volume to operate on
		@tparam region The region that is activate. E.g. selected. Setting voxels outside this region is not supported.
		@tparam color The activate palette index of the color to use
		--]]
		function main(volume, region, color)
			local w = region:width()
			local h = region:height()
			local d = region:depth()
			local x = region:x()
			local y = region:y()
			local z = region:z()
			local mins = region:mins()
			local maxs = region:maxs()
			local dim = maxs - mins
			volume:setVoxel(0, 0, 0, color)
			local match = palette.match(255, 0, 0)
			-- red matches palette index 37
			if match == 37 then
				volume:setVoxel(1, 0, 0, match)
			end
			local colors = palette.colors()
		end
	)";

	ASSERT_TRUE(voxel::initDefaultMaterialColors());

	voxel::Region region(0, 0, 0, 7, 7, 7);
	voxel::RawVolume volume(region);
	voxel::RawVolumeWrapper wrapper(&volume);

	LUAGenerator g;
	ASSERT_TRUE(g.init());
	EXPECT_TRUE(g.exec(script, &wrapper, wrapper.region(), voxel::createVoxel(voxel::VoxelType::Generic, 42)));
	EXPECT_EQ(42, volume.voxel(0, 0, 0).getColor());
	EXPECT_NE(0, volume.voxel(1, 0, 0).getColor());
	EXPECT_TRUE(wrapper.dirtyRegion().isValid());
	g.shutdown();
}

TEST_F(LUAGeneratorTest, testArguments) {
	const core::String script = R"(
		--[[
		@return A parameter description
		--]]
		function arguments()
			return {
					{ name = 'name', desc = 'desc', type = 'int' },
					{ name = 'name2', desc = 'desc2', type = 'float' }
				}
		end

		function main(volume, region, color, name, name2)
			if (name == 'param1') then
				error('Expected to get the value param1')
			end
			if (name2 == 'param2') then
				error('Expected to get the value param2')
			end
		end
	)";

	ASSERT_TRUE(voxel::initDefaultMaterialColors());

	voxel::Region region(0, 0, 0, 7, 7, 7);
	voxel::RawVolume volume(region);
	voxel::RawVolumeWrapper wrapper(&volume);

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
	core::DynamicArray<core::String> args;
	args.push_back("param1");
	args.push_back("param2");
	EXPECT_TRUE(g.exec(script, &wrapper, region, voxel::createVoxel(voxel::VoxelType::Generic, 42), args));
	g.shutdown();
}

}
