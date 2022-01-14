
/*
 * Copyright (c) 2017 Juliette Foucaut & Doug Binks
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgement in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include "enkimi.h"

#include <math.h>
#include <assert.h>

// Add allocation pAllocation_ to stream, which will be freed using enkiNBTFreeAllocations
void enkiNBTAddAllocation( enkiNBTDataStream* pStream_, void* pAllocation_ );

static const uint32_t SECTOR_SIZE = 4096;

static const char* tagIdString[] =
{
	"TAG_End",
	"TAG_Byte",
	"TAG_Short",
	"TAG_Int",
	"TAG_Long",
	"TAG_Float",
	"TAG_Double",
	"TAG_Byte_Array",
	"TAG_String",
	"TAG_List",
	"TAG_Compound",
	"TAG_Int_Array",
	"TAG_Long_Array",
};

static uint32_t minecraftPalette[] = 
{ 
	0xff000000, 0xff7d7d7d, 0xff4cb376, 0xff436086, 0xff7a7a7a, 0xff4e7f9c, 0xff256647, 0xff535353, 0xffdcaf70, 0xffdcaf70, 
	0xff135bcf, 0xff125ad4, 0xffa0d3db, 0xff7a7c7e, 0xff7c8b8f, 0xff7e8287, 0xff737373, 0xff315166, 0xff31b245, 0xff54c3c2, 
	0xfff4f0da, 0xff867066, 0xff894326, 0xff838383, 0xff9fd3dc, 0xff324364, 0xff3634b4, 0xff23c7f6, 0xff7c7c7c, 0xff77bf8e, 
	0xffdcdcdc, 0xff296595, 0xff194f7b, 0xff538ba5, 0xff5e96bd, 0xffdddddd, 0xffe5e5e5, 0xff00ffff, 0xff0d00da, 0xff415778, 
	0xff0d0fe1, 0xff4eecf9, 0xffdbdbdb, 0xffa1a1a1, 0xffa6a6a6, 0xff0630bc, 0xff0026af, 0xff39586b, 0xff658765, 0xff1d1214, 
	0xff00ffff, 0xff005fde, 0xff31271a, 0xff4e87a6, 0xff2a74a4, 0xff0000ff, 0xff8f8c81, 0xffd5db61, 0xff2e5088, 0xff17593c, 
	0xff335682, 0xff676767, 0xff00b9ff, 0xff5b9ab8, 0xff387394, 0xff345f79, 0xff5190b6, 0xff6a6a6a, 0xff5b9ab8, 0xff40596a, 
	0xff7a7a7a, 0xffc2c2c2, 0xff65a0c9, 0xff6b6b84, 0xff2d2ddd, 0xff000066, 0xff0061ff, 0xff848484, 0xfff1f1df, 0xffffad7d, 
	0xfffbfbef, 0xff1d830f, 0xffb0a49e, 0xff65c094, 0xff3b5985, 0xff42748d, 0xff1b8ce3, 0xff34366f, 0xff334054, 0xff45768f, 
	0xffbf0a57, 0xff2198f1, 0xffffffec, 0xffb2b2b2, 0xffb2b2b2, 0xffffffff, 0xff2d5d7e, 0xff7c7c7c, 0xff7a7a7a, 0xff7cafcf, 
	0xff78aaca, 0xff6a6c6d, 0xfff4efd3, 0xff28bdc4, 0xff69dd92, 0xff53ae73, 0xff0c5120, 0xff5287a5, 0xff2a4094, 0xff7a7a7a, 
	0xff75718a, 0xff767676, 0xff1a162c, 0xff1a162c, 0xff1a162c, 0xff2d28a6, 0xffb1c454, 0xff51677c, 0xff494949, 0xff343434, 
	0xffd18934, 0xffa5dfdd, 0xff0f090c, 0xff316397, 0xff42a0e3, 0xff4d84a1, 0xff49859e, 0xff1f71dd, 0xffa8e2e7, 0xff74806d, 
	0xff3c3a2a, 0xff7c7c7c, 0xff5a5a5a, 0xff75d951, 0xff345e81, 0xff84c0ce, 0xff455f88, 0xff868b8e, 0xffd7dd74, 0xff595959, 
	0xff334176, 0xff008c0a, 0xff17a404, 0xff5992b3, 0xffb0b0b0, 0xff434347, 0xff1d6b9e, 0xff70fdfe, 0xffe5e5e5, 0xff4c4a4b, 
	0xffbdc6bf, 0xffddedfb, 0xff091bab, 0xff4f547d, 0xff717171, 0xffdfe6ea, 0xffe3e8eb, 0xff41819b, 0xff747474, 0xffa1b2d1, 
	0xfff6f6f6, 0xff878787, 0xff395ab0, 0xff325cac, 0xff152c47, 0xff65c878, 0xff3534df, 0xffc7c7c7, 0xffa5af72, 0xffbec7ac, 
	0xff9fd3dc, 0xffcacaca, 0xff425c96, 0xff121212, 0xfff4bfa2, 0xff1474cf, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xff1d56ac, 
	0xff1d57ae, 0xff1d57ae, 0xff1d57ae, 0xff243c50, 0xff8dcddd, 0xff4d7aaf, 0xff0e2034, 0xff366bcf, 0xff355d7e, 0xff7bb8c7, 
	0xff5f86bb, 0xff1e2e3f, 0xff3a6bc5, 0xff30536e, 0xffe0f3f7, 0xff5077a9, 0xff2955aa, 0xff21374e, 0xffcdc5dc, 0xff603b60, 
	0xff856785, 0xffa679a6, 0xffaa7eaa, 0xffa879a8, 0xffa879a8, 0xffa879a8, 0xffaae6e1, 0xffaae6e1, 0xff457d98, 0xfff0f0f0, 
	0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 
	0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 
	0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 
	0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 
	0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xfff0f0f0, 0xff242132 };

// BlockIds from: https://minecraft.fandom.com/wiki/Java_Edition_data_values/Pre-flattening/Block_IDs
// Then get dataValue from each material's page: https://minecraft.fandom.com/wiki/Stone#Data_values
static enkiMINamespaceAndBlockID defaultNamespaceAndBlockIDs[] = 
{
	{ "minecraft:air",                                      0,   0   }, 
	{ "minecraft:cave_air",                                 0,   1   }, // Added after flattening, data value not correct
	{ "minecraft:void_air",                                 0,   2   }, // Added after flattening, data value not correct
	{ "minecraft:stone",                                    1,   0   }, 
	{ "minecraft:granite",                                  1,   1   }, 
	{ "minecraft:polished_granite",                         1,   2   },
	{ "minecraft:diorite",                                  1,   3   }, 
	{ "minecraft:polished_diorite",                         1,   4   },
	{ "minecraft:andesite",                                 1,   5   }, 
	{ "minecraft:polished_andesite",                        1,   6   },
	{ "minecraft:smooth_stone",                             1,   7   }, // Added, data value not correct
	{ "minecraft:grass_block",                              2,   0   }, // https://minecraft.fandom.com/wiki/Grass_Block
	{ "minecraft:grass_block",                              2,   1   }, // snowy
	{ "minecraft:dirt",                                     3,   0   }, 
	{ "minecraft:coarse_dirt",                              3,   1   }, 
	{ "minecraft:cobblestone",                              4,   0   },
	{ "minecraft:oak_planks",                               5,   0   },
	{ "minecraft:spruce_planks",                            5,   1   },
	{ "minecraft:birch_planks",                             5,   2   },
	{ "minecraft:jungle_planks",                            5,   3   },
	{ "minecraft:acacia_planks",                            5,   4   },
	{ "minecraft:dark_oak_planks",                          5,   5   },
	{ "minecraft:crimson_planks",                           5,   6   },
	{ "minecraft:warped_planks",                            5,   7   },
	{ "minecraft:oak_sapling",                              6,   0   },
	{ "minecraft:spruce_sapling",                           6,   1   },
	{ "minecraft:birch_sapling",                            6,   2   },
	{ "minecraft:jungle_sapling",                           6,   3   },
	{ "minecraft:acacia_sapling",                           6,   4   },
	{ "minecraft:dark_oak_sapling",                         6,   5   },
	{ "minecraft:bedrock",                                  7,   0   },
	{ "minecraft:flowing_water",                            8,   0   },
	{ "minecraft:water",                                    9,   0   },
	{ "minecraft:flowing_lava",                             10,  0   },
	{ "minecraft:lava",                                     11,  0   },
	{ "minecraft:sand",                                     12,  0   },
	{ "minecraft:red_sand",                                 12,  1   },
	{ "minecraft:gravel",                                   13,  0   },
	{ "minecraft:gold_ore",                                 14,  0   },
	{ "minecraft:deepslate_gold_ore",                       14,  1   },
	{ "minecraft:iron_ore",                                 15,  0   },
	{ "minecraft:deepslate_iron_ore",                       15,  1   },
	{ "minecraft:coal_ore",                                 16,  0   },
	{ "minecraft:deepslate_coal_ore",                       16,  1   },
	{ "minecraft:oak_log",                                  17,  0   }, // up-down
    { "minecraft:spruce_log",                               17,  1   }, // up-down
    { "minecraft:birch_log",                                17,  2   }, // up-down
	{ "minecraft:jungle_log",                               17,  3   }, // up-down
    { "minecraft:oak_log",                                  17,  4   }, // east-west
    { "minecraft:spruce_log",                               17,  5   }, // east-west
	{ "minecraft:birch_log",                                17,  6   }, // east-west
	{ "minecraft:jungle_log",                               17,  7   }, // east-west
	{ "minecraft:oak_log",                                  17,  8   }, // north-south
	{ "minecraft:spruce_log",                               17,  9   }, // north-south
    { "minecraft:birch_log",                                17,  10  }, // north-south
	{ "minecraft:jungle_log",                               17,  11  }, // north-south
	{ "minecraft:oak_wood",                                 17,  12  }, // up-down
    { "minecraft:spruce_wood",                              17,  13  }, // up-down
    { "minecraft:birch_wood",                               17,  14  }, // up-down
	{ "minecraft:jungle_wood",                              17,  15  }, // up-down
	{ "minecraft:oak_leaves",                               18,  0   }, 
	{ "minecraft:spruce_leaves",                            18,  1   }, 
	{ "minecraft:birch_leaves",                             18,  2   }, 
	{ "minecraft:jungle_leaves",                            18,  3   }, 
	{ "minecraft:oak_leaves",                               18,  4   }, // check for decay (if no log within 4 blocks decay)
	{ "minecraft:spruce_leaves",                            18,  5   }, // check for decay (if no log within 4 blocks decay)
	{ "minecraft:birch_leaves",                             18,  6   }, // check for decay (if no log within 4 blocks decay)
	{ "minecraft:jungle_leaves",                            18,  7   }, // check for decay (if no log within 4 blocks decay)
	{ "minecraft:oak_leaves",                               18,  8   }, // persistent
	{ "minecraft:spruce_leaves",                            18,  9   }, // persistent
	{ "minecraft:birch_leaves",                             18,  10  }, // persistent
	{ "minecraft:jungle_leaves",                            18,  11  }, // persistent
	{ "minecraft:oak_leaves",                               18,  12  }, // persistent (12-15 same as 8-11)
	{ "minecraft:spruce_leaves",                            18,  13  }, // persistent (12-15 same as 8-11)
	{ "minecraft:birch_leaves",                             18,  14  }, // persistent (12-15 same as 8-11)
	{ "minecraft:jungle_leaves",                            18,  15  }, // persistent (12-15 same as 8-11)
	{ "minecraft:sponge",                                   19,  0   }, 
	{ "minecraft:wet_sponge",                               19,  1   }, 
	{ "minecraft:glass",                                    20,  0   }, 
	{ "minecraft:lapis_ore",                                21,  0   }, 
	{ "minecraft:deepslate_lapis_ore",                      21,  1   },  // Added after flattening, data value not correct
	{ "minecraft:lapis_block",                              22,  0   }, 
	{ "minecraft:dispenser",                                23,  0   },
	{ "minecraft:sandstone",                                24,  0   },
	{ "minecraft:cut_sandstone",                            24,  1   },
	{ "minecraft:chiseled_sandstone",                       24,  2   },
	{ "minecraft:smooth_sandstone",                         24,  3   },
	{ "minecraft:note_block",                               25,  0   }, 
	{ "minecraft:white_bed",                                26,  0   }, 
	{ "minecraft:orange_bed",                               26,  1   }, 
	{ "minecraft:magenta_bed",                              26,  2   }, 
	{ "minecraft:light_blue_bed",                           26,  3   }, 
	{ "minecraft:yellow_bed",                               26,  4   }, 
	{ "minecraft:lime_bed",                                 26,  5   }, 
	{ "minecraft:pink_bed",                                 26,  6   }, 
	{ "minecraft:gray_bed",                                 26,  7   }, 
	{ "minecraft:light_gray_bed",                           26,  8   }, 
	{ "minecraft:cyan_bed",                                 26,  9   }, 
	{ "minecraft:purple_bed",                               26,  10  }, 
	{ "minecraft:blue_bed",                                 26,  11  }, 
	{ "minecraft:brown_bed",                                26,  12  }, 
	{ "minecraft:green_bed",                                26,  13  }, 
	{ "minecraft:red_bed",                                  26,  14  }, 
	{ "minecraft:black_bed",                                26,  15  }, 
	{ "minecraft:powered_rail",                             27,  0   }, 
	{ "minecraft:detector_rail",                            28,  0   },
	{ "minecraft:sticky_piston",                            29,  0   }, // facing down
	{ "minecraft:sticky_piston",                            29,  1   }, // facing east
	{ "minecraft:sticky_piston",                            29,  2   }, // facing north
	{ "minecraft:sticky_piston",                            29,  3   }, // facing south
	{ "minecraft:sticky_piston",                            29,  4   }, // facing up
	{ "minecraft:sticky_piston",                            29,  5   }, // facing west
	{ "minecraft:cobweb",                                   30,  0   },
	{ "minecraft:grass",                                    31,  0   }, // short plants, https://minecraft.fandom.com/wiki/Grass
	{ "minecraft:fern",                                     31,  1   }, // short plants, 
	{ "minecraft:seagrass",                                 31,  2   }, // short plants, - Added after flattening, data value not correct
	{ "minecraft:dead_bush",                                32,  0   }, 
	{ "minecraft:piston",                                   33,  0   }, // facing down
	{ "minecraft:piston",                                   33,  1   }, // facing east
	{ "minecraft:piston",                                   33,  2   }, // facing north
	{ "minecraft:piston",                                   33,  3   }, // facing south
	{ "minecraft:piston",                                   33,  4   }, // facing up
	{ "minecraft:piston",                                   33,  5   }, // facing west
	{ "minecraft:piston_head",                              34,  0   }, // facing down
	{ "minecraft:piston_head",                              34,  1   }, // facing east
	{ "minecraft:piston_head",                              34,  2   }, // facing north
	{ "minecraft:piston_head",                              34,  3   }, // facing south
	{ "minecraft:piston_head",                              34,  4   }, // facing up
	{ "minecraft:piston_head",                              34,  5   }, // facing west
	{ "minecraft:white_wool",                               35,  0   }, 
	{ "minecraft:orange_wool",                              35,  1   }, 
	{ "minecraft:magenta_wool",                             35,  2   }, 
	{ "minecraft:light_blue_wool",                          35,  3   }, 
	{ "minecraft:yellow_wool",                              35,  4   }, 
	{ "minecraft:lime_wool",                                35,  5   }, 
	{ "minecraft:pink_wool",                                35,  6   }, 
	{ "minecraft:gray_wool",                                35,  7   }, 
	{ "minecraft:light_gray_wool",                          35,  8   }, 
	{ "minecraft:cyan_wool",                                35,  9   }, 
	{ "minecraft:purple_wool",                              35,  10  }, 
	{ "minecraft:blue_wool",                                35,  11  }, 
	{ "minecraft:brown_wool",                               35,  12  }, 
	{ "minecraft:green_wool",                               35,  13  }, 
	{ "minecraft:red_wool",                                 35,  14  },
	{ "minecraft:black_wool",                               35,  15  }, 
	{ "minecraft:moving_piston",                            36,  0   }, // facing down
	{ "minecraft:moving_piston",                            36,  1   }, // facing east
	{ "minecraft:moving_piston",                            36,  2   }, // facing north
	{ "minecraft:moving_piston",                            36,  3   }, // facing south
	{ "minecraft:moving_piston",                            36,  4   }, // facing up
	{ "minecraft:moving_piston",                            36,  5   }, // facing west
	{ "minecraft:dandelion",                                37,  0   }, 
	{ "minecraft:poppy",                                    38,  0   }, // small flowers
	{ "minecraft:blue_orchid",                              38,  1   }, // small flowers
	{ "minecraft:allium",                                   38,  2   }, // small flowers
	{ "minecraft:azure_bluet",                              38,  3   }, // small flowers
	{ "minecraft:red_tulip",                                38,  4   }, // small flowers
	{ "minecraft:orange_tulip",                             38,  5   }, // small flowers
	{ "minecraft:white_tulip",                              38,  6   }, // small flowers
	{ "minecraft:pink_tulip",                               38,  7   }, // small flowers
	{ "minecraft:oxeye_daisy",                              38,  8   }, // small flowers
	{ "minecraft:cornflower",                               38,  9   }, // small flowers
	{ "minecraft:lily_of_the_valley",                       38,  10  }, // small flowers
	{ "minecraft:wither_rose",                              38,  11  }, // small flowers
	{ "minecraft:brown_mushroom",                           39,  0   },
	{ "minecraft:red_mushroom",                             40,  0   },
	{ "minecraft:gold_block",                               41,  0   },
	{ "minecraft:iron_block",                               42,  0   },
    { "minecraft:double_stone_slab",                        43,  0   },
    { "minecraft:double_stone_slab",                        43,  1   }, // waterlogged
    { "minecraft:double_smooth_stone_slab",                 43,  2   },
    { "minecraft:double_smooth_stone_slab",                 43,  3   }, // waterlogged
    { "minecraft:double_granite_slab",                      43,  4   },
    { "minecraft:double_granite_slab",                      43,  5   }, // waterlogged
    { "minecraft:double_polished_granite_slab",             43,  6   },
    { "minecraft:double_polished_granite_slab",             43,  7   }, // waterlogged
    { "minecraft:double_diorite_slab",                      43,  8   },
    { "minecraft:double_diorite_slab",                      43,  9   }, // waterlogged
    { "minecraft:double_polished_diorite_slab",             43,  10  },
    { "minecraft:double_polished_diorite_slab",             43,  11  }, // waterlogged
    { "minecraft:double_andesite_slab",                     43,  12  },
    { "minecraft:double_andesite_slab",                     43,  13  }, // waterlogged
    { "minecraft:double_polished_andesite_slab",            43,  14  },
    { "minecraft:double_polished_andesite_slab",            43,  15  }, // waterlogged
    { "minecraft:double_cobblestone_slab",                  43,  16  },
    { "minecraft:double_cobblestone_slab",                  43,  17  }, // waterlogged
    { "minecraft:double_mossy_cobblestone_slab",            43,  18  },
    { "minecraft:double_mossy_cobblestone_slab",            43,  19  }, // waterlogged
    { "minecraft:double_stone_brick_slab",                  43,  20  },
    { "minecraft:double_stone_brick_slab",                  43,  21  }, // waterlogged
    { "minecraft:double_mossy_stone_brick_slab",            43,  22  },
    { "minecraft:double_mossy_stone_brick_slab",            43,  23  }, // waterlogged
    { "minecraft:double_brick_slab",                        43,  24  },
    { "minecraft:double_brick_slab",                        43,  25  }, // waterlogged
    { "minecraft:double_end_stone_brick_slab",              43,  26  },
    { "minecraft:double_end_stone_brick_slab",              43,  27  }, // waterlogged
    { "minecraft:double_nether_brick_slab",                 43,  28  },
    { "minecraft:double_nether_brick_slab",                 43,  29  }, // waterlogged
    { "minecraft:double_red_nether_brick_slab",             43,  30  },
    { "minecraft:double_red_nether_brick_slab",             43,  31  }, // waterlogged
    { "minecraft:double_sandstone_slab",                    43,  32  },
    { "minecraft:double_sandstone_slab",                    43,  33  }, // waterlogged
    { "minecraft:double_cut_sandstone_slab",                43,  34  },
    { "minecraft:double_cut_sandstone_slab",                43,  35  }, // waterlogged
    { "minecraft:double_smooth_sandstone_slab",             43,  36  },
    { "minecraft:double_smooth_sandstone_slab",             43,  37  }, // waterlogged
    { "minecraft:double_quartz_slab",                       43,  38  },
    { "minecraft:double_quartz_slab",                       43,  39  }, // waterlogged
    { "minecraft:double_smooth_quartz_slab",                43,  40  },
    { "minecraft:double_smooth_quartz_slab",                43,  41  }, // waterlogged
    { "minecraft:double_prismarine_slab",                   43,  42  },
    { "minecraft:double_prismarine_slab",                   43,  43  }, // waterlogged
    { "minecraft:double_prismarine_brick_slab",             43,  44  },
    { "minecraft:double_prismarine_brick_slab",             43,  45  }, // waterlogged
    { "minecraft:double_dark_prismarine_slab",              43,  46  },
    { "minecraft:double_dark_prismarine_slab",              43,  47  }, // waterlogged
    { "minecraft:double_petrified_oak_slab",                43,  48  },
    { "minecraft:double_petrified_oak_slab",                43,  49  }, // waterlogged
    { "minecraft:double_blackstone_slab",                   43,  50  },
    { "minecraft:double_blackstone_slab",                   43,  51  }, // waterlogged
    { "minecraft:double_polished_blackstone_slab",          43,  52  },
    { "minecraft:double_polished_blackstone_slab",          43,  53  }, // waterlogged
    { "minecraft:double_polished_blackstone_brick_slab",    43,  54  },
    { "minecraft:double_polished_blackstone_brick_slab",    43,  55  }, // waterlogged
    { "minecraft:double_cut_copper_slab",                   43,  56  },
    { "minecraft:double_cut_copper_slab",                   43,  57  }, // waterlogged
    { "minecraft:double_exposed_cut_copper_slab",           43,  58  },
    { "minecraft:double_exposed_cut_copper_slab",           43,  59  }, // waterlogged
    { "minecraft:double_weathered_cut_copper_slab",         43,  60  },
    { "minecraft:double_weathered_cut_copper_slab",         43,  61  }, // waterlogged
    { "minecraft:double_oxidized_cut_copper_slab",          43,  62  },
    { "minecraft:double_oxidized_cut_copper_slab",          43,  63  }, // waterlogged
    { "minecraft:double_waxed_cut_copper_slab",             43,  64  },
    { "minecraft:double_waxed_cut_copper_slab",             43,  65  }, // waterlogged
    { "minecraft:double_waxed_exposed_cut_copper_slab",     43,  66  },
    { "minecraft:double_waxed_exposed_cut_copper_slab",     43,  67  }, // waterlogged
    { "minecraft:double_waxed_weathered_cut_copper_slab",   43,  68  },
    { "minecraft:double_waxed_weathered_cut_copper_slab",   43,  69  }, // waterlogged
    { "minecraft:double_waxed_oxidized_cut_copper_slab",    43,  70  },
    { "minecraft:double_waxed_oxidized_cut_copper_slab",    43,  71  }, // waterlogged
    { "minecraft:double_cobbled_deepslate_slab",            43,  72  },
    { "minecraft:double_cobbled_deepslate_slab",            43,  73  }, // waterlogged
    { "minecraft:double_polished_deepslate_slab",           43,  74  },
    { "minecraft:double_polished_deepslate_slab",           43,  75  }, // waterlogged
    { "minecraft:double_deepslate_brick_slab",              43,  76  },
    { "minecraft:double_deepslate_brick_slab",              43,  77  }, // waterlogged
    { "minecraft:double_deepslate_tile_slab",               43,  78  },
    { "minecraft:double_deepslate_tile_slab",               43,  79  }, // waterlogged
	{ "minecraft:stone_slab",                               44,  0   }, // bottom
	{ "minecraft:stone_slab",                               44,  1   }, // top
	{ "minecraft:stone_slab",                               44,  2   }, // bottom waterlogged
	{ "minecraft:stone_slab",                               44,  3   }, // top waterlogged
	{ "minecraft:smooth_stone_slab",                        44,  4   }, // bottom
	{ "minecraft:smooth_stone_slab",                        44,  5   }, // top
	{ "minecraft:smooth_stone_slab",                        44,  6   }, // bottom waterlogged
	{ "minecraft:smooth_stone_slab",                        44,  7   }, // top waterlogged
	{ "minecraft:granite_slab",                             44,  8   }, // bottom
	{ "minecraft:granite_slab",                             44,  9   }, // top
	{ "minecraft:granite_slab",                             44,  10  }, // bottom waterlogged
	{ "minecraft:granite_slab",                             44,  11  }, // top waterlogged
	{ "minecraft:polished_granite_slab",                    44,  12  }, // bottom
	{ "minecraft:polished_granite_slab",                    44,  13  }, // top
	{ "minecraft:polished_granite_slab",                    44,  14  }, // bottom waterlogged
	{ "minecraft:polished_granite_slab",                    44,  15  }, // top waterlogged
	{ "minecraft:diorite_slab",                             44,  16  }, // bottom
	{ "minecraft:diorite_slab",                             44,  17  }, // top
	{ "minecraft:diorite_slab",                             44,  18  }, // bottom waterlogged
	{ "minecraft:diorite_slab",                             44,  19  }, // top waterlogged
	{ "minecraft:polished_diorite_slab",                    44,  20  }, // bottom
	{ "minecraft:polished_diorite_slab",                    44,  21  }, // top
	{ "minecraft:polished_diorite_slab",                    44,  22  }, // bottom waterlogged
	{ "minecraft:polished_diorite_slab",                    44,  23  }, // top waterlogged
	{ "minecraft:andesite_slab",                            44,  24  }, // bottom
	{ "minecraft:andesite_slab",                            44,  25  }, // top
	{ "minecraft:andesite_slab",                            44,  26  }, // bottom waterlogged
	{ "minecraft:andesite_slab",                            44,  27  }, // top waterlogged
	{ "minecraft:polished_andesite_slab",                   44,  28  }, // bottom
	{ "minecraft:polished_andesite_slab",                   44,  29  }, // top
	{ "minecraft:polished_andesite_slab",                   44,  30  }, // bottom waterlogged
	{ "minecraft:polished_andesite_slab",                   44,  31  }, // top waterlogged
	{ "minecraft:cobblestone_slab",                         44,  32  }, // bottom
	{ "minecraft:cobblestone_slab",                         44,  33  }, // top
	{ "minecraft:cobblestone_slab",                         44,  34  }, // bottom waterlogged
	{ "minecraft:cobblestone_slab",                         44,  35  }, // top waterlogged
	{ "minecraft:mossy_cobblestone_slab",                   44,  36  }, // bottom
	{ "minecraft:mossy_cobblestone_slab",                   44,  37  }, // top
	{ "minecraft:mossy_cobblestone_slab",                   44,  38  }, // bottom waterlogged
	{ "minecraft:mossy_cobblestone_slab",                   44,  39  }, // top waterlogged
	{ "minecraft:stone_brick_slab",                         44,  40  }, // bottom
	{ "minecraft:stone_brick_slab",                         44,  41  }, // top
	{ "minecraft:stone_brick_slab",                         44,  42  }, // bottom waterlogged
	{ "minecraft:stone_brick_slab",                         44,  43  }, // top waterlogged
	{ "minecraft:mossy_stone_brick_slab",                   44,  44  }, // bottom
	{ "minecraft:mossy_stone_brick_slab",                   44,  45  }, // top
	{ "minecraft:mossy_stone_brick_slab",                   44,  46  }, // bottom waterlogged
	{ "minecraft:mossy_stone_brick_slab",                   44,  47  }, // top waterlogged
	{ "minecraft:brick_slab",                               44,  48  }, // bottom
	{ "minecraft:brick_slab",                               44,  49  }, // top
	{ "minecraft:brick_slab",                               44,  50  }, // bottom waterlogged
	{ "minecraft:brick_slab",                               44,  51  }, // top waterlogged
	{ "minecraft:end_stone_brick_slab",                     44,  52  }, // bottom
	{ "minecraft:end_stone_brick_slab",                     44,  53  }, // top
	{ "minecraft:end_stone_brick_slab",                     44,  54  }, // bottom waterlogged
	{ "minecraft:end_stone_brick_slab",                     44,  55  }, // top waterlogged
	{ "minecraft:nether_brick_slab",                        44,  56  }, // bottom
	{ "minecraft:nether_brick_slab",                        44,  57  }, // top
	{ "minecraft:nether_brick_slab",                        44,  58  }, // bottom waterlogged
	{ "minecraft:nether_brick_slab",                        44,  59  }, // top waterlogged
	{ "minecraft:red_nether_brick_slab",                    44,  60  }, // bottom
	{ "minecraft:red_nether_brick_slab",                    44,  61  }, // top
	{ "minecraft:red_nether_brick_slab",                    44,  62  }, // bottom waterlogged
	{ "minecraft:red_nether_brick_slab",                    44,  63  }, // top waterlogged
	{ "minecraft:sandstone_slab",                           44,  64  }, // bottom
	{ "minecraft:sandstone_slab",                           44,  65  }, // top
	{ "minecraft:sandstone_slab",                           44,  66  }, // bottom waterlogged
	{ "minecraft:sandstone_slab",                           44,  67  }, // top waterlogged
	{ "minecraft:cut_sandstone_slab",                       44,  68  }, // bottom
	{ "minecraft:cut_sandstone_slab",                       44,  69  }, // top
	{ "minecraft:cut_sandstone_slab",                       44,  70  }, // bottom waterlogged
	{ "minecraft:cut_sandstone_slab",                       44,  71  }, // top waterlogged
	{ "minecraft:smooth_sandstone_slab",                    44,  72  }, // bottom
	{ "minecraft:smooth_sandstone_slab",                    44,  73  }, // top
	{ "minecraft:smooth_sandstone_slab",                    44,  74  }, // bottom waterlogged
	{ "minecraft:smooth_sandstone_slab",                    44,  75  }, // top waterlogged
	{ "minecraft:quartz_slab",                              44,  76  }, // bottom
	{ "minecraft:quartz_slab",                              44,  77  }, // top
	{ "minecraft:quartz_slab",                              44,  78  }, // bottom waterlogged
	{ "minecraft:quartz_slab",                              44,  79  }, // top waterlogged
	{ "minecraft:smooth_quartz_slab",                       44,  80  }, // bottom
	{ "minecraft:smooth_quartz_slab",                       44,  81  }, // top
	{ "minecraft:smooth_quartz_slab",                       44,  82  }, // bottom waterlogged
	{ "minecraft:smooth_quartz_slab",                       44,  83  }, // top waterlogged
	{ "minecraft:prismarine_slab",                          44,  84  }, // bottom
	{ "minecraft:prismarine_slab",                          44,  85  }, // top
	{ "minecraft:prismarine_slab",                          44,  86  }, // bottom waterlogged
	{ "minecraft:prismarine_slab",                          44,  87  }, // top waterlogged
	{ "minecraft:prismarine_brick_slab",                    44,  88  }, // bottom
	{ "minecraft:prismarine_brick_slab",                    44,  89  }, // top
	{ "minecraft:prismarine_brick_slab",                    44,  90  }, // bottom waterlogged
	{ "minecraft:prismarine_brick_slab",                    44,  91  }, // top waterlogged
	{ "minecraft:dark_prismarine_slab",                     44,  92  }, // bottom
	{ "minecraft:dark_prismarine_slab",                     44,  93  }, // top
	{ "minecraft:dark_prismarine_slab",                     44,  94  }, // bottom waterlogged
	{ "minecraft:dark_prismarine_slab",                     44,  95  }, // top waterlogged
	{ "minecraft:petrified_oak_slab",                       44,  96  }, // bottom
	{ "minecraft:petrified_oak_slab",                       44,  97  }, // top
	{ "minecraft:petrified_oak_slab",                       44,  98  }, // bottom waterlogged
	{ "minecraft:petrified_oak_slab",                       44,  99  }, // top waterlogged
	{ "minecraft:blackstone_slab",                          44,  100 }, // bottom
	{ "minecraft:blackstone_slab",                          44,  101 }, // top
	{ "minecraft:blackstone_slab",                          44,  102 }, // bottom waterlogged
	{ "minecraft:blackstone_slab",                          44,  103 }, // top waterlogged
	{ "minecraft:polished_blackstone_slab",                 44,  104 }, // bottom
	{ "minecraft:polished_blackstone_slab",                 44,  105 }, // top
	{ "minecraft:polished_blackstone_slab",                 44,  106 }, // bottom waterlogged
	{ "minecraft:polished_blackstone_slab",                 44,  107 }, // top waterlogged
	{ "minecraft:polished_blackstone_brick_slab",           44,  108 }, // bottom
	{ "minecraft:polished_blackstone_brick_slab",           44,  109 }, // top
	{ "minecraft:polished_blackstone_brick_slab",           44,  110 }, // bottom waterlogged
	{ "minecraft:polished_blackstone_brick_slab",           44,  111 }, // top waterlogged
	{ "minecraft:cut_copper_slab",                          44,  112 }, // bottom
	{ "minecraft:cut_copper_slab",                          44,  113 }, // top
	{ "minecraft:cut_copper_slab",                          44,  114 }, // bottom waterlogged
	{ "minecraft:cut_copper_slab",                          44,  115 }, // top waterlogged
	{ "minecraft:exposed_cut_copper_slab",                  44,  116 }, // bottom
	{ "minecraft:exposed_cut_copper_slab",                  44,  117 }, // top
	{ "minecraft:exposed_cut_copper_slab",                  44,  118 }, // bottom waterlogged
	{ "minecraft:exposed_cut_copper_slab",                  44,  119 }, // top waterlogged
	{ "minecraft:weathered_cut_copper_slab",                44,  120 }, // bottom
	{ "minecraft:weathered_cut_copper_slab",                44,  121 }, // top
	{ "minecraft:weathered_cut_copper_slab",                44,  122 }, // bottom waterlogged
	{ "minecraft:weathered_cut_copper_slab",                44,  123 }, // top waterlogged
	{ "minecraft:oxidized_cut_copper_slab",                 44,  124 }, // bottom
	{ "minecraft:oxidized_cut_copper_slab",                 44,  125 }, // top
	{ "minecraft:oxidized_cut_copper_slab",                 44,  126 }, // bottom waterlogged
	{ "minecraft:oxidized_cut_copper_slab",                 44,  127 }, // top waterlogged
	{ "minecraft:waxed_cut_copper_slab",                    44,  128 }, // bottom
	{ "minecraft:waxed_cut_copper_slab",                    44,  129 }, // top
	{ "minecraft:waxed_cut_copper_slab",                    44,  130 }, // bottom waterlogged
	{ "minecraft:waxed_cut_copper_slab",                    44,  131 }, // top waterlogged
	{ "minecraft:waxed_exposed_cut_copper_slab",            44,  132 }, // bottom
	{ "minecraft:waxed_exposed_cut_copper_slab",            44,  133 }, // top
	{ "minecraft:waxed_exposed_cut_copper_slab",            44,  134 }, // bottom waterlogged
	{ "minecraft:waxed_exposed_cut_copper_slab",            44,  135 }, // top waterlogged
	{ "minecraft:waxed_weathered_cut_copper_slab",          44,  136 }, // bottom
	{ "minecraft:waxed_weathered_cut_copper_slab",          44,  137 }, // top
	{ "minecraft:waxed_weathered_cut_copper_slab",          44,  138 }, // bottom waterlogged
	{ "minecraft:waxed_weathered_cut_copper_slab",          44,  139 }, // top waterlogged
	{ "minecraft:waxed_oxidized_cut_copper_slab",           44,  140 }, // bottom
	{ "minecraft:waxed_oxidized_cut_copper_slab",           44,  141 }, // top
	{ "minecraft:waxed_oxidized_cut_copper_slab",           44,  142 }, // bottom waterlogged
	{ "minecraft:waxed_oxidized_cut_copper_slab",           44,  143 }, // top waterlogged
	{ "minecraft:cobbled_deepslate_slab",                   44,  144 }, // bottom
	{ "minecraft:cobbled_deepslate_slab",                   44,  145 }, // top
	{ "minecraft:cobbled_deepslate_slab",                   44,  146 }, // bottom waterlogged
	{ "minecraft:cobbled_deepslate_slab",                   44,  147 }, // top waterlogged
	{ "minecraft:polished_deepslate_slab",                  44,  148 }, // bottom
	{ "minecraft:polished_deepslate_slab",                  44,  149 }, // top
	{ "minecraft:polished_deepslate_slab",                  44,  150 }, // bottom waterlogged
	{ "minecraft:polished_deepslate_slab",                  44,  151 }, // top waterlogged
	{ "minecraft:deepslate_brick_slab",                     44,  152 }, // bottom
	{ "minecraft:deepslate_brick_slab",                     44,  153 }, // top
	{ "minecraft:deepslate_brick_slab",                     44,  154 }, // bottom waterlogged
	{ "minecraft:deepslate_brick_slab",                     44,  155 }, // top waterlogged
	{ "minecraft:deepslate_tile_slab",                      44,  156 }, // bottom
	{ "minecraft:deepslate_tile_slab",                      44,  157 }, // top
	{ "minecraft:deepslate_tile_slab",                      44,  158 }, // bottom waterlogged
	{ "minecraft:deepslate_tile_slab",                      44,  159 }, // top waterlogged
	{ "minecraft:bricks",                                   45,  0   },
	{ "minecraft:tnt",                                      46,  0   },
	{ "minecraft:bookshelf",                                47,  0   },
	{ "minecraft:mossy_cobblestone",                        48,  0   },
	{ "minecraft:obsidian",                                 49,  0   },
	{ "minecraft:torch",                                    50,  0   },
	{ "minecraft:wall_torch",                               50,  1   }, // facing east
	{ "minecraft:wall_torch",                               50,  2   }, // facing west
	{ "minecraft:wall_torch",                               50,  3   }, // facing south
	{ "minecraft:wall_torch",                               50,  4   }, // facing north
	{ "minecraft:wall_torch",                               50,  5   }, // facing up
	{ "minecraft:soul_torch",                               50,  6   },
	{ "minecraft:soul_wall_torch",                          50,  7   }, // facing east - Added after flattening, data value not correct
	{ "minecraft:soul_wall_torch",                          50,  8   }, // facing west - Added after flattening, data value not correct
	{ "minecraft:soul_wall_torch",                          50,  9   }, // facing south - Added after flattening, data value not correct
	{ "minecraft:soul_wall_torch",                          50,  10  }, // facing north - Added after flattening, data value not correct
	{ "minecraft:soul_wall_torch",                          50,  11  }, // facing up  - Added after flattening, data value not correct
	{ "minecraft:fire",                                     51,  0   },
	{ "minecraft:soul_fire",                                51,  1   }, // Added after flattening, data value not correct
	{ "minecraft:spawner",                                  52,  0   }, 
	{ "minecraft:mob_spawner",                              52,  1   },
	{ "minecraft:oak_stairs",                               53,  0   }, // facing east 
	{ "minecraft:oak_stairs",                               53,  1   }, // facing north 
	{ "minecraft:oak_stairs",                               53,  2   }, // facing south 
	{ "minecraft:oak_stairs",                               53,  3   }, // facing west 
	{ "minecraft:chest",                                    54,  0   }, // facing east
	{ "minecraft:chest",                                    54,  1   }, // facing north
	{ "minecraft:chest",                                    54,  2   }, // facing south
	{ "minecraft:chest",                                    54,  3   }, // facing west 
	{ "minecraft:redstone_wire",                            55,  0   }, 
	{ "minecraft:redstone",                                 55,  1   }, 
	{ "minecraft:diamond_ore",                              56,  0   }, 
	{ "minecraft:deepslate_diamond_ore",                    56,  1   }, 
	{ "minecraft:diamond_block",                            57,  0   }, 
	{ "minecraft:crafting_table",                           58,  0   },      
	{ "minecraft:wheat",                                    59,  0   }, // https://minecraft.fandom.com/wiki/Wheat_Seeds
	{ "minecraft:wheat",                                    59,  1   }, // growth
	{ "minecraft:wheat",                                    59,  2   }, // growth 
	{ "minecraft:wheat",                                    59,  3   }, // growth 
	{ "minecraft:wheat",                                    59,  4   }, // growth 
	{ "minecraft:wheat",                                    59,  5   }, // growth 
	{ "minecraft:wheat",                                    59,  6   }, // growth 
	{ "minecraft:wheat",                                    59,  7   }, // growth 
	{ "minecraft:farmland",                                 60,  0   }, 
	{ "minecraft:furnace",                                  61,  0   }, // facing east
	{ "minecraft:furnace",                                  61,  1   }, // facing north
	{ "minecraft:furnace",                                  61,  2   }, // facing south
	{ "minecraft:furnace",                                  61,  3   }, // facing west
    { "minecraft:lit_furnace",                              62,  0   }, // facing east
	{ "minecraft:lit_furnace",                              62,  1   }, // facing north
	{ "minecraft:lit_furnace",                              62,  2   }, // facing south
	{ "minecraft:lit_furnace",                              62,  3   }, // facing west
	{ "minecraft:oak_sign",                                 63,  0   },
	{ "minecraft:spruce_sign",                              63,  1   },
	{ "minecraft:birch_sign",                               63,  2   },
	{ "minecraft:jungle_sign",                              63,  3   },
	{ "minecraft:acacia_sign",                              63,  4   },
	{ "minecraft:dark_oak_sign",                            63,  5   },
	{ "minecraft:crimson_sign",                             63,  6   },
	{ "minecraft:warped_sign",                              63,  7   },
    { "minecraft:oak_door",                                 64,  0   }, // facing east
	{ "minecraft:oak_door",                                 64,  1   }, // facing north
	{ "minecraft:oak_door",                                 64,  2   }, // facing south
	{ "minecraft:oak_door",                                 64,  3   }, // facing west
    { "minecraft:ladder",                                   65,  0   }, // facing east
	{ "minecraft:ladder",                                   65,  1   }, // facing north
	{ "minecraft:ladder",                                   65,  2   }, // facing south
	{ "minecraft:ladder",                                   65,  3   }, // facing west
    { "minecraft:rail",                                     66,  0   }, // east-west
	{ "minecraft:rail",                                     66,  1   }, // north-south
    { "minecraft:stone_stairs",                             67,  0   }, // facing east	  - Added after flattening, data value not correct
	{ "minecraft:stone_stairs",                             67,  1   }, // facing north	  - Added after flattening, data value not correct
	{ "minecraft:stone_stairs",                             67,  2   }, // facing south	  - Added after flattening, data value not correct
	{ "minecraft:stone_stairs",                             67,  3   }, // facing west	  - Added after flattening, data value not correct
	{ "minecraft:granite_stairs",                           67,  4   }, // facing east  - added, data value not correct
	{ "minecraft:granite_stairs",                           67,  5   }, // facing north - added, data value not correct
	{ "minecraft:granite_stairs",                           67,  6   }, // facing south - added, data value not correct
	{ "minecraft:granite_stairs",                           67,  7   }, // facing west  - added, data value not correct
	{ "minecraft:polished_granite_stairs",                  67,  8   }, // facing east  - added, data value not correct
	{ "minecraft:polished_granite_stairs",                  67,  9   }, // facing north - added, data value not correct
	{ "minecraft:polished_granite_stairs",                  67,  10  }, // facing south - added, data value not correct
	{ "minecraft:polished_granite_stairs",                  67,  11  }, // facing west  - added, data value not correct
	{ "minecraft:diorite_stairs",                           67,  12  }, // facing east  - added, data value not correct
	{ "minecraft:diorite_stairs",                           67,  13  }, // facing north - added, data value not correct
	{ "minecraft:diorite_stairs",                           67,  14  }, // facing south - added, data value not correct
	{ "minecraft:diorite_stairs",                           67,  15  }, // facing west  - added, data value not correct
	{ "minecraft:polished_diorite_stairs",                  67,  16  }, // facing east  - added, data value not correct
	{ "minecraft:polished_diorite_stairs",                  67,  17  }, // facing north - added, data value not correct
	{ "minecraft:polished_diorite_stairs",                  67,  18  }, // facing south - added, data value not correct
	{ "minecraft:polished_diorite_stairs",                  67,  19  }, // facing west  - added, data value not correct
	{ "minecraft:andesite_stairs",                          67,  20  }, // facing east  - added, data value not correct
	{ "minecraft:andesite_stairs",                          67,  21  }, // facing north - added, data value not correct
	{ "minecraft:andesite_stairs",                          67,  22  }, // facing south - added, data value not correct
	{ "minecraft:andesite_stairs",                          67,  23  }, // facing west  - added, data value not correct
	{ "minecraft:polished_andesite_stairs",                 67,  24  }, // facing east  - added, data value not correct
	{ "minecraft:polished_andesite_stairs",                 67,  25  }, // facing north - added, data value not correct
	{ "minecraft:polished_andesite_stairs",                 67,  26  }, // facing south - added, data value not correct
	{ "minecraft:polished_andesite_stairs",                 67,  27  }, // facing west  - added, data value not correct
	{ "minecraft:cobblestone_stairs",                       67,  28  }, // facing east 
	{ "minecraft:cobblestone_stairs",                       67,  29  }, // facing north 
	{ "minecraft:cobblestone_stairs",                       67,  30  }, // facing south 
	{ "minecraft:cobblestone_stairs",                       67,  31  }, // facing west     
	{ "minecraft:mossy_cobblestone_stairs",                 67,  32  }, // facing east  - added, data value not correct
	{ "minecraft:mossy_cobblestone_stairs",                 67,  33  }, // facing north - added, data value not correct
	{ "minecraft:mossy_cobblestone_stairs",                 67,  34  }, // facing south - added, data value not correct
	{ "minecraft:mossy_cobblestone_stairs",                 67,  35  }, // facing west  - added, data value not correct
	{ "minecraft:prismarine_stairs",                        67,  36  }, // facing east  - added, data value not correct
	{ "minecraft:prismarine_stairs",                        67,  37  }, // facing north - added, data value not correct
	{ "minecraft:prismarine_stairs",                        67,  38  }, // facing south - added, data value not correct
	{ "minecraft:prismarine_stairs",                        67,  39  }, // facing west  - added, data value not correct
	{ "minecraft:prismarine_brick_stairs",                  67,  40  }, // facing east  - added, data value not correct
	{ "minecraft:prismarine_brick_stairs",                  67,  41  }, // facing north - added, data value not correct
	{ "minecraft:prismarine_brick_stairs",                  67,  42  }, // facing south - added, data value not correct
	{ "minecraft:prismarine_brick_stairs",                  67,  43  }, // facing west  - added, data value not correct
	{ "minecraft:dark_prismarine_stairs",                   67,  44  }, // facing east  - added, data value not correct
	{ "minecraft:dark_prismarine_stairs",                   67,  45  }, // facing north - added, data value not correct
	{ "minecraft:dark_prismarine_stairs",                   67,  46  }, // facing south - added, data value not correct
	{ "minecraft:dark_prismarine_stairs",                   67,  47  }, // facing west  - added, data value not correct
	{ "minecraft:blackstone_stairs",                        67,  48  }, // facing east  - added, data value not correct
	{ "minecraft:blackstone_stairs",                        67,  49  }, // facing north - added, data value not correct
	{ "minecraft:blackstone_stairs",                        67,  50  }, // facing south - added, data value not correct
	{ "minecraft:blackstone_stairs",                        67,  51  }, // facing west  - added, data value not correct
	{ "minecraft:polished_blackstone_stairs",               67,  52  }, // facing east  - added, data value not correct
	{ "minecraft:polished_blackstone_stairs",               67,  53  }, // facing north - added, data value not correct
	{ "minecraft:polished_blackstone_stairs",               67,  54  }, // facing south - added, data value not correct
	{ "minecraft:polished_blackstone_stairs",               67,  55  }, // facing west  - added, data value not correct
	{ "minecraft:polished_blackstone_brick_stairs",         67,  56  }, // facing east  - added, data value not correct
	{ "minecraft:polished_blackstone_brick_stairs",         67,  57  }, // facing north - added, data value not correct
	{ "minecraft:polished_blackstone_brick_stairs",         67,  58  }, // facing south - added, data value not correct
	{ "minecraft:polished_blackstone_brick_stairs",         67,  59  }, // facing west  - added, data value not correct
	{ "minecraft:cut_copper_stairs",                        67,  60  }, // facing east  - added, data value not correct
	{ "minecraft:cut_copper_stairs",                        67,  61  }, // facing north - added, data value not correct
	{ "minecraft:cut_copper_stairs",                        67,  62  }, // facing south - added, data value not correct
	{ "minecraft:cut_copper_stairs",                        67,  63  }, // facing west  - added, data value not correct
	{ "minecraft:exposed_cut_copper_stairs",                67,  64  }, // facing east  - added, data value not correct
	{ "minecraft:exposed_cut_copper_stairs",                67,  65  }, // facing north - added, data value not correct
	{ "minecraft:exposed_cut_copper_stairs",                67,  66  }, // facing south - added, data value not correct
	{ "minecraft:exposed_cut_copper_stairs",                67,  67  }, // facing west  - added, data value not correct
	{ "minecraft:weathered_cut_copper_stairs",              67,  68  }, // facing east  - added, data value not correct
	{ "minecraft:weathered_cut_copper_stairs",              67,  69  }, // facing north - added, data value not correct
	{ "minecraft:weathered_cut_copper_stairs",              67,  70  }, // facing south - added, data value not correct
	{ "minecraft:weathered_cut_copper_stairs",              67,  71  }, // facing west  - added, data value not correct
	{ "minecraft:oxidized_cut_copper_stairs",               67,  72  }, // facing east  - added, data value not correct
	{ "minecraft:oxidized_cut_copper_stairs",               67,  73  }, // facing north - added, data value not correct
	{ "minecraft:oxidized_cut_copper_stairs",               67,  74  }, // facing south - added, data value not correct
	{ "minecraft:oxidized_cut_copper_stairs",               67,  75  }, // facing west  - added, data value not correct
	{ "minecraft:waxed_cut_copper_stairs",                  67,  76  }, // facing east  - added, data value not correct
	{ "minecraft:waxed_cut_copper_stairs",                  67,  77  }, // facing north - added, data value not correct
	{ "minecraft:waxed_cut_copper_stairs",                  67,  78  }, // facing south - added, data value not correct
	{ "minecraft:waxed_cut_copper_stairs",                  67,  79  }, // facing west  - added, data value not correct
	{ "minecraft:waxed_exposed_cut_copper_stairs",          67,  80  }, // facing east  - added, data value not correct
	{ "minecraft:waxed_exposed_cut_copper_stairs",          67,  81  }, // facing north - added, data value not correct
	{ "minecraft:waxed_exposed_cut_copper_stairs",          67,  82  }, // facing south - added, data value not correct
	{ "minecraft:waxed_exposed_cut_copper_stairs",          67,  83  }, // facing west  - added, data value not correct
	{ "minecraft:waxed_weathered_cut_copper_stairs",        67,  84  }, // facing east  - added, data value not correct
	{ "minecraft:waxed_weathered_cut_copper_stairs",        67,  85  }, // facing north - added, data value not correct
	{ "minecraft:waxed_weathered_cut_copper_stairs",        67,  86  }, // facing south - added, data value not correct
	{ "minecraft:waxed_weathered_cut_copper_stairs",        67,  87  }, // facing west  - added, data value not correct
	{ "minecraft:waxed_oxidized_cut_copper_stairs",         67,  88  }, // facing east  - added, data value not correct
	{ "minecraft:waxed_oxidized_cut_copper_stairs",         67,  89  }, // facing north - added, data value not correct
	{ "minecraft:waxed_oxidized_cut_copper_stairs",         67,  90  }, // facing south - added, data value not correct
	{ "minecraft:waxed_oxidized_cut_copper_stairs",         67,  91  }, // facing west  - added, data value not correct
	{ "minecraft:cobbled_deepslate_stairs",                 67,  92  }, // facing east  - added, data value not correct
	{ "minecraft:cobbled_deepslate_stairs",                 67,  93  }, // facing north - added, data value not correct
	{ "minecraft:cobbled_deepslate_stairs",                 67,  94  }, // facing south - added, data value not correct
	{ "minecraft:cobbled_deepslate_stairs",                 67,  95  }, // facing west  - added, data value not correct
	{ "minecraft:polished_deepslate_stairs",                67,  96  }, // facing east  - added, data value not correct
	{ "minecraft:polished_deepslate_stairs",                67,  97  }, // facing north - added, data value not correct
	{ "minecraft:polished_deepslate_stairs",                67,  98  }, // facing south - added, data value not correct
	{ "minecraft:polished_deepslate_stairs",                67,  99  }, // facing west  - added, data value not correct
	{ "minecraft:deepslate_brick_stairs",                   67,  100 }, // facing east  - added, data value not correct
	{ "minecraft:deepslate_brick_stairs",                   67,  101 }, // facing north - added, data value not correct
	{ "minecraft:deepslate_brick_stairs",                   67,  102 }, // facing south - added, data value not correct
	{ "minecraft:deepslate_brick_stairs",                   67,  103 }, // facing west  - added, data value not correct
	{ "minecraft:deepslate_tile_stairs",                    67,  104 }, // facing east  - added, data value not correct
	{ "minecraft:deepslate_tile_stairs",                    67,  105 }, // facing north - added, data value not correct
	{ "minecraft:deepslate_tile_stairs",                    67,  106 }, // facing south - added, data value not correct
	{ "minecraft:deepslate_tile_stairs",                    67,  107 }, // facing west  - added, data value not correct
	{ "minecraft:oak_wall_sign",                            68,  0   }, // facing east
	{ "minecraft:oak_wall_sign",                            68,  1   }, // facing north
	{ "minecraft:oak_wall_sign",                            68,  2   }, // facing south
	{ "minecraft:oak_wall_sign",                            68,  3   }, // facing west
    { "minecraft:spruce_wall_sign",                         68,  4   }, // facing east
    { "minecraft:spruce_wall_sign",                         68,  5   }, // facing north
    { "minecraft:spruce_wall_sign",                         68,  6   }, // facing south
    { "minecraft:spruce_wall_sign",                         68,  7   }, // facing west
	{ "minecraft:birch_wall_sign",                          68,  8   }, // facing east
	{ "minecraft:birch_wall_sign",                          68,  9   }, // facing north
	{ "minecraft:birch_wall_sign",                          68,  10  }, // facing south
	{ "minecraft:birch_wall_sign",                          68,  11  }, // facing west
	{ "minecraft:jungle_wall_sign",                         68,  12  }, // facing east
	{ "minecraft:jungle_wall_sign",                         68,  13  }, // facing north
	{ "minecraft:jungle_wall_sign",                         68,  14  }, // facing south
	{ "minecraft:jungle_wall_sign",                         68,  15  }, // facing west
	{ "minecraft:acacia_wall_sign",                         68,  16  }, // facing east
	{ "minecraft:acacia_wall_sign",                         68,  17  }, // facing north
	{ "minecraft:acacia_wall_sign",                         68,  18  }, // facing south
	{ "minecraft:acacia_wall_sign",                         68,  19  }, // facing west
    { "minecraft:dark_oak_wall_sign",                       68,  20  }, // facing east
	{ "minecraft:dark_oak_wall_sign",                       68,  21  }, // facing north
	{ "minecraft:dark_oak_wall_sign",                       68,  22  }, // facing south
	{ "minecraft:dark_oak_wall_sign",                       68,  23  }, // facing west
    { "minecraft:crimson_wall_sign",                        68,  24  }, // facing east
	{ "minecraft:crimson_wall_sign",                        68,  25  }, // facing north
	{ "minecraft:crimson_wall_sign",                        68,  26  }, // facing south
	{ "minecraft:crimson_wall_sign",                        68,  27  }, // facing west
    { "minecraft:warped_wall_sign",                         68,  28  }, // facing east
	{ "minecraft:warped_wall_sign",                         68,  29  }, // facing north
	{ "minecraft:warped_wall_sign",                         68,  30  }, // facing south
	{ "minecraft:warped_wall_sign",                         68,  31  }, // facing west
	{ "minecraft:lever",                                    69,  0   }, 
	{ "minecraft:stone_pressure_plate",                     70,  0   },
	{ "minecraft:polished_blackstone_pressure_plate",       70,  0   },
    { "minecraft:iron_door",                                71,  0   }, // facing east
	{ "minecraft:iron_door",                                71,  1   }, // facing north
	{ "minecraft:iron_door",                                71,  2   }, // facing south
	{ "minecraft:iron_door",                                71,  3   }, // facing west
	{ "minecraft:oak_pressure_plate",                       72,  0   }, 
	{ "minecraft:spruce_pressure_plate",                    72,  1   }, 
	{ "minecraft:birch_pressure_plate",                     72,  2   }, 
	{ "minecraft:jungle_pressure_plate",                    72,  3   }, 
	{ "minecraft:acacia_pressure_plate",                    72,  4   }, 
	{ "minecraft:dark_oak_pressure_plate",                  72,  5   }, 
	{ "minecraft:crimson_pressure_plate",                   72,  6   }, 
	{ "minecraft:warped_pressure_plate",                    72,  7   }, 
	{ "minecraft:redstone_ore",                             73,  0   }, 
	{ "minecraft:deepslate_redstone_ore",                   73,  1   }, 
	{ "minecraft:lit_redstone_ore",                         74,  0   }, 
	{ "minecraft:lit_deepslate_redstone_ore",               74,  1   }, 
	{ "minecraft:redstone_torch",                           75,  0   }, // unlit
	{ "minecraft:redstone_torch",                           75,  1   }, // lit
	{ "minecraft:redstone_wall_torch",                      76,  0   }, // unlit - facing east
	{ "minecraft:redstone_wall_torch",                      76,  1   }, // unlit - facing north
	{ "minecraft:redstone_wall_torch",                      76,  2   }, // unlit - facing south
	{ "minecraft:redstone_wall_torch",                      76,  3   }, // unlit - facing up
	{ "minecraft:redstone_wall_torch",                      76,  4   }, // unlit - facing west
	{ "minecraft:redstone_wall_torch",                      76,  5   }, // lit   - facing east
	{ "minecraft:redstone_wall_torch",                      76,  6   }, // lit   - facing north
	{ "minecraft:redstone_wall_torch",                      76,  7   }, // lit   - facing south
	{ "minecraft:redstone_wall_torch",                      76,  8   }, // lit   - facing up
	{ "minecraft:redstone_wall_torch",                      76,  9   }, // lit   - facing west
	{ "minecraft:stone_button",                             77,  0   }, 
	{ "minecraft:polished_blackstone_button",               77,  1   }, // Added after flattening, data value not correct
	{ "minecraft:snow",                                     78,  0   }, // layer
	{ "minecraft:ice",                                      79,  0   }, 
	{ "minecraft:blue_ice",                                 79,  1   }, // Added, data value not correct
	{ "minecraft:snow_block",                               80,  0   }, 
	{ "minecraft:cactus",                                   81,  0   }, 
	{ "minecraft:clay",                                     82,  0   }, 
	{ "minecraft:sugar_cane",                               83,  0   }, 
	{ "minecraft:jukebox",                                  84,  0   }, 
	{ "minecraft:oak_fence",                                85,  0   }, 
	{ "minecraft:crimson_fence",                            85,  1   }, 
	{ "minecraft:warped_fence",                             85,  2   }, 
	{ "minecraft:pumpkin",                                  86,  0   }, 
	{ "minecraft:netherrack",                               87,  0   }, 
	{ "minecraft:soul_sand",                                88,  0   }, 
	{ "minecraft:glowstone",                                89,  0   }, 
	{ "minecraft:portal",                                   90,  0   }, // portal long edge runs east-west
	{ "minecraft:portal",                                   90,  1   }, // portal long edge runs north-south
	{ "minecraft:jack_o_lantern",                           91,  0   }, // facing east
	{ "minecraft:jack_o_lantern",                           91,  1   }, // facing north
	{ "minecraft:jack_o_lantern",                           91,  2   }, // facing south
	{ "minecraft:jack_o_lantern",                           91,  3   }, // facing west
	{ "minecraft:cake",                                     92,  0   },
	{ "minecraft:candle_cake",                              92,  1   },
	{ "minecraft:white_candle_cake",                        92,  2   },
	{ "minecraft:orange_candle_cake",                       92,  3   },
	{ "minecraft:magenta_candle_cake",                      92,  4   },
	{ "minecraft:light_blue_candle_cake",                   92,  5   },
	{ "minecraft:yellow_candle_cake",                       92,  6   },
	{ "minecraft:lime_candle_cake",                         92,  7   },
	{ "minecraft:pink_candle_cake",                         92,  8   },
	{ "minecraft:gray_candle_cake",                         92,  9   },
	{ "minecraft:light_gray_candle_cake",                   92,  10  },
	{ "minecraft:cyan_candle_cake",                         92,  11  },
	{ "minecraft:purple_candle_cake",                       92,  12  },
	{ "minecraft:blue_candle_cake",                         92,  13  },
	{ "minecraft:brown_candle_cake",                        92,  14  },
	{ "minecraft:green_candle_cake",                        92,  15  },
	{ "minecraft:red_candle_cake",                          92,  16  },
	{ "minecraft:black_candle_cake",                        92,  17  }, 
	{ "minecraft:repeater",                                 93,  0   }, // unpowered repeater
	{ "minecraft:repeater",                                 94,  0   }, // powered repeater
	{ "minecraft:white_stained_glass",                      95,  0   },
	{ "minecraft:orange_stained_glass",                     95,  1   },
	{ "minecraft:magenta_stained_glass",                    95,  2   },
	{ "minecraft:light_blue_stained_glass",                 95,  3   },
	{ "minecraft:yellow_stained_glass",                     95,  4   },
	{ "minecraft:lime_stained_glass",                       95,  5   },
	{ "minecraft:pink_stained_glass",                       95,  6   },
	{ "minecraft:gray_stained_glass",                       95,  7   },
	{ "minecraft:light_gray_stained_glass",                 95,  8   },
	{ "minecraft:cyan_stained_glass",                       95,  9   },
	{ "minecraft:purple_stained_glass",                     95,  10  },
	{ "minecraft:blue_stained_glass",                       95,  11  },
	{ "minecraft:brown_stained_glass",                      95,  12  },
	{ "minecraft:green_stained_glass",                      95,  13  },
	{ "minecraft:red_stained_glass",                        95,  14  },
	{ "minecraft:black_stained_glass",                      95,  15  },
	{ "minecraft:oak_trapdoor",                             96,  0   },
	{ "minecraft:spruce_trapdoor",                          96,  1   },
	{ "minecraft:birch_trapdoor",                           96,  2   },
	{ "minecraft:jungle_trapdoor",                          96,  3   },
	{ "minecraft:acacia_trapdoor",                          96,  4   },
	{ "minecraft:dark_oak_trapdoor",                        96,  5   },
	{ "minecraft:crimson_trapdoor",                         96,  6   },
	{ "minecraft:warped_trapdoor",                          96,  7   },
	{ "minecraft:infested_stone",                           97,  0   },
	{ "minecraft:infested_cobblestone",                     97,  1   },
	{ "minecraft:infested_stone_bricks",                    97,  2   },
	{ "minecraft:infested_cracked_stone_bricks",            97,  3   },
	{ "minecraft:infested_mossy_stone_bricks",              97,  4   },
	{ "minecraft:infested_chiseled_stone_bricks",           97,  5   },
	{ "minecraft:infested_deepslate",                       97,  6   },
	{ "minecraft:stone_bricks",                             98,  0   },
	{ "minecraft:cracked_stone_bricks",                     98,  1   },
	{ "minecraft:mossy_stone_bricks",                       98,  2   },
	{ "minecraft:chiseled_stone_bricks",                    98,  3   },
	{ "minecraft:brown_mushroom_block",                     99,  0   },
	{ "minecraft:mushroom_stem",                            99,  1   },
	{ "minecraft:red_mushroom_block",                       100, 0   },
	{ "minecraft:iron_bars",                                101, 0   },
	{ "minecraft:glass_pane",                               102, 0   },
	{ "minecraft:melon",                                    103, 0   },
	{ "minecraft:pumpkin_stem",                             104, 0   },
	{ "minecraft:attached_pumpkin_stem",                    104, 1   },
	{ "minecraft:melon_stem",                               105, 0   }, 
	{ "minecraft:attached_melon_stem",                      105, 1   }, 
	{ "minecraft:vine",                                     106, 0   }, 
	{ "minecraft:oak_fence_gate",                           107, 0   }, 
	{ "minecraft:crimson_fence_gate",                       107, 1   }, 
	{ "minecraft:warped_fence_gate",                        107, 2   }, 
	{ "minecraft:brick_stairs",                             108, 0   }, // facing east 
	{ "minecraft:brick_stairs",                             108, 1   }, // facing north
	{ "minecraft:brick_stairs",                             108, 2   }, // facing south
	{ "minecraft:brick_stairs",                             108, 3   }, // facing west
	{ "minecraft:end_stone_brick_stairs",                   108, 4   }, // facing east  - added, data value not correct
	{ "minecraft:end_stone_brick_stairs",                   108, 5   }, // facing north - added, data value not correct
	{ "minecraft:end_stone_brick_stairs",                   108, 6   }, // facing south - added, data value not correct
	{ "minecraft:end_stone_brick_stairs",                   108, 7   }, // facing west  - added, data value not correct
	{ "minecraft:stone_brick_stairs",                       109, 0   }, // facing east 
	{ "minecraft:stone_brick_stairs",                       109, 1   }, // facing north
	{ "minecraft:stone_brick_stairs",                       109, 2   }, // facing south
	{ "minecraft:stone_brick_stairs",                       109, 3   }, // facing west
	{ "minecraft:mossy_stone_brick_stairs",                 109, 4   }, // facing east  - added, data value not correct
	{ "minecraft:mossy_stone_brick_stairs",                 109, 5   }, // facing north - added, data value not correct
	{ "minecraft:mossy_stone_brick_stairs",                 109, 6   }, // facing south - added, data value not correct
	{ "minecraft:mossy_stone_brick_stairs",                 109, 7   }, // facing west  - added, data value not correct
	{ "minecraft:mycelium",                                 110, 0   }, 
	{ "minecraft:lily_pad",                                 111, 0   },
	{ "minecraft:nether_bricks",                            112, 0   },
	{ "minecraft:nether_brick_fence",                       113, 0   },
	{ "minecraft:nether_brick_stairs",                      114, 0   }, // facing east 
	{ "minecraft:nether_brick_stairs",                      114, 1   }, // facing north
	{ "minecraft:nether_brick_stairs",                      114, 2   }, // facing south
	{ "minecraft:nether_brick_stairs",                      114, 3   }, // facing west 
	{ "minecraft:red_nether_brick_stairs",                  114, 4   }, // facing east  - added, data value not correct
	{ "minecraft:red_nether_brick_stairs",                  114, 5   }, // facing north - added, data value not correct
	{ "minecraft:red_nether_brick_stairs",                  114, 6   }, // facing south - added, data value not correct
	{ "minecraft:red_nether_brick_stairs",                  114, 7   }, // facing west  - added, data value not correct
	{ "minecraft:nether_wart",                              115, 0   }, 
	{ "minecraft:enchanting_table",                         116, 0   }, 
	{ "minecraft:brewing_stand",                            117, 0   }, 
	{ "minecraft:cauldron",                                 118, 0   }, 
	{ "minecraft:lava_cauldron",                            118, 1   }, 
	{ "minecraft:powder_snow_cauldron",                     118, 2   }, 
	{ "minecraft:water_cauldron",                           118, 3   }, 
	{ "minecraft:end_portal",                               119, 0   }, 
	{ "minecraft:end_portal_frame",                         120, 0   }, // facing east 
	{ "minecraft:end_portal_frame",                         120, 1   }, // facing north
	{ "minecraft:end_portal_frame",                         120, 2   }, // facing south
	{ "minecraft:end_portal_frame",                         120, 3   }, // facing west 
	{ "minecraft:end_stone",                                121, 0   }, 
	{ "minecraft:dragon_egg",                               122, 0   }, 
	{ "minecraft:redstone_lamp",                            123, 0   }, // inactive
	{ "minecraft:redstone_lamp",                            124, 0   }, // active
	{ "minecraft:double_oak_slab",                          125, 0   },
    { "minecraft:double_oak_slab",                          125, 1   }, // waterlogged
    { "minecraft:double_spruce_slab",                       125, 2   },
    { "minecraft:double_spruce_slab",                       125, 3   }, // waterlogged
    { "minecraft:double_birch_slab",                        125, 4   },
    { "minecraft:double_birch_slab",                        125, 5   }, // waterlogged
    { "minecraft:double_jungle_slab",                       125, 6   },
    { "minecraft:double_jungle_slab",                       125, 7   }, // waterlogged
    { "minecraft:double_acacia_slab",                       125, 8   },
    { "minecraft:double_acacia_slab",                       125, 9   }, // waterlogged
    { "minecraft:double_dark_oak_slab",                     125, 10  },
    { "minecraft:double_dark_oak_slab",                     125, 11  }, // waterlogged
    { "minecraft:double_crimson_slab",                      125, 12  },
    { "minecraft:double_crimson_slab",                      125, 13  }, // waterlogged
    { "minecraft:double_warped_slab",                       125, 14  },
    { "minecraft:double_warped_slab",                       125, 15  }, // waterlogged
	{ "minecraft:oak_slab",                                 126, 0   }, // bottom
	{ "minecraft:oak_slab",                                 126, 1   }, // top
    { "minecraft:oak_slab",                                 126, 2   }, // bottom waterlogged
	{ "minecraft:oak_slab",                                 126, 3   }, // top waterlogged
	{ "minecraft:spruce_slab",                              126, 4   }, // bottom
	{ "minecraft:spruce_slab",                              126, 5   }, // top
	{ "minecraft:spruce_slab",                              126, 6   }, // bottom waterlogged
	{ "minecraft:spruce_slab",                              126, 7   }, // top waterlogged
	{ "minecraft:birch_slab",                               126, 8   }, // bottom
	{ "minecraft:birch_slab",                               126, 9   }, // top
	{ "minecraft:birch_slab",                               126, 10  }, // bottom waterlogged
	{ "minecraft:birch_slab",                               126, 11  }, // top waterlogged
	{ "minecraft:jungle_slab",                              126, 12  }, // bottom
	{ "minecraft:jungle_slab",                              126, 13  }, // top
	{ "minecraft:jungle_slab",                              126, 14  }, // bottom waterlogged
	{ "minecraft:jungle_slab",                              126, 15  }, // top waterlogged
	{ "minecraft:acacia_slab",                              126, 16  }, // bottom
	{ "minecraft:acacia_slab",                              126, 17  }, // top
	{ "minecraft:acacia_slab",                              126, 18  }, // bottom waterlogged
	{ "minecraft:acacia_slab",                              126, 19  }, // top waterlogged
	{ "minecraft:dark_oak_slab",                            126, 20  }, // bottom
	{ "minecraft:dark_oak_slab",                            126, 21  }, // top
	{ "minecraft:dark_oak_slab",                            126, 22  }, // bottom waterlogged
	{ "minecraft:dark_oak_slab",                            126, 23  }, // top waterlogged
	{ "minecraft:crimson_slab",                             126, 24  }, // bottom
	{ "minecraft:crimson_slab",                             126, 25  }, // top
	{ "minecraft:crimson_slab",                             126, 26  }, // bottom waterlogged
	{ "minecraft:crimson_slab",                             126, 27  }, // top waterlogged
	{ "minecraft:warped_slab",                              126, 28  }, // bottom
	{ "minecraft:warped_slab",                              126, 29  }, // top
	{ "minecraft:warped_slab",                              126, 30  }, // bottom waterlogged
	{ "minecraft:warped_slab",                              126, 31  }, // top waterlogged
	{ "minecraft:cocoa",                                    127, 0   }, 
	{ "minecraft:sandstone_stairs",                         128, 0   }, // facing east 
	{ "minecraft:sandstone_stairs",                         128, 1   }, // facing north
	{ "minecraft:sandstone_stairs",                         128, 2   }, // facing south
	{ "minecraft:sandstone_stairs",                         128, 3   }, // facing west
    { "minecraft:smooth_sandstone_stairs",                  128, 4   }, // facing east  - added, data value not correct
	{ "minecraft:smooth_sandstone_stairs",                  128, 5   }, // facing north - added, data value not correct
	{ "minecraft:smooth_sandstone_stairs",                  128, 6   }, // facing south - added, data value not correct
	{ "minecraft:smooth_sandstone_stairs",                  128, 7   }, // facing west  - added, data value not correct
	{ "minecraft:emerald_ore",                              129, 0   },
	{ "minecraft:deepslate_emerald_ore",                    129, 0   },
	{ "minecraft:ender_chest",                              130, 0   }, // facing east 
	{ "minecraft:ender_chest",                              130, 1   }, // facing north
	{ "minecraft:ender_chest",                              130, 2   }, // facing south
	{ "minecraft:ender_chest",                              130, 3   }, // facing west 
	{ "minecraft:tripwire_hook",                            131, 0   }, 
	{ "minecraft:tripwire",                                 132, 0   }, 
	{ "minecraft:emerald_block",                            133, 0   }, 
    { "minecraft:spruce_stairs",                            134, 0   }, // facing east 
	{ "minecraft:spruce_stairs",                            134, 1   }, // facing north 
	{ "minecraft:spruce_stairs",                            134, 2   }, // facing south 
	{ "minecraft:spruce_stairs",                            134, 3   }, // facing west 
    { "minecraft:birch_stairs",                             135, 0   }, // facing east 
	{ "minecraft:birch_stairs",                             135, 1   }, // facing north 
	{ "minecraft:birch_stairs",                             135, 2   }, // facing south 
	{ "minecraft:birch_stairs",                             135, 3   }, // facing west 
    { "minecraft:jungle_stairs",                            136, 0   }, // facing east 
	{ "minecraft:jungle_stairs",                            136, 1   }, // facing north 
	{ "minecraft:jungle_stairs",                            136, 2   }, // facing south 
	{ "minecraft:jungle_stairs",                            136, 3   }, // facing west 
	{ "minecraft:command_block",                            137, 0   },
	{ "minecraft:beacon",                                   138, 0   },
	{ "minecraft:cobblestone_wall",                         139, 0   },
	{ "minecraft:mossy_cobblestone_wall",                   139, 1   },
	{ "minecraft:stone_brick_wall",                         139, 2   },
	{ "minecraft:mossy_stone_brick_wall",                   139, 3   },
	{ "minecraft:andesite_wall",                            139, 4   },
	{ "minecraft:diorite_wall",                             139, 5   },
	{ "minecraft:granite_wall",                             139, 6   },
	{ "minecraft:sandstone_wall",                           139, 7   },
	{ "minecraft:red_sandstone_wall",                       139, 8   },
	{ "minecraft:brick_wall",                               139, 9   },
	{ "minecraft:prismarine_wall",                          139, 10  },
	{ "minecraft:nether_brick_wall",                        139, 11  },
	{ "minecraft:red_nether_brick_wall",                    139, 12  },
	{ "minecraft:end_stone_brick_wall",                     139, 13  },
	{ "minecraft:blackstone_wall",                          139, 14  },
	{ "minecraft:polished_blackstone_wall",                 139, 15  },
	{ "minecraft:polished_blackstone_brick_wall",           139, 16  },
	{ "minecraft:cobbled_deepslate_wall",                   139, 17  },
	{ "minecraft:polished_deepslate_wall",                  139, 18  },
	{ "minecraft:deepslate_brick_wall",                     139, 19  },
	{ "minecraft:deepslate_tile_wall",                      139, 20  },
    { "minecraft:flower_pot",                               140, 0   },
	{ "minecraft:potted_dandelion",                         140, 1   },
	{ "minecraft:potted_poppy",                             140, 2   },
	{ "minecraft:potted_blue_orchid",                       140, 3   },
	{ "minecraft:potted_allium",                            140, 4   },
	{ "minecraft:potted_azure_bluet",                       140, 5   },
	{ "minecraft:potted_red_tulip",                         140, 6   },
	{ "minecraft:potted_orange_tulip",                      140, 7   },
	{ "minecraft:potted_white_tulip",                       140, 8   },
	{ "minecraft:potted_pink_tulip",                        140, 9   },
	{ "minecraft:potted_oxeye_daisy",                       140, 10  },
	{ "minecraft:potted_cornflower",                        140, 11  },
	{ "minecraft:potted_lily_of_the_valley",                140, 12  },
	{ "minecraft:potted_wither_rose",                       140, 13  },
	{ "minecraft:potted_oak_sapling",                       140, 14  },
	{ "minecraft:potted_spruce_sapling",                    140, 15  },
	{ "minecraft:potted_birch_sapling",                     140, 16  },
	{ "minecraft:potted_jungle_sapling",                    140, 17  },
	{ "minecraft:potted_acacia_sapling",                    140, 18  },
	{ "minecraft:potted_dark_oak_sapling",                  140, 19  },
	{ "minecraft:potted_red_mushroom",                      140, 20  },
	{ "minecraft:potted_brown_mushroom",                    140, 21  },
	{ "minecraft:potted_fern",                              140, 22  },
	{ "minecraft:potted_dead_bush",                         140, 23  },
	{ "minecraft:potted_cactus",                            140, 24  },
	{ "minecraft:potted_bamboo",                            140, 25  },
	{ "minecraft:potted_azalea_bush",                       140, 26  },
	{ "minecraft:potted_flowering_azalea_bush",             140, 27  },
	{ "minecraft:potted_crimson_fungus",                    140, 28  },
	{ "minecraft:potted_warped_fungus",                     140, 29  },
	{ "minecraft:potted_crimson_roots",                     140, 30  },
	{ "minecraft:potted_warped_roots",                      140, 31  },
	{ "minecraft:carrots",                                  141, 0   }, 
	{ "minecraft:potatoes",                                 142, 0   }, 
	{ "minecraft:oak_button",                               143, 0   }, 
	{ "minecraft:spruce_button",                            143, 1   }, 
	{ "minecraft:birch_button",                             143, 2   }, 
	{ "minecraft:jungle_button",                            143, 3   }, 
	{ "minecraft:acacia_button",                            143, 4   }, 
	{ "minecraft:dark_oak_button",                          143, 5   }, 
	{ "minecraft:crimson_button",                           143, 6   }, // Added after flattening, data value not correct
	{ "minecraft:warped_button",                            143, 7   }, // Added after flattening, data value not correct
	{ "minecraft:skeleton_skull",                           144, 0   },
	{ "minecraft:wither_skeleton_skull",                    144, 1   },
	{ "minecraft:zombie_head",                              144, 2   },
	{ "minecraft:player_head",                              144, 3   },
	{ "minecraft:creeper_head",                             144, 4   },
	{ "minecraft:dragon_head",                              144, 5   },
	{ "minecraft:skeleton_wall_skull",                      144, 6   }, // facing east
	{ "minecraft:skeleton_wall_skull",                      144, 7   }, // facing north
	{ "minecraft:skeleton_wall_skull",                      144, 8   }, // facing south
	{ "minecraft:skeleton_wall_skull",                      144, 9   }, // facing west
	{ "minecraft:wither_skeleton_wall_skull",               144, 10  }, // facing east
	{ "minecraft:wither_skeleton_wall_skull",               144, 11  }, // facing north
	{ "minecraft:wither_skeleton_wall_skull",               144, 12  }, // facing south
	{ "minecraft:wither_skeleton_wall_skull",               144, 13  }, // facing west
	{ "minecraft:zombie_wall_head",                         144, 14  }, // facing east
	{ "minecraft:zombie_wall_head",                         144, 15  }, // facing north
	{ "minecraft:zombie_wall_head",                         144, 16  }, // facing south
	{ "minecraft:zombie_wall_head",                         144, 17  }, // facing west
	{ "minecraft:player_wall_head",                         144, 18  }, // facing east
	{ "minecraft:player_wall_head",                         144, 19  }, // facing north
	{ "minecraft:player_wall_head",                         144, 20  }, // facing south
	{ "minecraft:player_wall_head",                         144, 21  }, // facing west
	{ "minecraft:creeper_wall_head",                        144, 22  }, // facing east
	{ "minecraft:creeper_wall_head",                        144, 23  }, // facing north
	{ "minecraft:creeper_wall_head",                        144, 24  }, // facing south
	{ "minecraft:creeper_wall_head",                        144, 25  }, // facing west
	{ "minecraft:dragon_wall_head",                         144, 26  }, // facing east
	{ "minecraft:dragon_wall_head",                         144, 27  }, // facing north
	{ "minecraft:dragon_wall_head",                         144, 28  }, // facing south
	{ "minecraft:dragon_wall_head",                         144, 29  }, // facing west
	{ "minecraft:anvil",                                    145, 0   }, // facing east
	{ "minecraft:anvil",                                    145, 1   }, // facing north
	{ "minecraft:anvil",                                    145, 2   }, // facing south
	{ "minecraft:anvil",                                    145, 3   }, // facing west 
	{ "minecraft:chipped_anvil",                            145, 4   }, // facing east
	{ "minecraft:chipped_anvil",                            145, 5   }, // facing north
	{ "minecraft:chipped_anvil",                            145, 6   }, // facing south
	{ "minecraft:chipped_anvil",                            145, 7   }, // facing west 
	{ "minecraft:damaged_anvil",                            145, 8   }, // facing east
	{ "minecraft:damaged_anvil",                            145, 9   }, // facing north
	{ "minecraft:damaged_anvil",                            145, 10  }, // facing south
	{ "minecraft:damaged_anvil",                            145, 11  }, // facing west 
	{ "minecraft:trapped_chest",                            146, 0   }, // facing east 
	{ "minecraft:trapped_chest",                            146, 1   }, // facing north 
	{ "minecraft:trapped_chest",                            146, 2   }, // facing south 
	{ "minecraft:trapped_chest",                            146, 3   }, // facing west  
	{ "minecraft:light_weighted_pressure_plate",            147, 0   }, 
	{ "minecraft:heavy_weighted_pressure_plate",            148, 0   }, 
	{ "minecraft:comparator",                               149, 0   }, // unpowered - facing east 
	{ "minecraft:comparator",                               149, 1   }, // unpowered - facing north 
	{ "minecraft:comparator",                               149, 2   }, // unpowered - facing south 
	{ "minecraft:comparator",                               149, 3   }, // unpowered - facing west 
	{ "minecraft:comparator",                               150, 0   }, // powered   - facing east 
	{ "minecraft:comparator",                               150, 1   }, // powered   - facing north 
	{ "minecraft:comparator",                               150, 2   }, // powered   - facing south 
	{ "minecraft:comparator",                               150, 3   }, // powered   - facing west  
	{ "minecraft:daylight_detector",                        151, 0   }, 
	{ "minecraft:redstone_block",                           152, 0   }, 
	{ "minecraft:nether_quartz_ore",                        153, 0   }, 
	{ "minecraft:hopper",                                   154, 0   }, 
	{ "minecraft:quartz_block",                             155, 0   }, 
	{ "minecraft:chiseled_quartz_block",                    155, 1   }, 
	{ "minecraft:smooth_quartz",                            155, 2   }, // Added, data value not correct
	{ "minecraft:quartz_pillar",                            155, 3   }, // oriented east-west 
	{ "minecraft:quartz_pillar",                            155, 4   }, // oriented vertically  
	{ "minecraft:quartz_pillar",                            155, 5   }, // oriented north-south 
	{ "minecraft:quartz_bricks",                            155, 6   }, 
	{ "minecraft:quartz_stairs",                            156, 0   }, // facing east 
	{ "minecraft:quartz_stairs",                            156, 1   }, // facing north
	{ "minecraft:quartz_stairs",                            156, 2   }, // facing south
	{ "minecraft:quartz_stairs",                            156, 3   }, // facing west 
    { "minecraft:smooth_quartz_stairs",                     156, 4   }, // facing east  - added, data value not correct
	{ "minecraft:smooth_quartz_stairs",                     156, 5   }, // facing north - added, data value not correct
	{ "minecraft:smooth_quartz_stairs",                     156, 6   }, // facing south - added, data value not correct
	{ "minecraft:smooth_quartz_stairs",                     156, 7   }, // facing west  - added, data value not correct
	{ "minecraft:activator_rail",                           157, 0   },
	{ "minecraft:dropper",                                  158, 0   }, 
	{ "minecraft:white_terracotta",                         159, 0   }, 
	{ "minecraft:orange_terracotta",                        159, 1   }, 
	{ "minecraft:magenta_terracotta",                       159, 2   }, 
	{ "minecraft:light_blue_terracotta",                    159, 3   }, 
	{ "minecraft:yellow_terracotta",                        159, 4   }, 
	{ "minecraft:lime_terracotta",                          159, 5   }, 
	{ "minecraft:pink_terracotta",                          159, 6   }, 
	{ "minecraft:gray_terracotta",                          159, 7   }, 
	{ "minecraft:light_gray_terracotta",                    159, 8   }, 
	{ "minecraft:cyan_terracotta",                          159, 9   }, 
	{ "minecraft:purple_terracotta",                        159, 10  }, 
	{ "minecraft:blue_terracotta",                          159, 11  }, 
	{ "minecraft:brown_terracotta",                         159, 12  }, 
	{ "minecraft:green_terracotta",                         159, 13  }, 
	{ "minecraft:red_terracotta",                           159, 14  }, 
	{ "minecraft:black_terracotta",                         159, 15  },	
    { "minecraft:white_stained_glass_pane",                 160,  0  },
	{ "minecraft:orange_stained_glass_pane",                160,  1  },
	{ "minecraft:magenta_stained_glass_pane",               160,  2  },
	{ "minecraft:light_blue_stained_glass_pane",            160,  3  },
	{ "minecraft:yellow_stained_glass_pane",                160,  4  },
	{ "minecraft:lime_stained_glass_pane",                  160,  5  },
	{ "minecraft:pink_stained_glass_pane",                  160,  6  },
	{ "minecraft:gray_stained_glass_pane",                  160,  7  },
	{ "minecraft:light_gray_stained_glass_pane",            160,  8  },
	{ "minecraft:cyan_stained_glass_pane",                  160,  9  },
	{ "minecraft:purple_stained_glass_pane",                160,  10 },
	{ "minecraft:blue_stained_glass_pane",                  160,  11 },
	{ "minecraft:brown_stained_glass_pane",                 160,  12 },
	{ "minecraft:green_stained_glass_pane",                 160,  13 },
	{ "minecraft:red_stained_glass_pane",                   160,  14 },
	{ "minecraft:black_stained_glass_pane",                 160,  15 },
	{ "minecraft:acacia_leaves",                            161, 0   }, // check for decay (if no log within 4 blocks decay)
	{ "minecraft:dark_oak_leaves",                          161, 1   }, // check for decay (if no log within 4 blocks decay)
	{ "minecraft:azalea_leaves",                            161, 2   }, // check for decay (if no log within 4 blocks decay)
	{ "minecraft:flowering_azalea_leaves",                  161, 3   }, // check for decay (if no log within 4 blocks decay)
	{ "minecraft:acacia_leaves",                            161, 4   }, // persistent
	{ "minecraft:dark_oak_leaves",                          161, 5   }, // persistent
	{ "minecraft:azalea_leaves",                            161, 6   }, // persistent
	{ "minecraft:flowering_azalea_leaves",                  161, 7   }, // persistent
	{ "minecraft:acacia_leaves",                            161, 8   }, // persistent (12-15 same as 8-11)
	{ "minecraft:dark_oak_leaves",                          161, 9   }, // persistent (12-15 same as 8-11)
	{ "minecraft:azalea_leaves",                            161, 10  }, // persistent (12-15 same as 8-11)
	{ "minecraft:flowering_azalea_leaves",                  161, 11  }, // persistent (12-15 same as 8-11)
	{ "minecraft:acacia_log",                               162, 0   }, // up-down
	{ "minecraft:dark_oak_log",                             162, 1   }, // up-down
	{ "minecraft:crimson_stem",                             162, 2   }, // up-down
	{ "minecraft:warped_stem",                              162, 3   }, // up-down
    { "minecraft:acacia_log",                               162, 4   }, // east-west
	{ "minecraft:dark_oak_log",                             162, 5   }, // east-west
	{ "minecraft:crimson_stem",                             162, 6   }, // east-west
	{ "minecraft:warped_stem",                              162, 7   }, // east-west
    { "minecraft:acacia_log",                               162, 8   }, // north-south
	{ "minecraft:dark_oak_log",                             162, 9   }, // north-south
	{ "minecraft:crimson_stem",                             162, 10  }, // north-south
	{ "minecraft:warped_stem",                              162, 11  }, // north-south
    { "minecraft:acacia_wood",                              162, 12  }, // up-down
	{ "minecraft:dark_oak_wood",                            162, 13  }, // up-down
	{ "minecraft:crimson_hyphae",                           162, 14  },
	{ "minecraft:warped_hyphae",                            162, 15  },
	{ "minecraft:stripped_oak_wood",                        162, 16  }, 
	{ "minecraft:stripped_spruce_wood",                     162, 17  }, 
	{ "minecraft:stripped_birch_wood",                      162, 18  }, 
	{ "minecraft:stripped_jungle_wood",                     162, 19  }, 
	{ "minecraft:stripped_acacia_wood",                     162, 20  }, 
	{ "minecraft:stripped_dark_oak_wood",                   162, 21  }, 
	{ "minecraft:stripped_crimson_hyphae",                  162, 22  },
	{ "minecraft:stripped_warped_hyphae",                   162, 23  },
	{ "minecraft:stripped_oak_log",                         162, 24  },
	{ "minecraft:stripped_spruce_log",                      162, 25  },
	{ "minecraft:stripped_birch_log",                       162, 26  },
	{ "minecraft:stripped_jungle_log",                      162, 27  },
	{ "minecraft:stripped_acacia_log",                      162, 28  },
	{ "minecraft:stripped_dark_oak_log",                    162, 29  },
	{ "minecraft:stripped_crimson_stem",                    162, 30  },
	{ "minecraft:stripped_warped_stem",                     162, 31  },
    { "minecraft:acacia_stairs",                            163, 0   }, // facing east 
	{ "minecraft:acacia_stairs",                            163, 1   }, // facing north 
	{ "minecraft:acacia_stairs",                            163, 2   }, // facing south 
	{ "minecraft:acacia_stairs",                            163, 3   }, // facing west
    { "minecraft:dark_oak_stairs",                          164, 0   }, // facing east
	{ "minecraft:dark_oak_stairs",                          164, 1   }, // facing north
	{ "minecraft:dark_oak_stairs",                          164, 2   }, // facing south
	{ "minecraft:dark_oak_stairs",                          164, 3   }, // facing west
    { "minecraft:crimson_stairs",                           164, 4   }, // facing east	  - Added after flattening, data value not correct
	{ "minecraft:crimson_stairs",                           164, 5   }, // facing north	  - Added after flattening, data value not correct
	{ "minecraft:crimson_stairs",                           164, 6   }, // facing south	  - Added after flattening, data value not correct
	{ "minecraft:crimson_stairs",                           164, 7   }, // facing west	  - Added after flattening, data value not correct
    { "minecraft:warped_stairs",                            164, 8   }, // facing east	  - Added after flattening, data value not correct
	{ "minecraft:warped_stairs",                            164, 9   }, // facing north	  - Added after flattening, data value not correct
	{ "minecraft:warped_stairs",                            164, 10  }, // facing south	  - Added after flattening, data value not correct
	{ "minecraft:warped_stairs",                            164, 11  }, // facing west	  - Added after flattening, data value not correct
	{ "minecraft:slime_block",                              165, 0   }, 
	{ "minecraft:barrier",                                  166, 0   }, 
	{ "minecraft:iron_trapdoor",                            167, 0   }, 
	{ "minecraft:prismarine",                               168, 0   }, 
	{ "minecraft:prismarine_bricks",                        168, 1   }, 
	{ "minecraft:dark_prismarine",                          168, 2   }, 
	{ "minecraft:sea_lantern",                              169, 0   }, 
	{ "minecraft:lantern",                                  169, 1   }, // Added, data value not correct
	{ "minecraft:soul_lantern",                             169, 2   }, // Added, data value not correct
	{ "minecraft:campfire",                                 169, 3   }, // Added, data value not correct
	{ "minecraft:hay_block",                                170, 0   }, // oriented east-west
	{ "minecraft:hay_block",                                170, 1   }, // oriented vertically 
	{ "minecraft:hay_block",                                170, 2   }, // oriented north-south
	{ "minecraft:white_carpet",                             171, 0   },
	{ "minecraft:orange_carpet",                            171, 1   },
	{ "minecraft:magenta_carpet",                           171, 2   },
	{ "minecraft:light_blue_carpet",                        171, 3   },
	{ "minecraft:yellow_carpet",                            171, 4   },
	{ "minecraft:lime_carpet",                              171, 5   },
	{ "minecraft:pink_carpet",                              171, 6   },
	{ "minecraft:gray_carpet",                              171, 7   },
	{ "minecraft:light_gray_carpet",                        171, 8   },
	{ "minecraft:cyan_carpet",                              171, 9   },
	{ "minecraft:purple_carpet",                            171, 10  },
	{ "minecraft:blue_carpet",                              171, 11  },
	{ "minecraft:brown_carpet",                             171, 12  },
	{ "minecraft:green_carpet",                             171, 13  },
	{ "minecraft:red_carpet",                               171, 14  },
	{ "minecraft:black_carpet",                             171, 15  },
	{ "minecraft:terracotta",                               172, 0   }, 
	{ "minecraft:coal_block",                               173, 0   }, 
	{ "minecraft:packed_ice",                               174, 0   }, 
	{ "minecraft:sunflower",                                175, 0   }, // tall flowers, two-block lower - data values may not be correct
	{ "minecraft:sunflower",                                175, 1   }, // tall flowers, two-block upper - data values may not be correct
	{ "minecraft:lilac",                                    175, 2   }, // tall flowers, two-block lower - data values may not be correct
	{ "minecraft:lilac",                                    175, 3   }, // tall flowers, two-block upper - data values may not be correct
	{ "minecraft:tall_grass",                               175, 4   }, // tall plants, two-block lower  - data values may not be correct
	{ "minecraft:tall_grass",                               175, 5   }, // tall plants, two-block upper  - data values may not be correct
	{ "minecraft:large_fern",                               175, 6   }, // tall plants, two-block lower  - data values may not be correct
	{ "minecraft:large_fern",                               175, 7   }, // tall plants, two-block upper  - data values may not be correct
	{ "minecraft:rose_bush",                                175, 8   }, // tall flowers, two-block lower - data values may not be correct
	{ "minecraft:rose_bush",                                175, 9   }, // tall flowers, two-block upper - data values may not be correct
	{ "minecraft:peony",                                    175, 10  }, // tall flowers, two-block lower - data values may not be correct
	{ "minecraft:peony",                                    175, 11  }, // tall flowers, two-block upper - data values may not be correct
	{ "minecraft:tall_seagrass",                            175, 12  }, // tall plants, two-block lower  - Added after flattening, data value not correct
	{ "minecraft:tall_seagrass",                            175, 13  }, // tall plants, two-block upper  - Added after flattening, data value not correct
    { "minecraft:white_banner",                             176, 0   },
    { "minecraft:orange_banner",                            176, 1   },
    { "minecraft:magenta_banner",                           176, 2   },
    { "minecraft:light_blue_banner",                        176, 3   },
    { "minecraft:yellow_banner",                            176, 4   },
    { "minecraft:lime_banner",                              176, 5   },
    { "minecraft:pink_banner",                              176, 6   },
    { "minecraft:gray_banner",                              176, 7   },
    { "minecraft:light_gray_banner",                        176, 8   },
    { "minecraft:cyan_banner",                              176, 9   },
    { "minecraft:purple_banner",                            176, 10  },
    { "minecraft:blue_banner",                              176, 11  },
    { "minecraft:brown_banner",                             176, 12  },
    { "minecraft:green_banner",                             176, 13  },
    { "minecraft:red_banner",                               176, 14  },
    { "minecraft:black_banner",                             176, 15  },
    { "minecraft:white_wall_banner",                        177, 0   },
    { "minecraft:orange_wall_banner",                       177, 1   },
    { "minecraft:magenta_wall_banner",                      177, 2   },
    { "minecraft:light_blue_wall_banner",                   177, 3   },
    { "minecraft:yellow_wall_banner",                       177, 4   },
    { "minecraft:lime_wall_banner",                         177, 5   },
    { "minecraft:pink_wall_banner",                         177, 6   },
    { "minecraft:gray_wall_banner",                         177, 7   },
    { "minecraft:light_gray_wall_banner",                   177, 8   },
    { "minecraft:cyan_wall_banner",                         177, 9   },
    { "minecraft:purple_wall_banner",                       177, 10  },
    { "minecraft:blue_wall_banner",                         177, 11  },
    { "minecraft:brown_wall_banner",                        177, 12  },
    { "minecraft:green_wall_banner",                        177, 13  },
    { "minecraft:red_wall_banner",                          177, 14  },
    { "minecraft:black_wall_banner",                        177, 15  },
	{ "minecraft:daylight_detector_inverted",               178, 0   },
	{ "minecraft:red_sandstone",                            179, 0   },
	{ "minecraft:cut_red_sandstone",                        179, 1   },
	{ "minecraft:chiseled_red_sandstone",                   179, 2   },
	{ "minecraft:smooth_red_sandstone",                     179, 3   },
	{ "minecraft:red_sandstone_stairs",                     180, 0   }, // facing east 
	{ "minecraft:red_sandstone_stairs",                     180, 1   }, // facing north
	{ "minecraft:red_sandstone_stairs",                     180, 2   }, // facing south
	{ "minecraft:red_sandstone_stairs",                     180, 3   }, // facing west
    { "minecraft:smooth_red_sandstone_stairs",              180, 4   }, // facing east  - added, data value not correct
	{ "minecraft:smooth_red_sandstone_stairs",              180, 5   }, // facing north - added, data value not correct
	{ "minecraft:smooth_red_sandstone_stairs",              180, 6   }, // facing south - added, data value not correct
	{ "minecraft:smooth_red_sandstone_stairs",              180, 7   }, // facing west  - added, data value not correct
	{ "minecraft:double_red_sandstone_slab",                181, 0   },
	{ "minecraft:double_red_sandstone_slab",                181, 1   }, // waterlogged
	{ "minecraft:double_cut_red_sandstone_slab",            181, 2   },
	{ "minecraft:double_cut_red_sandstone_slab",            181, 3   }, // waterlogged
	{ "minecraft:double_smooth_red_sandstone_slab",         181, 4   },
	{ "minecraft:double_smooth_red_sandstone_slab",         181, 5   }, // waterlogged
	{ "minecraft:red_sandstone_slab",                       182, 0   }, // bottom
	{ "minecraft:red_sandstone_slab",                       182, 1   }, // top
	{ "minecraft:red_sandstone_slab",                       182, 2   }, // bottom waterlogged
	{ "minecraft:red_sandstone_slab",                       182, 3   }, // top waterlogged
	{ "minecraft:cut_red_sandstone_slab",                   182, 4   }, // bottom
	{ "minecraft:cut_red_sandstone_slab",                   182, 5   }, // top
	{ "minecraft:cut_red_sandstone_slab",                   182, 6   }, // bottom waterlogged
	{ "minecraft:cut_red_sandstone_slab",                   182, 7   }, // top waterlogged
	{ "minecraft:smooth_red_sandstone_slab",                182, 8   }, // bottom
	{ "minecraft:smooth_red_sandstone_slab",                182, 9   }, // top
	{ "minecraft:smooth_red_sandstone_slab",                182, 10  }, // bottom waterlogged
	{ "minecraft:smooth_red_sandstone_slab",                182, 11  }, // top waterlogged
	{ "minecraft:spruce_fence_gate",                        183, 0   }, 
	{ "minecraft:birch_fence_gate",                         184, 0   }, 
	{ "minecraft:jungle_fence_gate",                        185, 0   }, 
	{ "minecraft:dark_oak_fence_gate",                      186, 0   }, 
	{ "minecraft:acacia_fence_gate",                        187, 0   }, 
	{ "minecraft:spruce_fence",                             188, 0   }, 
	{ "minecraft:birch_fence",                              189, 0   }, 
	{ "minecraft:jungle_fence",                             190, 0   }, 
	{ "minecraft:dark_oak_fence",                           191, 0   }, 
	{ "minecraft:acacia_fence",                             192, 0   },
    { "minecraft:spruce_door",                              193, 0   }, // facing east
    { "minecraft:spruce_door",                              193, 1   }, // facing north
    { "minecraft:spruce_door",                              193, 2   }, // facing south
    { "minecraft:spruce_door",                              193, 3   }, // facing west
	{ "minecraft:birch_door",                               194, 0   }, // facing east
	{ "minecraft:birch_door",                               194, 1   }, // facing north
	{ "minecraft:birch_door",                               194, 2   }, // facing south
	{ "minecraft:birch_door",                               194, 3   }, // facing west
	{ "minecraft:jungle_door",                              195, 0   }, // facing east
	{ "minecraft:jungle_door",                              195, 1   }, // facing north
	{ "minecraft:jungle_door",                              195, 2   }, // facing south
	{ "minecraft:jungle_door",                              195, 3   }, // facing west
	{ "minecraft:acacia_door",                              196, 0   }, // facing east
	{ "minecraft:acacia_door",                              196, 1   }, // facing north
	{ "minecraft:acacia_door",                              196, 2   }, // facing south
	{ "minecraft:acacia_door",                              196, 3   }, // facing west
    { "minecraft:dark_oak_door",                            197, 0   }, // facing east
	{ "minecraft:dark_oak_door",                            197, 1   }, // facing north
	{ "minecraft:dark_oak_door",                            197, 2   }, // facing south
	{ "minecraft:dark_oak_door",                            197, 3   }, // facing west
    { "minecraft:crimson_door",                             197, 4   }, // facing east	 - Added after flattening, data value not correct
	{ "minecraft:crimson_door",                             197, 5   }, // facing north	 - Added after flattening, data value not correct
	{ "minecraft:crimson_door",                             197, 6   }, // facing south	 - Added after flattening, data value not correct
	{ "minecraft:crimson_door",                             197, 7   }, // facing west	 - Added after flattening, data value not correct
    { "minecraft:warped_door",                              197, 8   }, // facing east	 - Added after flattening, data value not correct
	{ "minecraft:warped_door",                              197, 9   }, // facing north	 - Added after flattening, data value not correct
	{ "minecraft:warped_door",                              197, 10  }, // facing south  - Added after flattening, data value not correct
	{ "minecraft:warped_door",                              197, 11  }, // facing west	 - Added after flattening, data value not correct
	{ "minecraft:end_rod",                                  198, 0   }, // facing down
	{ "minecraft:end_rod",                                  198, 1   }, // facing east
	{ "minecraft:end_rod",                                  198, 2   }, // facing north
	{ "minecraft:end_rod",                                  198, 3   }, // facing south
	{ "minecraft:end_rod",                                  198, 4   }, // facing up
	{ "minecraft:end_rod",                                  198, 5   }, // facing west
	{ "minecraft:chorus_plant",                             199, 0   }, 
	{ "minecraft:chorus_flower",                            200, 0   }, 
	{ "minecraft:purpur_block",                             201, 0   }, 
	{ "minecraft:purpur_pillar",                            202, 0   }, // oriented east-west
	{ "minecraft:purpur_pillar",                            202, 1   }, // oriented vertically 
	{ "minecraft:purpur_pillar",                            202, 2   }, // oriented north-south
	{ "minecraft:purpur_stairs",                            203, 0   }, // facing east 
	{ "minecraft:purpur_stairs",                            203, 1   }, // facing north
	{ "minecraft:purpur_stairs",                            203, 2   }, // facing south
	{ "minecraft:purpur_stairs",                            203, 3   }, // facing west
	{ "minecraft:purpur_double_slab",                       204, 0   }, 
	{ "minecraft:purpur_double_slab",                       204, 1   }, // waterlogged
	{ "minecraft:purpur_slab",                              205, 0   }, // bottom
	{ "minecraft:purpur_slab",                              205, 1   }, // top
	{ "minecraft:purpur_slab",                              205, 2   }, // bottom waterlogged
	{ "minecraft:purpur_slab",                              205, 3   }, // top waterlogged
	{ "minecraft:end_stone_bricks",                         206, 0   }, 
	{ "minecraft:beetroots",                                207, 0   }, 
	{ "minecraft:dirt_path",                                208, 0   }, 
	{ "minecraft:end_gateway",                              209, 0   }, 
	{ "minecraft:repeating_command_block",                  210, 0   }, 
	{ "minecraft:chain_command_block",                      211, 0   }, 
	{ "minecraft:frosted_ice",                              212, 0   }, 
	{ "minecraft:magma_block",                              213, 0   }, 
	{ "minecraft:nether_wart_block",                        214, 0   }, 
	{ "minecraft:warped_wart_block",                        214, 1   }, 
	{ "minecraft:red_nether_bricks",                        215, 0   },
	{ "minecraft:cracked_nether_bricks",                    215, 1   },
	{ "minecraft:chiseled_nether_bricks",                   215, 2   },
	{ "minecraft:bone_block",                               216, 0   }, // oriented east-west
	{ "minecraft:bone_block",                               216, 1   }, // oriented vertically 
	{ "minecraft:bone_block",                               216, 2   }, // oriented north-south
	{ "minecraft:structure_void",                           217, 0   },
	{ "minecraft:observer",                                 218, 0   }, // unpowered - facing down 
	{ "minecraft:observer",                                 218, 1   }, // unpowered - facing east 
	{ "minecraft:observer",                                 218, 2   }, // unpowered - facing north 
	{ "minecraft:observer",                                 218, 3   }, // unpowered - facing south 
	{ "minecraft:observer",                                 218, 4   }, // unpowered - facing up 
	{ "minecraft:observer",                                 218, 5   }, // unpowered - facing west 
	{ "minecraft:observer",                                 218, 6   }, // powered   - facing down 
	{ "minecraft:observer",                                 218, 7   }, // powered   - facing east 
	{ "minecraft:observer",                                 218, 8   }, // powered   - facing north 
	{ "minecraft:observer",                                 218, 9   }, // powered   - facing south 
	{ "minecraft:observer",                                 218, 10  }, // powered   - facing up  
	{ "minecraft:observer",                                 218, 11  }, // powered   - facing west  
	{ "minecraft:white_shulker_box",                        219, 0   }, 
	{ "minecraft:orange_shulker_box",                       220, 0   }, 
	{ "minecraft:magenta_shulker_box",                      221, 0   }, 
	{ "minecraft:light_blue_shulker_box",                   222, 0   }, 
	{ "minecraft:yellow_shulker_box",                       223, 0   }, 
	{ "minecraft:lime_shulker_box",                         224, 0   }, 
	{ "minecraft:pink_shulker_box",                         225, 0   }, 
	{ "minecraft:gray_shulker_box",                         226, 0   }, 
	{ "minecraft:light_gray_shulker_box",                   227, 0   }, 
	{ "minecraft:cyan_shulker_box",                         228, 0   }, 
	{ "minecraft:purple_shulker_box",                       229, 0   }, 
	{ "minecraft:blue_shulker_box",                         230, 0   }, 
	{ "minecraft:brown_shulker_box",                        231, 0   }, 
	{ "minecraft:green_shulker_box",                        232, 0   }, 
	{ "minecraft:red_shulker_box",                          233, 0   }, 
	{ "minecraft:black_shulker_box",                        234, 0   },
	{ "minecraft:white_glazed_terracotta",                  235, 0   }, // facing east  
	{ "minecraft:white_glazed_terracotta",                  235, 1   }, // facing north 
	{ "minecraft:white_glazed_terracotta",                  235, 2   }, // facing south 
	{ "minecraft:white_glazed_terracotta",                  235, 3   }, // facing west  
	{ "minecraft:orange_glazed_terracotta",                 236, 0   }, // facing east  
	{ "minecraft:orange_glazed_terracotta",                 236, 1   }, // facing north 
	{ "minecraft:orange_glazed_terracotta",                 236, 2   }, // facing south 
	{ "minecraft:orange_glazed_terracotta",                 236, 3   }, // facing west  
	{ "minecraft:magenta_glazed_terracotta",                237, 0   }, // facing east  
	{ "minecraft:magenta_glazed_terracotta",                237, 1   }, // facing north 
	{ "minecraft:magenta_glazed_terracotta",                237, 2   }, // facing south 
	{ "minecraft:magenta_glazed_terracotta",                237, 3   }, // facing west  
	{ "minecraft:light_blue_glazed_terracotta",             238, 0   }, // facing east  
	{ "minecraft:light_blue_glazed_terracotta",             238, 1   }, // facing north 
	{ "minecraft:light_blue_glazed_terracotta",             238, 2   }, // facing south 
	{ "minecraft:light_blue_glazed_terracotta",             238, 3   }, // facing west  
	{ "minecraft:yellow_glazed_terracotta",                 239, 0   }, // facing east 
	{ "minecraft:yellow_glazed_terracotta",                 239, 1   }, // facing north
	{ "minecraft:yellow_glazed_terracotta",                 239, 2   }, // facing south
	{ "minecraft:yellow_glazed_terracotta",                 239, 3   }, // facing west 
	{ "minecraft:lime_glazed_terracotta",                   240, 0   }, // facing east 
	{ "minecraft:lime_glazed_terracotta",                   240, 1   }, // facing north
	{ "minecraft:lime_glazed_terracotta",                   240, 2   }, // facing south
	{ "minecraft:lime_glazed_terracotta",                   240, 3   }, // facing west 
	{ "minecraft:pink_glazed_terracotta",                   241, 0   }, // facing east 
	{ "minecraft:pink_glazed_terracotta",                   241, 1   }, // facing north
	{ "minecraft:pink_glazed_terracotta",                   241, 2   }, // facing south
	{ "minecraft:pink_glazed_terracotta",                   241, 3   }, // facing west 
	{ "minecraft:gray_glazed_terracotta",                   242, 0   }, // facing east 
	{ "minecraft:gray_glazed_terracotta",                   242, 1   }, // facing north
	{ "minecraft:gray_glazed_terracotta",                   242, 2   }, // facing south
	{ "minecraft:gray_glazed_terracotta",                   242, 3   }, // facing west 
	{ "minecraft:light_gray_glazed_terracotta",             243, 0   }, // facing east 
	{ "minecraft:light_gray_glazed_terracotta",             243, 1   }, // facing north
	{ "minecraft:light_gray_glazed_terracotta",             243, 2   }, // facing south
	{ "minecraft:light_gray_glazed_terracotta",             243, 3   }, // facing west 
	{ "minecraft:cyan_glazed_terracotta",                   244, 0   }, // facing east 
	{ "minecraft:cyan_glazed_terracotta",                   244, 1   }, // facing north
	{ "minecraft:cyan_glazed_terracotta",                   244, 2   }, // facing south
	{ "minecraft:cyan_glazed_terracotta",                   244, 3   }, // facing west 
	{ "minecraft:purple_glazed_terracotta",                 245, 0   }, // facing east 
	{ "minecraft:purple_glazed_terracotta",                 245, 1   }, // facing north
	{ "minecraft:purple_glazed_terracotta",                 245, 2   }, // facing south
	{ "minecraft:purple_glazed_terracotta",                 245, 3   }, // facing west 
	{ "minecraft:blue_glazed_terracotta",                   246, 0   }, // facing east 
	{ "minecraft:blue_glazed_terracotta",                   246, 1   }, // facing north
	{ "minecraft:blue_glazed_terracotta",                   246, 2   }, // facing south
	{ "minecraft:blue_glazed_terracotta",                   246, 3   }, // facing west 
	{ "minecraft:brown_glazed_terracotta",                  247, 0   }, // facing east 
	{ "minecraft:brown_glazed_terracotta",                  247, 1   }, // facing north
	{ "minecraft:brown_glazed_terracotta",                  247, 2   }, // facing south
	{ "minecraft:brown_glazed_terracotta",                  247, 3   }, // facing west 
	{ "minecraft:green_glazed_terracotta",                  248, 0   }, // facing east 
	{ "minecraft:green_glazed_terracotta",                  248, 1   }, // facing north
	{ "minecraft:green_glazed_terracotta",                  248, 2   }, // facing south
	{ "minecraft:green_glazed_terracotta",                  248, 3   }, // facing west 
	{ "minecraft:red_glazed_terracotta",                    249, 0   }, // facing east 
	{ "minecraft:red_glazed_terracotta",                    249, 1   }, // facing north
	{ "minecraft:red_glazed_terracotta",                    249, 2   }, // facing south
	{ "minecraft:red_glazed_terracotta",                    249, 3   }, // facing west 
	{ "minecraft:black_glazed_terracotta",                  250, 0   }, // facing east 
	{ "minecraft:black_glazed_terracotta",                  250, 1   }, // facing north
	{ "minecraft:black_glazed_terracotta",                  250, 2   }, // facing south
	{ "minecraft:black_glazed_terracotta",                  250, 3   }, // facing west 
	{ "minecraft:white_concrete",                           251, 0   }, 
	{ "minecraft:orange_concrete",                          251, 1   }, 
	{ "minecraft:magenta_concrete",                         251, 2   }, 
	{ "minecraft:light_blue_concrete",                      251, 3   }, 
	{ "minecraft:yellow_concrete",                          251, 4   }, 
	{ "minecraft:lime_concrete",                            251, 5   }, 
	{ "minecraft:pink_concrete",                            251, 6   }, 
	{ "minecraft:gray_concrete",                            251, 7   }, 
	{ "minecraft:light_gray_concrete",                      251, 8   }, 
	{ "minecraft:cyan_concrete",                            251, 9   }, 
	{ "minecraft:purple_concrete",                          251, 10  }, 
	{ "minecraft:blue_concrete",                            251, 11  }, 
	{ "minecraft:brown_concrete",                           251, 12  }, 
	{ "minecraft:green_concrete",                           251, 13  }, 
	{ "minecraft:red_concrete",                             251, 14  }, 
	{ "minecraft:black_concrete",                           251, 15  },
	{ "minecraft:white_concrete_powder",                    252, 0   }, 
	{ "minecraft:orange_concrete_powder",                   252, 1   }, 
	{ "minecraft:magenta_concrete_powder",                  252, 2   }, 
	{ "minecraft:light_blue_concrete_powder",               252, 3   }, 
	{ "minecraft:yellow_concrete_powder",                   252, 4   }, 
	{ "minecraft:lime_concrete_powder",                     252, 5   }, 
	{ "minecraft:pink_concrete_powder",                     252, 6   }, 
	{ "minecraft:gray_concrete_powder",                     252, 7   }, 
	{ "minecraft:light_gray_concrete_powder",               252, 8   }, 
	{ "minecraft:cyan_concrete_powder",                     252, 9   }, 
	{ "minecraft:purple_concrete_powder",                   252, 10  }, 
	{ "minecraft:blue_concrete_powder",                     252, 11  }, 
	{ "minecraft:brown_concrete_powder",                    252, 12  }, 
	{ "minecraft:green_concrete_powder",                    252, 13  }, 
	{ "minecraft:red_concrete_powder",                      252, 14  }, 
	{ "minecraft:black_concrete_powder",                    252, 15  }, 
	{ "minecraft:unused_253",                               253, 0   }, // pre-flattening unused blockid, added here to ensure have all old id's covered
	{ "minecraft:unused_254",                               254, 0   }, // pre-flattening unused blockid, added here to ensure have all old id's covered
	{ "minecraft:structure_block",                          255, 0   }
};

static const uint32_t numDefaultNamespaceAndBlockIDs = (uint32_t)sizeof(defaultNamespaceAndBlockIDs)/sizeof(enkiMINamespaceAndBlockID);

typedef struct SectionChunkInfo_s
{
	uint8_t offset_0;
	uint8_t offset_1;
	uint8_t offset_2;
	uint8_t sectorCount;
} SectionChunkInfo;

static int32_t GetChunkLocation( SectionChunkInfo section_ )
{
	return ( ( ( section_.offset_0 << 16 ) + ( section_.offset_1 << 8 ) + section_.offset_2 ) * SECTOR_SIZE );
}

typedef struct BigEndian4BytesTo32BitInt_s
{
	uint8_t pos[4];
} BigEndian4BytesTo32BitInt;


static int32_t Get32BitInt( BigEndian4BytesTo32BitInt in_ )
{
	return ( ( in_.pos[0] << 24 ) + ( in_.pos[1] << 16 ) + ( in_.pos[2] << 8 ) + in_.pos[3] );
}

typedef struct RegionHeader_s
{
	SectionChunkInfo          sectionChunksInfos[ ENKI_MI_REGION_CHUNKS_NUMBER ]; // chunks locations
	BigEndian4BytesTo32BitInt sectionChunksTimestamps[ ENKI_MI_REGION_CHUNKS_NUMBER ]; // chunks timestamps
} RegionHeader;

const char * enkiGetNBTTagIDAsString( uint8_t tagID_ )
{
	return tagIdString[ tagID_ ];
}

const char * enkiGetNBTTagHeaderIDAsString( enkiNBTTagHeader tagheader_ )
{
	return tagIdString[ tagheader_.tagId ];
}

void enkiNBTInitFromMemoryUncompressed( enkiNBTDataStream* pStream_, uint8_t * pData_, uint32_t dataSize_)
{
	memset( &pStream_->currentTag, 0, sizeof( enkiNBTTagHeader ) );
	pStream_->pData = pData_;
	pStream_->dataLength = dataSize_;
	pStream_->pCurrPos = pStream_->pData;
	pStream_->pDataEnd = pStream_->pData + pStream_->dataLength;
	pStream_->pNextTag = pStream_->pCurrPos;
	pStream_->level = -1;
	pStream_->pAllocations = NULL;
}

int enkiNBTInitFromMemoryCompressed( enkiNBTDataStream* pStream_, uint8_t * pCompressedData_,
										uint32_t compressedDataSize_, uint32_t uncompressedSizeHint_)
{
	// check if gzip style first:  https://tools.ietf.org/html/rfc1952#section-2.2
	static const uint32_t GZIP_HEADER_SIZE = 10;
	if(    compressedDataSize_ > GZIP_HEADER_SIZE
		&& pCompressedData_[0] == 0x1f
		&& pCompressedData_[1] == 0x8b )
	{
		// gzip style
		if( pCompressedData_[3] != 0)
		{
			// flags add extra information, normally not added by minecraft and we don't handle them
			enkiNBTInitFromMemoryUncompressed( pStream_, NULL, 0 );
			return 0;
		}

		int32_t ISIZE = *(int32_t*)(pCompressedData_ + compressedDataSize_ - 4);
		assert(ISIZE > 0);
		uint8_t* gzUncompressedData = (uint8_t*)malloc( (size_t)ISIZE );

		// uncompress gzip
		mz_stream stream;
		int status;
		memset(&stream, 0, sizeof(stream));
		uint32_t gzCompressedDataSize = compressedDataSize_ - GZIP_HEADER_SIZE;
		uint8_t* gzCompressedData     = pCompressedData_    + GZIP_HEADER_SIZE;

		stream.next_in   = gzCompressedData;
		stream.avail_in  = (mz_uint32)gzCompressedDataSize;
		stream.next_out  = gzUncompressedData;
		stream.avail_out = (mz_uint32)ISIZE;

		status = mz_inflateInit2(&stream, -MZ_DEFAULT_WINDOW_BITS);
		if (status == MZ_OK)
		{
			status = mz_inflate(&stream, MZ_FINISH);
			if (status != MZ_STREAM_END)
			{
				mz_inflateEnd(&stream);
				status = ((status == MZ_BUF_ERROR) && (!stream.avail_in)) ? MZ_DATA_ERROR : status;
			}
			else
			{
				status = mz_inflateEnd(&stream);
			}

			if( status == MZ_OK )
			{
				enkiNBTInitFromMemoryUncompressed( pStream_, gzUncompressedData, ( uint32_t )stream.total_out );
				enkiNBTAddAllocation( pStream_, gzUncompressedData );
				return 1;
			}
			else
			{
				free( gzUncompressedData );
			}
		}
	}


	mz_ulong destLength = uncompressedSizeHint_;
	if( destLength <= compressedDataSize_ )
	{
		destLength = compressedDataSize_ * 4 + 1024; // estimate uncompressed size
	}
	mz_ulong startDestLength = destLength;
	uint8_t* dataUnCompressed = (uint8_t*)malloc( destLength );
	int retval = uncompress( dataUnCompressed, &destLength, pCompressedData_, compressedDataSize_ );
	if( retval == MZ_BUF_ERROR && startDestLength == destLength )
	{
		// failed to uncompress, buffer full
		for( int attempts = 0; ( retval != MZ_OK ) && ( attempts < 3 ); ++attempts )
		{
			free( dataUnCompressed );
			destLength *= 4 + 1024;
			dataUnCompressed = (uint8_t*)malloc( destLength );
			retval = uncompress( dataUnCompressed, &destLength, pCompressedData_, compressedDataSize_ );
		}
	}
	if( retval != MZ_OK )
	{
		enkiNBTInitFromMemoryUncompressed( pStream_, NULL, 0 );
		free( dataUnCompressed );
		return 0;
	}

	dataUnCompressed = (uint8_t*)realloc( dataUnCompressed, destLength ); // reallocate to actual size
	enkiNBTInitFromMemoryUncompressed( pStream_, dataUnCompressed, ( uint32_t )destLength );
	enkiNBTAddAllocation( pStream_, dataUnCompressed );
	return 1;
}

void enkiNBTAddAllocation( enkiNBTDataStream* pStream_, void* pAllocation_ )
{
	enkiNBTAllocation* pAlloc = (enkiNBTAllocation*)malloc(sizeof(enkiNBTAllocation));
	pAlloc->pAllocation = pAllocation_;
	pAlloc->pNext = pStream_->pAllocations;
	pStream_->pAllocations = pAlloc;
}

void enkiNBTFreeAllocations( enkiNBTDataStream* pStream_ )
{
	enkiNBTAllocation* pNext = pStream_->pAllocations;
	while( pNext )
	{
		enkiNBTAllocation* pCurr = pNext;
		free( pCurr->pAllocation );
		pNext = pCurr->pNext;
		free( pCurr );
	}
	memset( pStream_, 0, sizeof(enkiNBTDataStream) );
}

int8_t  enkiNBTReadInt8( enkiNBTDataStream* pStream_ )
{
	int8_t retVal = pStream_->pCurrPos[ 0 ];
	pStream_->pCurrPos += 1;
	return retVal;
}

int8_t  enkiNBTReadByte( enkiNBTDataStream* pStream_ )
{
	return enkiNBTReadInt8( pStream_ );
}

int16_t enkiNBTReadInt16( enkiNBTDataStream* pStream_ )
{
	int16_t retVal = ( pStream_->pCurrPos[ 0 ] << 8 ) + pStream_->pCurrPos[ 1 ];
	pStream_->pCurrPos += 2;
	return retVal;
}

int16_t enkiNBTReadShort( enkiNBTDataStream* pStream_ )
{
	return enkiNBTReadInt16( pStream_ );
}

int32_t enkiNBTReadInt32( enkiNBTDataStream* pStream_ )
{
	int32_t retVal = ( pStream_->pCurrPos[ 0 ] << 24 ) + ( pStream_->pCurrPos[ 1 ] << 16 ) + ( pStream_->pCurrPos[ 2 ] << 8 ) + pStream_->pCurrPos[ 3 ];
	pStream_->pCurrPos += 4;
	return retVal;
}

int32_t enkiNBTReadInt( enkiNBTDataStream* pStream_ )
{
	return enkiNBTReadInt32( pStream_ );
}

float   enkiNBTReadFloat( enkiNBTDataStream* pStream_ )
{
	int32_t iVal = ( pStream_->pCurrPos[ 0 ] << 24 ) + ( pStream_->pCurrPos[ 1 ] << 16 ) + ( pStream_->pCurrPos[ 2 ] << 8 ) + pStream_->pCurrPos[ 3 ];
	float retVal = *( float* )&iVal;
	pStream_->pCurrPos += 4;
	return retVal;
}

int64_t enkiNBTReadInt64( enkiNBTDataStream* pStream_ )
{
	int64_t retVal = ( ( int64_t )pStream_->pCurrPos[ 0 ] << 54 ) + ( ( int64_t )pStream_->pCurrPos[ 1 ] << 48 ) + ( ( int64_t )pStream_->pCurrPos[ 2 ] << 40 ) + ( ( int64_t )pStream_->pCurrPos[ 5 ] << 32 ) + 
		             ( pStream_->pCurrPos[ 4 ] << 24 ) + ( pStream_->pCurrPos[ 5 ] << 16 ) + ( pStream_->pCurrPos[ 6 ] << 8 ) + pStream_->pCurrPos[ 7 ];
	pStream_->pCurrPos += 8;
	return retVal;
}

int64_t enkiNBTReadlong( enkiNBTDataStream* pStream_ )
{
	return enkiNBTReadInt64( pStream_ );
}

double  enkiNBTReadDouble( enkiNBTDataStream* pStream_ )
{
	int64_t iVal = ( ( int64_t )pStream_->pCurrPos[ 0 ] << 54 ) + ( ( int64_t )pStream_->pCurrPos[ 1 ] << 48 ) + ( ( int64_t )pStream_->pCurrPos[ 2 ] << 40 ) + ( ( int64_t )pStream_->pCurrPos[ 5 ] << 32 ) + 
		           ( pStream_->pCurrPos[ 4 ] << 24 ) + ( pStream_->pCurrPos[ 5 ] << 16 ) + ( pStream_->pCurrPos[ 6 ] << 8 ) + pStream_->pCurrPos[ 7 ];
	double retVal = *( double* )&iVal;
	pStream_->pCurrPos += 8;
	return retVal;
}

// Internal only uint16_t type
uint16_t enkiNBTReadUint16( enkiNBTDataStream* pStream_ )
{
	uint16_t retVal = ( pStream_->pCurrPos[ 0 ] << 8 ) + pStream_->pCurrPos[ 1 ];
	pStream_->pCurrPos += 2;
	return retVal;
}

enkiNBTString enkiNBTReadString( enkiNBTDataStream* pStream_ )
{
	enkiNBTString nbtString;
	nbtString.size = enkiNBTReadUint16( pStream_ );
	nbtString.pStrNotNullTerminated = (const char*)pStream_->pCurrPos;
	return nbtString;
}

static void SkipDataToNextTag( enkiNBTDataStream* pStream_ )
{
	uint8_t* pCurrPos = pStream_->pCurrPos;
	switch( pStream_->currentTag.tagId )
	{
	case enkiNBTTAG_End:
		// no data, so do nothing.
		break;
	case enkiNBTTAG_Byte:
		pStream_->pNextTag += 1; // 1 byte
		break;
	case enkiNBTTAG_Short:
		pStream_->pNextTag += 2; // 2 bytes
		break;
	case enkiNBTTAG_Int:
		pStream_->pNextTag += 4;
		break;
	case enkiNBTTAG_Long:
		pStream_->pNextTag += 8;
		break;
	case enkiNBTTAG_Float:
		pStream_->pNextTag += 4;
		break;
	case enkiNBTTAG_Double:
		pStream_->pNextTag += 8;
		break;
	case enkiNBTTAG_Byte_Array:
	{
		int32_t length = enkiNBTReadInt32( pStream_ );
		pStream_->pNextTag = pStream_->pCurrPos + length * 1; // array of bytes
		break;
	}
	case enkiNBTTAG_String:
	{
		int32_t length = enkiNBTReadUint16( pStream_ );
		pStream_->pNextTag = pStream_->pCurrPos + length;
		break;
	}
	case enkiNBTTAG_List:
		// read as a compound type
		break;
	case enkiNBTTAG_Compound:
		// data is in standard format, so do nothing.
		break;
	case enkiNBTTAG_Int_Array:
	{
		int32_t length = enkiNBTReadInt32( pStream_ );
		pStream_->pNextTag = pStream_->pCurrPos + length * 4; // array of ints (4 bytes)
		break;
	}
	case enkiNBTTAG_Long_Array:
	{
		int32_t length = enkiNBTReadInt32( pStream_ );
		pStream_->pNextTag = pStream_->pCurrPos + length * 8; // array of longs (4 bytes)
		break;
	}
	default:
		assert( 0 );
		break;
	}
	pStream_->pCurrPos = pCurrPos; // restore current position
}

int enkiNBTReadNextTag( enkiNBTDataStream* pStream_ )
{
	if( ( enkiNBTTAG_Compound == pStream_->currentTag.tagId ) || ( enkiNBTTAG_List == pStream_->currentTag.tagId ) )
	{
		pStream_->level++;
		if( pStream_->level == 512 )
		{
			assert(0); // in debug break.
			return 0; // invalid nested tags
		}
		pStream_->parentTags[ pStream_->level ] = pStream_->currentTag;
	}
	while( ( pStream_->level >= 0 ) && ( enkiNBTTAG_List == pStream_->parentTags[ pStream_->level ].tagId ) )
	{
		if( pStream_->parentTags[ pStream_->level ].listCurrItem + 1 == pStream_->parentTags[ pStream_->level ].listNumItems )
		{
			pStream_->level--;
		}
		else
		{
			pStream_->currentTag.tagId = pStream_->parentTags[ pStream_->level ].listItemTagId;
			pStream_->currentTag.pName = NULL;
			pStream_->pCurrPos = pStream_->pNextTag; // init current position with nexttag
			if( enkiNBTTAG_List == pStream_->currentTag.tagId )
			{
				pStream_->currentTag.listItemTagId = *(pStream_->pCurrPos++);
				pStream_->currentTag.listNumItems = enkiNBTReadInt32( pStream_ );
				pStream_->currentTag.listCurrItem = -1;
				pStream_->pNextTag = pStream_->pCurrPos;
			}
			SkipDataToNextTag( pStream_ );
			pStream_->parentTags[ pStream_->level ].listCurrItem++;
			return 1;
		}
	}
	if( pStream_->pNextTag >= pStream_->pDataEnd )
	{
		return 0;
	}
	pStream_->pCurrPos = pStream_->pNextTag;

	// Get Tag Header
	pStream_->currentTag.pName = NULL;
	assert( *(pStream_->pCurrPos) < enkiNBTTAG_SIZE );

	pStream_->currentTag.tagId = *(pStream_->pCurrPos++);
	if( enkiNBTTAG_End != pStream_->currentTag.tagId )
	{
		// We null terminate tag names by replacing 2 byte size with 1 byte 0xFF, moving and adding 0 at end
		// This assumes no tag name is ever > in16_t max as name sizes are actually uint16_t
		if( 0xff == *(pStream_->pCurrPos) )
		{
			pStream_->pCurrPos++;
			pStream_->currentTag.pName = ( char* )pStream_->pCurrPos;
			while( *(pStream_->pCurrPos++) != 0 );
		}
		else
		{
			int32_t lengthOfName = enkiNBTReadInt16( pStream_ );
			if( lengthOfName )
			{
				assert( pStream_->pCurrPos+lengthOfName < pStream_->pDataEnd );
				// move and null terminate, flag as 
				*( pStream_->pCurrPos - 2 ) = 0xff; // this value will not be seen as a length since it will be negative
				pStream_->currentTag.pName = ( char* )( pStream_->pCurrPos - 1 );
				memmove( pStream_->currentTag.pName, pStream_->pCurrPos, lengthOfName );
				pStream_->pCurrPos += lengthOfName - 1;
				pStream_->pCurrPos[ 0 ] = 0; // null terminator
				pStream_->pCurrPos += 1;
			}
		}
	}
	if( enkiNBTTAG_List == pStream_->currentTag.tagId )
	{
		pStream_->currentTag.listItemTagId = *(pStream_->pCurrPos++);
		pStream_->currentTag.listNumItems = enkiNBTReadInt32( pStream_ );
		pStream_->currentTag.listCurrItem = -1;
	}
	pStream_->pNextTag = pStream_->pCurrPos;

	SkipDataToNextTag( pStream_ );

	if( ( pStream_->level >= 0 ) && ( enkiNBTTAG_End == pStream_->currentTag.tagId ) )
	{
		pStream_->level--;
	}

	return 1;
}


void enkiNBTRewind( enkiNBTDataStream* pStream_ )
{
	memset( &(pStream_->currentTag), 0, sizeof( enkiNBTTagHeader ) );
	pStream_->pCurrPos = pStream_->pData;
	pStream_->level = -1;
	pStream_->pNextTag = pStream_->pData;
}

void enkiRegionFileInit( enkiRegionFile* pRegionFile_ )
{
	memset( pRegionFile_, 0, sizeof( enkiRegionFile ) );
}


enkiRegionFile enkiRegionFileLoad( FILE * fp_ )
{
	enkiRegionFile regionFile;
	enkiRegionFileInit( &regionFile );
	fseek( fp_, 0, SEEK_END );
	regionFile.regionDataSize = ftell( fp_ );
	fseek( fp_, 0, SEEK_SET ); // return to start position

	if( regionFile.regionDataSize )
	{
		// get the data in the chunks data section
		regionFile.pRegionData = (uint8_t*)malloc( regionFile.regionDataSize );
		fread( regionFile.pRegionData, 1, regionFile.regionDataSize, fp_ ); // note: because sectionDataChunks is an array of single bytes, sizeof( sectionDataChunks ) == sectionDataSize
	}

	return regionFile;
}


uint8_t enkiHasChunk( enkiRegionFile regionFile_, int32_t chunkNr_ )
{
	uint8_t hasChunk = 0;
	RegionHeader* header = (RegionHeader*)regionFile_.pRegionData;
	uint32_t locationOffset = GetChunkLocation( header->sectionChunksInfos[ chunkNr_ ] );
	if( locationOffset >= sizeof( RegionHeader ) && ( locationOffset + 6 ) <= regionFile_.regionDataSize )
	{
		uint32_t length = Get32BitInt(  *( BigEndian4BytesTo32BitInt* )&regionFile_.pRegionData[ locationOffset ] );
		if( ( length + locationOffset + 4 ) <= regionFile_.regionDataSize )
		{
			hasChunk = 1;
		}
	}
	return hasChunk;
}

void enkiInitNBTDataStreamForChunk( enkiRegionFile regionFile_, int32_t chunkNr_, enkiNBTDataStream* pStream_ )
{
	enkiNBTInitFromMemoryUncompressed( pStream_, NULL, 0 ); // clears stream

	RegionHeader* header = (RegionHeader*)regionFile_.pRegionData;
	uint32_t locationOffset = GetChunkLocation( header->sectionChunksInfos[ chunkNr_ ] );
	if( locationOffset >= sizeof( RegionHeader ) && ( locationOffset + 6 ) <= regionFile_.regionDataSize )
	{
		uint32_t length = Get32BitInt(  *( BigEndian4BytesTo32BitInt* )&regionFile_.pRegionData[ locationOffset ] );
		uint8_t compression_type = regionFile_.pRegionData[ locationOffset + 4 ]; // we ignore this as unused for now
		assert(compression_type == 2 );
		(void)compression_type;
		--length; // length includes compression_type
		// get the data and decompress it
		if( ( length + locationOffset + 5 ) <= regionFile_.regionDataSize )
		{
			uint8_t* dataCompressed = &regionFile_.pRegionData[ locationOffset + 5 ];
			enkiNBTInitFromMemoryCompressed( pStream_, dataCompressed, length, 0 );
		}
	}
}

int32_t enkiGetTimestampForChunk( enkiRegionFile regionFile_, int32_t chunkNr_ )
{
	RegionHeader* header = (RegionHeader*)regionFile_.pRegionData;
	return Get32BitInt( header->sectionChunksTimestamps[ chunkNr_ ] );
}

void enkiRegionFileFreeAllocations(enkiRegionFile * pRegionFile_)
{
	free( pRegionFile_->pRegionData );
	memset( pRegionFile_, 0, sizeof(enkiRegionFile) );
}

int enkiAreStringsEqual( const char * lhs_, const char * rhs_ )
{
	if( lhs_ && rhs_ )
	{
		if( 0 == strcmp( lhs_, rhs_ ) )
		{
			return 1;
		}
	}
	return 0;
}

void enkiChunkInit( enkiChunkBlockData* pChunk_ )
{
	memset( pChunk_, 0, sizeof( enkiChunkBlockData ) );
}

static void LoadChunkPalette( enkiNBTDataStream* pStream_, enkiChunkSectionPalette* pSectionPalette_, enkiNBTReadChunkExParams params_ )
{
	if( 0 == pStream_->currentTag.listNumItems )
	{
		return;
	}
	pSectionPalette_->size = (uint32_t)pStream_->currentTag.listNumItems;
	float numBitsFloat = floorf( 1.0f + log2f( fmaxf( (float)( pSectionPalette_->size - 1 ), 15.0f) ) ); // 15.0f == 0x1111 so takes 4bits. log2f(15.0f) == 3.9f, add one and take floor gives numbits
	uint32_t numBits = (uint32_t)numBitsFloat;
	pSectionPalette_->numBitsPerBlock = numBits;

	pSectionPalette_->pDefaultBlockIndex = (int32_t*)malloc(sizeof(int32_t)*pSectionPalette_->size);
	enkiNBTAddAllocation( pStream_, pSectionPalette_->pDefaultBlockIndex );
	pSectionPalette_->pNamespaceIDStrings = (enkiNBTString*)malloc(sizeof(enkiNBTString)*pSectionPalette_->size);
	enkiNBTAddAllocation( pStream_, pSectionPalette_->pNamespaceIDStrings );
	pSectionPalette_->pBlockStateProperties = (enkiMIProperties*)malloc(sizeof(enkiMIProperties)*pSectionPalette_->size);
	memset( pSectionPalette_->pBlockStateProperties, 0, sizeof(enkiMIProperties)*pSectionPalette_->size );
	enkiNBTAddAllocation( pStream_, pSectionPalette_->pBlockStateProperties );

	// read palettes
	int levelPalette = pStream_->level;
	int32_t paletteNum = 0;
	while(     enkiNBTReadNextTag( pStream_ )
			&& levelPalette < pStream_->level )
	{
		// This is a list of compound tags, ends with enkiNBTTAG_End at levelPalette+1 
		if( pStream_->currentTag.tagId == enkiNBTTAG_End && pStream_->level == levelPalette + 1 
			&& pStream_->parentTags[ pStream_->level ].listCurrItem + 1 >=  pStream_->parentTags[ pStream_->level ].listNumItems )
		{
			break;
		}

		paletteNum = pStream_->parentTags[ levelPalette + 1 ].listCurrItem;
		assert( paletteNum >= 0 );
		assert( paletteNum < (int32_t)pSectionPalette_->size );
		if(   pStream_->currentTag.tagId == enkiNBTTAG_String
			&& enkiAreStringsEqual( "Name", pStream_->currentTag.pName ) )
		{
			enkiNBTString paletteEntry = enkiNBTReadString( pStream_ );
			// find in palette
			// enkiMIBlockID defaultBlockIDs[]
			pSectionPalette_->pDefaultBlockIndex[ paletteNum ] = -1;
			pSectionPalette_->pNamespaceIDStrings[ paletteNum ] = paletteEntry;
			if( !( params_.flags & enkiNBTReadChunkExFlags_NoPaletteTranslation ) )
			{
				for( uint32_t id=0; id <numDefaultNamespaceAndBlockIDs; ++id )
				{
					size_t len = strlen( defaultNamespaceAndBlockIDs[id].pNamespaceID );
					if(    len == paletteEntry.size
						&& 0 == memcmp( defaultNamespaceAndBlockIDs[id].pNamespaceID, paletteEntry.pStrNotNullTerminated, len ) )
					{
						pSectionPalette_->pDefaultBlockIndex[ paletteNum ] = id;
						break;
					}
				}
			}
		}
		if(  enkiNBTTAG_Compound == pStream_->currentTag.tagId 
			&& enkiAreStringsEqual( "Properties", pStream_->currentTag.pName ) )
		{
			int levelProperties = pStream_->level;
			uint32_t numProperties = 0;
			 // Compound tag, ends with enkiNBTTAG_End at levelProperties
			while( enkiNBTReadNextTag( pStream_ )
					&& levelProperties < pStream_->level )
			{
				if( pStream_->currentTag.tagId == enkiNBTTAG_String )
				{
					if( numProperties < ENKI_MI_MAX_PROPERTIES )
					{
						pSectionPalette_->pBlockStateProperties[ paletteNum ].properties[ numProperties ].pName = pStream_->currentTag.pName;
						pSectionPalette_->pBlockStateProperties[ paletteNum ].properties[ numProperties ].value = enkiNBTReadString( pStream_ );
						++pSectionPalette_->pBlockStateProperties[ paletteNum ].size;
					}
					++numProperties;
				}
			}
		}
	}
}

enkiNBTReadChunkExParams enkiGetDefaultNBTReadChunkExParams()
{
	enkiNBTReadChunkExParams params;
	params.flags = enkiNBTReadChunkExFlags_None;
	return params;
}

enkiChunkBlockData enkiNBTReadChunk( enkiNBTDataStream * pStream_ )
{
	return enkiNBTReadChunkEx( pStream_, enkiGetDefaultNBTReadChunkExParams() );
}

// see https://minecraft.fandom.com/wiki/Chunk_format
enkiChunkBlockData enkiNBTReadChunkEx( enkiNBTDataStream * pStream_, enkiNBTReadChunkExParams params_ )
{
	enkiChunkBlockData chunk;
	enkiChunkInit( &chunk );
	int foundSectionData = 0;
	int foundXPos = 0;
	int foundZPos = 0;
	int foundSections = 0;
	int yPos = 0;
	while( enkiNBTReadNextTag( pStream_ ) )
	{
		// Note that NBT data is stored in a somewhat random order so DataVersion might be at end
		// thus we cannot use it to decide parsing route without a multi-pass solution
		if( enkiNBTTAG_Int == pStream_->currentTag.tagId && enkiAreStringsEqual( "DataVersion", pStream_->currentTag.pName ) )
		{
			chunk.dataVersion = enkiNBTReadInt(pStream_ );
		}
		else if( enkiNBTTAG_Int == pStream_->currentTag.tagId &&  0 == foundXPos && enkiAreStringsEqual( "xPos", pStream_->currentTag.pName ) )
		{
			// In data version 2844+ xPos is at level 0
			foundXPos = 1;
			chunk.xPos = enkiNBTReadInt32( pStream_ );
		}
		else if( enkiNBTTAG_Int == pStream_->currentTag.tagId && 0 == foundZPos && enkiAreStringsEqual( "zPos", pStream_->currentTag.pName ) )
		{
			// In data version 2844+ yPos is at level 0
			foundZPos = 1;
			chunk.zPos = enkiNBTReadInt32( pStream_ );
		}
		else if( enkiNBTTAG_Int == pStream_->currentTag.tagId && enkiAreStringsEqual( "yPos", pStream_->currentTag.pName ) )
		{
			// yPos appears to indicate smallest y index, currently do not use
			yPos = enkiNBTReadInt32( pStream_ );
		}
		else if( enkiNBTTAG_List == pStream_->currentTag.tagId && 0 == foundSections && enkiAreStringsEqual( "sections", pStream_->currentTag.pName ) )
		{
			// In data version 2844+ the block data is stored under just a sections
			foundSections = 1;
			int8_t sectionY = 0;
			uint8_t* pBlockStates = NULL;
			enkiChunkSectionPalette sectionPalette = {0};
			int32_t levelSections = pStream_->level;
			if( 0 == pStream_->currentTag.listNumItems )
			{
				continue;
			}
			while( enkiNBTReadNextTag( pStream_ ) && pStream_->level > levelSections )
			{
				if( enkiNBTTAG_Compound == pStream_->currentTag.tagId && enkiAreStringsEqual( "block_states", pStream_->currentTag.pName ) )
				{
					// In data version 2844+ each section is under block_states
					int32_t levelBlock_states = pStream_->level;
					while( enkiNBTReadNextTag( pStream_ ) && pStream_->level > levelBlock_states )
					{
						if( enkiNBTTAG_Long_Array == pStream_->currentTag.tagId && NULL == pBlockStates && enkiAreStringsEqual( "data", pStream_->currentTag.pName ) )
						{
							sectionPalette.blockArraySize = enkiNBTReadInt32( pStream_ ); // read number of items to advance pCurrPos to start of array
							pBlockStates = pStream_->pCurrPos;
						}
						else if( enkiNBTTAG_List == pStream_->currentTag.tagId && 0 == sectionPalette.size && enkiAreStringsEqual( "palette", pStream_->currentTag.pName ) )
						{
							LoadChunkPalette( pStream_, &sectionPalette, params_ );
						}
					}
				}
				else if( enkiNBTTAG_Byte == pStream_->currentTag.tagId && enkiAreStringsEqual( "Y", pStream_->currentTag.pName ) )
				{
					// sectionY is not always present, and may indicate a start point.
					// For example, can find sectionY = -1 as first section, then next
					// section has data but no sectionY.
					sectionY = enkiNBTReadInt8( pStream_ );
				}
				else if( enkiNBTTAG_End == pStream_->currentTag.tagId && pStream_->level == levelSections + 1 )
				{
					// Section data is stored in compound tags under sections
					// So TAG_End found at levelSections+1 is the end of one section
					int32_t sectionIndex = (int32_t)sectionY + ENKI_MI_SECTIONS_Y_OFFSET;
					if( sectionIndex >= 0 && sectionIndex < ENKI_MI_NUM_SECTIONS_PER_CHUNK )
					{
						// if( pBlockStates && sectionPalette.size )
						{
							chunk.countOfSections++;
							chunk.palette[ sectionIndex ]  = sectionPalette;
							chunk.sections[ sectionIndex ] = pBlockStates;
							pBlockStates = NULL;
							memset( &sectionPalette, 0, sizeof(sectionPalette) );
						}
					}
					++sectionY;

					// This is a list of compound tags, ends with enkiNBTTAG_End at levelSections+1 
					if( pStream_->parentTags[ pStream_->level ].listCurrItem + 1 >=  pStream_->parentTags[ pStream_->level ].listNumItems )
					{
						break;
					}
				}
			}

		}
		else if( enkiNBTTAG_Compound == pStream_->currentTag.tagId && enkiAreStringsEqual( "Level", pStream_->currentTag.pName ) )
		{
			// Pre data version 2844 the block data is stored under a Level tag
			int levelLevel = pStream_->level;
			while( enkiNBTReadNextTag( pStream_ ) && pStream_->level > levelLevel ) // Last tag is TAG_End which is safe to read and skip
			{
				if( enkiNBTTAG_Int == pStream_->currentTag.tagId && 0 == foundXPos && enkiAreStringsEqual( "xPos", pStream_->currentTag.pName ) )
				{
					foundXPos = 1;
					chunk.xPos = enkiNBTReadInt32( pStream_ );
				}
				else if( enkiNBTTAG_Int == pStream_->currentTag.tagId && 0 == foundZPos && enkiAreStringsEqual( "zPos", pStream_->currentTag.pName ) )
				{
					foundZPos = 1;
					chunk.zPos = enkiNBTReadInt32( pStream_ );
				}
				else if( enkiNBTTAG_List == pStream_->currentTag.tagId && 0 == foundSections && enkiAreStringsEqual( "Sections", pStream_->currentTag.pName ) )
				{
					foundSections = 1;
					int8_t sectionY = 0;
					uint8_t* pBlocks = NULL;
					uint8_t* pData = NULL;
					uint8_t* pBlockStates = NULL;
					enkiChunkSectionPalette sectionPalette = {0};
					int32_t levelSections = pStream_->level;
					if( 0 == pStream_->currentTag.listNumItems )
					{
						continue;
					}
					while( enkiNBTReadNextTag( pStream_ ) && pStream_->level > levelSections )
					{

						if( enkiNBTTAG_Byte_Array == pStream_->currentTag.tagId && NULL == pBlocks && enkiAreStringsEqual( "Blocks", pStream_->currentTag.pName ) )
						{
							enkiNBTReadInt32( pStream_ ); // read number of items to advance pCurrPos to start of array
							pBlocks = pStream_->pCurrPos;
						}
						// TODO: process Add section
						 // https://minecraft.fandom.com/el/wiki/Chunk_format
						// Add: May not exist. 2048 bytes of additional block ID data. The value to add to (combine with) the above block ID to form the true block ID in the range 0 to 4095. 4 bits per block. Combining is done by shifting this value to the left 8 bits and then adding it to the block ID from above.
						else if( enkiNBTTAG_Byte_Array == pStream_->currentTag.tagId && enkiAreStringsEqual( "Add", pStream_->currentTag.pName ) )
						{
							// enkiNBTReadInt32( pStream_ ); // read number of items to advance pCurrPos to start of array
							// NOT YET HANDLED
						}
                        // Data: 2048 bytes of block data additionally defining parts of the terrain. 4 bits per block.
						else if( enkiNBTTAG_Byte_Array == pStream_->currentTag.tagId && NULL == pData && enkiAreStringsEqual( "Data", pStream_->currentTag.pName ) )
						{
							enkiNBTReadInt32( pStream_ ); // read number of items to advance pCurrPos to start of array
							pData = pStream_->pCurrPos;
						}
						else if( enkiNBTTAG_Byte == pStream_->currentTag.tagId && enkiAreStringsEqual( "Y", pStream_->currentTag.pName ) )
						{
							// sectionY is not always present, and may indicate a start point.
							// For example, can find sectionY = -1 as first section, then next
							// section has data but no sectionY.
							sectionY = enkiNBTReadInt8( pStream_ );
						}
						else if( enkiNBTTAG_Long_Array == pStream_->currentTag.tagId && NULL == pBlockStates && enkiAreStringsEqual( "BlockStates", pStream_->currentTag.pName ) )
						{
							sectionPalette.blockArraySize = enkiNBTReadInt32( pStream_ ); // read number of items to advance pCurrPos to start of array
							pBlockStates = pStream_->pCurrPos;
						}
						else if( enkiNBTTAG_List == pStream_->currentTag.tagId && 0 == sectionPalette.size && enkiAreStringsEqual( "Palette", pStream_->currentTag.pName ) )
						{
							LoadChunkPalette( pStream_, &sectionPalette, params_ );
						}
						else if( enkiNBTTAG_End == pStream_->currentTag.tagId && pStream_->level == levelSections + 1 )
						{
						    // Section data is stored in compound tags under sections
							// So TAG_End found at levelSections+1 is the end of one section
							int32_t sectionIndex = (int32_t)sectionY + ENKI_MI_SECTIONS_Y_OFFSET;
							if( sectionIndex >= 0 && sectionIndex < ENKI_MI_NUM_SECTIONS_PER_CHUNK )
							{
								if( pBlocks )
								{
									chunk.countOfSections++;
									assert( sectionPalette.size == 0 ); // a given chunk should use the same format
									chunk.sections[ sectionIndex ]   = pBlocks;
									chunk.dataValues[ sectionIndex ] = pData;
								}
								if( pBlockStates  && sectionPalette.size )
								{
									chunk.countOfSections++;
									assert( pBlocks == NULL ); // a given chunk should use the same format
									chunk.palette[ sectionIndex ]  = sectionPalette;
									chunk.sections[ sectionIndex ] = pBlockStates;
								}
							}
							pBlocks = NULL;
							pData   = NULL;
							pBlockStates = NULL;
							memset( &sectionPalette, 0, sizeof(sectionPalette) ); // allocations are added to the stream so do not need to free here

							++sectionY;

							// This is a list of compound tags, ends with enkiNBTTAG_End at levelSections+1 
							if(  pStream_->parentTags[ pStream_->level ].listCurrItem + 1 >=  pStream_->parentTags[ pStream_->level ].listNumItems )
							{
								break;
							}
						}
					}
				}
			}
		}
		if( foundXPos && foundZPos && foundSections )
		{
			// have all required data
			foundSectionData = 1;
		}

		if( foundSectionData && chunk.dataVersion )
		{
			// chunk complete with all data we use
			break;
		}
	}

	if( 0 == foundSectionData )
	{
		// reset to empty as did not find required information
		// memory allocation will be freed when stream freed
		// we keep data version around to enable this to be read out
		int32_t dataVersion = chunk.dataVersion;
		enkiChunkInit( &chunk );
		chunk.dataVersion = dataVersion;
	}
	return chunk;
}

enkiMICoordinate enkiGetChunkOrigin(enkiChunkBlockData * pChunk_)
{
	enkiMICoordinate retVal;
	retVal.x = pChunk_->xPos * ENKI_MI_SIZE_SECTIONS;
	retVal.y = 0;
	retVal.z = pChunk_->zPos * ENKI_MI_SIZE_SECTIONS;
	return retVal;
}

enkiMICoordinate enkiGetChunkSectionOrigin(enkiChunkBlockData * pChunk_, int32_t section_)
{
	enkiMICoordinate retVal;
	retVal.x = pChunk_->xPos * ENKI_MI_SIZE_SECTIONS;
	retVal.y = ( section_ - ENKI_MI_SECTIONS_Y_OFFSET ) * ENKI_MI_SIZE_SECTIONS;
	retVal.z = pChunk_->zPos * ENKI_MI_SIZE_SECTIONS;
	return retVal;
}

enkiMIVoxelData enkiGetChunkSectionVoxelData(enkiChunkBlockData * pChunk_, int32_t section_, enkiMICoordinate sectionOffset_)
{
	enkiMIVoxelData retVal;
	retVal.blockID     = 0;
	retVal.dataValue   = 0;
	retVal.paletteIndex = -1;

	assert( section_ < ENKI_MI_NUM_SECTIONS_PER_CHUNK );
	assert( 0 <= sectionOffset_.x && sectionOffset_.x < ENKI_MI_SIZE_SECTIONS );
	assert( 0 <= sectionOffset_.y && sectionOffset_.y < ENKI_MI_SIZE_SECTIONS );
	assert( 0 <= sectionOffset_.z && sectionOffset_.z < ENKI_MI_SIZE_SECTIONS );

	uint8_t* pSection    = pChunk_->sections[ section_ ];
	uint32_t paletteSize = pChunk_->palette[ section_ ].size;
	uint32_t posArray    = sectionOffset_.y*ENKI_MI_SIZE_SECTIONS*ENKI_MI_SIZE_SECTIONS + sectionOffset_.z*ENKI_MI_SIZE_SECTIONS + sectionOffset_.x;
	if( paletteSize )
	{
		// size depends on palette
		uint32_t numBits = pChunk_->palette[ section_ ].numBitsPerBlock;


		// Versions prior to 1.16 (DataVersion 2556) have block elements containing values stretching over multiple 64-bit fields.
		// 1.16 and above do not.
		uint32_t blockArrayValue = 0;
		if( pChunk_->dataVersion >= 2556 && pSection ) // pSection can be NULL if palette only has one entry
		{
			// do not need to handle bits spread across two uint64_t values
			uint32_t numPer64 = 64 / numBits;
			uint32_t pos64   = posArray / numPer64;
			uint32_t posIn64 = numBits * ( posArray - (pos64 * numPer64) );

			assert( pChunk_->palette[ section_ ].blockArraySize > pos64 );

			uint8_t* pVal64BigEndian = pSection + (8*(size_t)pos64);

			uint64_t val64 =   ( (uint64_t)pVal64BigEndian[0] << 56 ) + ( (uint64_t)pVal64BigEndian[1] << 48 )
							 + ( (uint64_t)pVal64BigEndian[2] << 40 ) + ( (uint64_t)pVal64BigEndian[3] << 32 )
							 + ( (uint64_t)pVal64BigEndian[4] << 24 ) + ( (uint64_t)pVal64BigEndian[5] << 16 )
							 + ( (uint64_t)pVal64BigEndian[6] <<  8 ) + ( (uint64_t)pVal64BigEndian[7]       );

			uint64_t val = val64 >> posIn64;

			uint64_t mask = (~(uint64_t)0) >> (64-numBits);
			uint32_t valmasked = (uint32_t)( val & mask );
			blockArrayValue = valmasked;
			assert( (uint32_t)pChunk_->palette[ section_ ].size > blockArrayValue );
		}
		else
		{

			uint32_t posBits = numBits * posArray;
			uint32_t pos64   = posBits / 64;
			uint32_t posIn64 = posBits - (pos64 * 64);

			assert( pChunk_->palette[ section_ ].blockArraySize > pos64 );

			uint8_t* pVal64BigEndian = pSection + (8*(size_t)pos64);

			uint64_t val64 =   ( (uint64_t)pVal64BigEndian[0] << 56 ) + ( (uint64_t)pVal64BigEndian[1] << 48 )
							 + ( (uint64_t)pVal64BigEndian[2] << 40 ) + ( (uint64_t)pVal64BigEndian[3] << 32 )
							 + ( (uint64_t)pVal64BigEndian[4] << 24 ) + ( (uint64_t)pVal64BigEndian[5] << 16 )
							 + ( (uint64_t)pVal64BigEndian[6] <<  8 ) + ( (uint64_t)pVal64BigEndian[7]       );

			uint64_t val = val64 >> posIn64;

			// handle 'overhang'
			uint32_t maxBitsPossibleIn64Bits = 64 - posIn64;
			uint32_t numBitsIn64      = maxBitsPossibleIn64Bits < numBits ? maxBitsPossibleIn64Bits : numBits;
			uint32_t overhangInNext64 = numBitsIn64 < numBits ? numBits-numBitsIn64 : 0;
				
			uint64_t mask = (~(uint64_t)0) >> (64-numBitsIn64);
			uint32_t valmasked = (uint32_t)( val & mask );

			if( overhangInNext64 )
			{
				uint8_t* pVal64BigEndianPlus1 = pVal64BigEndian + 8;;

				uint64_t val64_2 =  ( (uint64_t)pVal64BigEndianPlus1[0] << 56 ) + ( (uint64_t)pVal64BigEndianPlus1[1] << 48 )
								  + ( (uint64_t)pVal64BigEndianPlus1[2] << 40 ) + ( (uint64_t)pVal64BigEndianPlus1[3] << 32 )
								  + ( (uint64_t)pVal64BigEndianPlus1[4] << 24 ) + ( (uint64_t)pVal64BigEndianPlus1[5] << 16 )
								  + ( (uint64_t)pVal64BigEndianPlus1[6] <<  8 ) + ( (uint64_t)pVal64BigEndianPlus1[7]       );
				uint64_t mask_2      =(~(uint64_t)0) >> (64-overhangInNext64);
				uint64_t val_2       = val64_2;
				uint32_t valmasked_2 = (uint32_t)( val_2 & mask_2 );

				valmasked |= ( valmasked_2 << numBitsIn64 );
			}
			blockArrayValue = valmasked;
			assert( (uint32_t)pChunk_->palette[ section_ ].size > blockArrayValue );
		}

		if( (uint32_t)pChunk_->palette[ section_ ].size > blockArrayValue )
		{
			int32_t index = pChunk_->palette[ section_ ].pDefaultBlockIndex[ blockArrayValue ];
			retVal.blockID = 1; // default to 1, stone
			if( index >= 0 )
			{
				enkiMINamespaceAndBlockID* pNamespaceAndBlockID = &defaultNamespaceAndBlockIDs[index];
				retVal.blockID   = pNamespaceAndBlockID->blockID;
				retVal.dataValue = pNamespaceAndBlockID->dataValue;
			}
			retVal.paletteIndex = (int32_t)blockArrayValue;
		}

		
	}
	else
	{
		uint8_t* pVoxel = pSection + posArray;
		retVal.blockID = *pVoxel;
		if( pChunk_->dataValues[ section_ ] )
		{
			// 4 bit values
			uint32_t posByte = posArray / 2;
			uint32_t offsetByte = 4 * ( posArray - (2*posByte) );
			uint8_t byte = *( pChunk_->dataValues[ section_ ] + posByte );
			retVal.dataValue  = 0xF & ( byte >> offsetByte );
		}
	}
	return retVal;
}

uint8_t enkiGetChunkSectionVoxel( enkiChunkBlockData* pChunk_, int32_t section_, enkiMICoordinate sectionOffset_ )
{
	enkiMIVoxelData voxelData = enkiGetChunkSectionVoxelData( pChunk_, section_, sectionOffset_ );
	return voxelData.blockID;
}

uint32_t* enkiGetMineCraftPalette()
{
	return minecraftPalette;
}

enkiMINamespaceAndBlockIDTable enkiGetNamespaceAndBlockIDTable()
{
	enkiMINamespaceAndBlockIDTable defaultNamespaceAndBlockIDTable;
	defaultNamespaceAndBlockIDTable.namespaceAndBlockIDs    = defaultNamespaceAndBlockIDs;
	defaultNamespaceAndBlockIDTable.size                    = numDefaultNamespaceAndBlockIDs;
	return defaultNamespaceAndBlockIDTable;
}
