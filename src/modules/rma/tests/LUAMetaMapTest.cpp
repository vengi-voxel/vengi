/**
 * @file
 */
#include "rma/MetaMap.h"
#include "app/tests/AbstractTest.h"
#include "gtest/gtest.h"

namespace rma {

class LUAMetaMapTest : public app::AbstractTest {};

TEST_F(LUAMetaMapTest, testLoadMaps) {
	MetaMap metaMap("test");
	ASSERT_TRUE(metaMap.load(R"(
local tiles_city = {
	['house1'] = {
		'0',  '0', '0',
		'0', '+a', '0',
		'0',  'h', '0'
	},
	['street_h_1'] = {
		'0',  '0', '0',
		'h', '+h', 'h',
		'0',  '0', '0'
	},
	['street_v_1'] = {
		'0', 'vx', '0',
		'0', '+v', '0',
		'0', 'vx', '0'
	},
	['street_cross_1'] = {
		 '0',  'v', '0',
		 'h', '+x',  'h',
		 '0',  'v',  '0'
	}
}

function init(map)
	map:setTiles(tiles_city)
	map:setSize(6, 6)
	map:addTileConfig('street_h_1', 10)
	map:addTileConfig('street_cross', 1)
	map:addFixedTile('street_h_1', 1, 4)
	--map:addFixedTile('street_h_1', -10, -10)
	map:setDescription('Small test map to implement the game. Not much that can be seen here yet - but more will follow')
	map:setTitle('Small test map')
	map:setImage('city')
end
	)"));
	EXPECT_STREQ("test", metaMap.name().c_str());
	EXPECT_EQ(4, (int)metaMap.tiles.size());
	EXPECT_EQ(2, (int)metaMap.tileConfigs.size());

	Tile street_h_1;
	EXPECT_TRUE(metaMap.tiles.get("street_h_1", street_h_1));

	TileConfig street_h_1_cfg;
	EXPECT_TRUE(metaMap.tileConfigs.get("street_h_1", street_h_1_cfg));
	EXPECT_EQ(10, street_h_1_cfg.maximum);

	ASSERT_EQ(1, (int)metaMap.fixedTiles.size());
	EXPECT_STREQ("street_h_1", metaMap.fixedTiles[0].tileName.c_str());
	EXPECT_EQ(1, metaMap.fixedTiles[0].x);
	EXPECT_EQ(4, metaMap.fixedTiles[0].z);
}

} // namespace towerdefense
