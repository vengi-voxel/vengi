/**
 * @file
 */

#pragma once

#include "core/RGBA.h"

namespace voxelformat {

struct BlockColor {
	int blockId;
	core::RGBA color;
};
static const BlockColor BLOCKCOLOR[]{
	{ 6, core::RGBA(49, 50, 49, 255) }, // WEAPON_CONTROLLER_ID
	{ 16, core::RGBA(53, 56, 56, 255) }, // WEAPON_ID
	{ 1, core::RGBA(66, 80, 81, 255) }, // CORE_ID
	{ 65, core::RGBA(62, 62, 61, 255) }, // DEATHSTAR_CORE_ID
	{ 63, core::RGBA(99, 171, 219, 56) }, // GLASS_ID
	{ 8, core::RGBA(62, 62, 62, 255) }, // THRUSTER_ID
	{ 7, core::RGBA(63, 61, 61, 255) }, // DOCK_ID
	{ 2, core::RGBA(58, 69, 69, 255) }, // POWER_ID
	{ 3, core::RGBA(92, 84, 73, 255) }, // SHIELD_ID
	{ 14, core::RGBA(89, 65, 60, 255) }, // EXPLOSIVE_ID
	{ 15, core::RGBA(133, 79, 73, 255) }, // RADAR_JAMMING_ID
	{ 22, core::RGBA(77, 107, 137, 255) }, // CLOAKING_ID
	{ 24, core::RGBA(53, 56, 52, 255) }, // SALVAGE_ID
	{ 38, core::RGBA(49, 49, 49, 255) }, // MISSILE_DUMB_CONTROLLER_ID
	{ 32, core::RGBA(63, 60, 59, 255) }, // MISSILE_DUMB_ID
	{ 46, core::RGBA(49, 50, 50, 255) }, // MISSILE_HEAT_CONTROLLER_ID
	{ 40, core::RGBA(56, 59, 61, 255) }, // MISSILE_HEAT_ID
	{ 54, core::RGBA(50, 49, 49, 255) }, // MISSILE_FAFO_CONTROLLER_ID
	{ 48, core::RGBA(60, 58, 55, 255) }, // MISSILE_FAFO_ID
	{ 4, core::RGBA(49, 50, 49, 255) }, // SALVAGE_CONTROLLER_ID
	{ 56, core::RGBA(57, 56, 53, 255) }, // GRAVITY_ID
	{ 30, core::RGBA(61, 58, 58, 255) }, // REPAIR_ID
	{ 39, core::RGBA(49, 49, 49, 255) }, // REPAIR_CONTROLLER_ID
	{ 47, core::RGBA(61, 66, 63, 255) }, // COCKPIT_ID
	{ 55, core::RGBA(147, 147, 148, 255) }, // LIGHT_ID
	{ 62, core::RGBA(149, 150, 150, 255) }, // LIGHT_BEACON_ID
	{ 64, core::RGBA(155, 199, 223, 208) }, // TERRAIN_ICE_ID
	{ 112, core::RGBA(65, 52, 26, 255) }, // LANDING_ELEMENT
	{ 113, core::RGBA(45, 46, 46, 255) }, // LIFT_ELEMENT
	{ 114, core::RGBA(135, 86, 117, 255) }, // RECYCLER_ELEMENT
	{ 120, core::RGBA(69, 80, 78, 255) }, // STASH_ELEMENT
	{ 121, core::RGBA(69, 54, 48, 255) }, // AI_ELEMENT
	{ 122, core::RGBA(28, 27, 27, 255) }, // DOOR_ELEMENT
	{ 123, core::RGBA(83, 77, 64, 255) }, // BUILD_BLOCK_ID
	{ 80, core::RGBA(239, 111, 3, 255) }, // TERRAIN_LAVA_ID
	{ 128, core::RGBA(95, 58, 33, 255) }, // TERRAIN_EXOGEN_ID
	{ 129, core::RGBA(187, 133, 62, 255) }, // TERRAIN_OCTOGEN_ID
	{ 130, core::RGBA(95, 58, 33, 255) }, // TERRAIN_QUANTAGEN_ID
	{ 131, core::RGBA(95, 58, 33, 255) }, // TERRAIN_QUANTANIUM_ID
	{ 132, core::RGBA(95, 58, 33, 255) }, // TERRAIN_PLEXTANIUM_ID
	{ 133, core::RGBA(95, 58, 33, 255) }, // TERRAIN_ORANGUTANIUM_ID
	{ 134, core::RGBA(187, 133, 62, 255) }, // TERRAIN_SUCCUMITE_ID
	{ 135, core::RGBA(95, 58, 33, 255) }, // TERRAIN_CENOMITE_ID
	{ 136, core::RGBA(187, 133, 62, 255) }, // TERRAIN_AWESOMITE_ID
	{ 137, core::RGBA(95, 58, 33, 255) }, // TERRAIN_VAPPECIDE_ID
	{ 138, core::RGBA(56, 23, 18, 255) }, // TERRAIN_MARS_TOP
	{ 139, core::RGBA(17, 23, 28, 255) }, // TERRAIN_MARS_ROCK
	{ 140, core::RGBA(56, 23, 18, 255) }, // TERRAIN_MARS_DIRT
	{ 141, core::RGBA(17, 23, 28, 255) }, // TERRAIN_MARS_TOP_ROCK
	{ 72, core::RGBA(95, 58, 33, 255) }, // TERRAIN_EXTRANIUM_ID
	{ 73, core::RGBA(115, 105, 102, 255) }, // TERRAIN_ROCK_ID
	{ 74, core::RGBA(187, 133, 62, 255) }, // TERRAIN_SAND_ID
	{ 82, core::RGBA(95, 58, 33, 255) }, // TERRAIN_EARTH_TOP_DIRT
	{ 83, core::RGBA(115, 105, 102, 255) }, // TERRAIN_EARTH_TOP_ROCK
	{ 84, core::RGBA(51, 39, 26, 255) }, // TERRAIN_TREE_TRUNK_ID
	{ 85, core::RGBA(31, 47, 19, 255) }, // TERRAIN_TREE_LEAF_ID
	{ 86, core::RGBA(70, 164, 186, 51) }, // TERRAIN_WATER
	{ 87, core::RGBA(95, 58, 33, 255) }, // TERRAIN_DIRT_ID
	{ 88, core::RGBA(66, 60, 60, 255) }, // DOCKING_ENHANCER_ID
	{ 89, core::RGBA(85, 82, 61, 255) }, // TERRAIN_CACTUS_ID
	{ 90, core::RGBA(33, 26, 48, 255) }, // TERRAIN_PURPLE_ALIEN_TOP
	{ 91, core::RGBA(33, 26, 48, 255) }, // TERRAIN_PURPLE_ALIEN_ROCK
	{ 92, core::RGBA(53, 32, 64, 255) }, // TERRAIN_PURPLE_ALIEN_VINE
	{ 93, core::RGBA(84, 112, 114, 255) }, // TERRAIN_GRASS_SPRITE
	{ 94, core::RGBA(135, 69, 57, 255) }, // PLAYER_SPAWN_MODULE
	{ 95, core::RGBA(73, 74, 15, 255) }, // TERRAIN_BROWNWEED_SPRITE
	{ 96, core::RGBA(186, 90, 56, 255) }, // TERRAIN_MARSTENTACLES_SPRITE
	{ 97, core::RGBA(54, 65, 95, 255) }, // TERRAIN_ALIENVINE_SPRITE
	{ 98, core::RGBA(59, 76, 9, 255) }, // TERRAIN_GRASSFLOWERS_SPRITE
	{ 99, core::RGBA(121, 100, 44, 255) }, // TERRAIN_LONGWEED_SPRITE
	{ 100, core::RGBA(178, 91, 55, 255) }, // TERRAIN_TALLSHROOM_SPRITE
	{ 101, core::RGBA(28, 47, 63, 255) }, // TERRAIN_PURSPIRE_SPRITE
	{ 102, core::RGBA(82, 94, 21, 255) }, // TERRAIN_TALLGRASSFLOWERS_SPRITE
	{ 103, core::RGBA(118, 96, 36, 255) }, // TERRAIN_MINICACTUS_SPRITE
	{ 104, core::RGBA(137, 78, 52, 255) }, // TERRAIN_REDSHROOM_SPRITE
	{ 105, core::RGBA(57, 53, 78, 255) }, // TERRAIN_PURPTACLES_SPRITE
	{ 106, core::RGBA(130, 117, 19, 255) }, // TERRAIN_TALLFLOWERS_SPRITE
	{ 107, core::RGBA(107, 95, 79, 255) }, // TERRAIN_ROCK_SPRITE
	{ 108, core::RGBA(141, 62, 37, 255) }, // TERRAIN_ALIENFLOWERS_SPRITE
	{ 109, core::RGBA(47, 40, 58, 255) }, // TERRAIN_YHOLE_SPRITE
	{ 142, core::RGBA(92, 106, 126, 255) }, // TERRAIN_M1L2_ID
	{ 143, core::RGBA(6, 73, 113, 255) }, // TERRAIN_M1L3_ID
	{ 144, core::RGBA(119, 127, 130, 255) }, // TERRAIN_M1L4_ID
	{ 145, core::RGBA(28, 93, 130, 255) }, // TERRAIN_M1L5_ID
	{ 146, core::RGBA(162, 138, 89, 255) }, // TERRAIN_M2L2_ID
	{ 147, core::RGBA(239, 111, 3, 255) }, // TERRAIN_M2L3_ID
	{ 148, core::RGBA(70, 164, 186, 51) }, // TERRAIN_M2L4_ID
	{ 149, core::RGBA(70, 164, 186, 51) }, // TERRAIN_M2L5_ID
	{ 150, core::RGBA(147, 104, 74, 255) }, // TERRAIN_M3L2_ID
	{ 151, core::RGBA(131, 62, 30, 255) }, // TERRAIN_M3L3_ID
	{ 152, core::RGBA(142, 120, 107, 255) }, // TERRAIN_M3L4_ID
	{ 153, core::RGBA(135, 73, 45, 255) }, // TERRAIN_M3L5_ID
	{ 154, core::RGBA(143, 125, 77, 255) }, // TERRAIN_M4L2_ID
	{ 155, core::RGBA(117, 89, 43, 255) }, // TERRAIN_M4L3_ID
	{ 156, core::RGBA(140, 133, 106, 255) }, // TERRAIN_M4L4_ID
	{ 157, core::RGBA(156, 120, 71, 255) }, // TERRAIN_M4L5_ID
	{ 158, core::RGBA(167, 163, 158, 255) }, // TERRAIN_M5L2_ID
	{ 159, core::RGBA(102, 102, 102, 255) }, // TERRAIN_M5L3_ID
	{ 160, core::RGBA(148, 143, 139, 255) }, // TERRAIN_M5L4_ID
	{ 161, core::RGBA(167, 167, 167, 255) }, // TERRAIN_M5L5_ID
	{ 162, core::RGBA(106, 97, 119, 255) }, // TERRAIN_M6L2_ID
	{ 163, core::RGBA(58, 39, 82, 255) }, // TERRAIN_M6L3_ID
	{ 164, core::RGBA(127, 117, 123, 255) }, // TERRAIN_M6L4_ID
	{ 165, core::RGBA(54, 39, 79, 255) }, // TERRAIN_M6L5_ID
	{ 166, core::RGBA(159, 154, 149, 255) }, // TERRAIN_M7L2_ID
	{ 167, core::RGBA(239, 111, 3, 255) }, // TERRAIN_M7L3_ID
	{ 168, core::RGBA(70, 164, 186, 51) }, // TERRAIN_M7L4_ID
	{ 169, core::RGBA(70, 164, 186, 51) }, // TERRAIN_M7L5_ID
	{ 170, core::RGBA(131, 90, 89, 255) }, // TERRAIN_M8L2_ID
	{ 171, core::RGBA(98, 36, 30, 255) }, // TERRAIN_M8L3_ID
	{ 172, core::RGBA(136, 114, 108, 255) }, // TERRAIN_M8L4_ID
	{ 173, core::RGBA(104, 43, 36, 255) }, // TERRAIN_M8L5_ID
	{ 174, core::RGBA(145, 113, 154, 255) }, // TERRAIN_M9L2_ID
	{ 175, core::RGBA(239, 111, 3, 255) }, // TERRAIN_M9L3_ID
	{ 176, core::RGBA(70, 164, 186, 51) }, // TERRAIN_M9L4_ID
	{ 177, core::RGBA(70, 164, 186, 51) }, // TERRAIN_M9L5_ID
	{ 178, core::RGBA(98, 123, 100, 255) }, // TERRAIN_M10L2_ID
	{ 179, core::RGBA(42, 110, 63, 255) }, // TERRAIN_M10L3_ID
	{ 180, core::RGBA(122, 131, 117, 255) }, // TERRAIN_M10L4_ID
	{ 181, core::RGBA(55, 120, 77, 255) }, // TERRAIN_M10L5_ID
	{ 182, core::RGBA(150, 140, 86, 255) }, // TERRAIN_M11L2_ID
	{ 183, core::RGBA(239, 111, 3, 255) }, // TERRAIN_M11L3_ID
	{ 184, core::RGBA(70, 164, 186, 51) }, // TERRAIN_M11L4_ID
	{ 185, core::RGBA(70, 164, 186, 51) }, // TERRAIN_M11L5_ID
	{ 186, core::RGBA(115, 148, 119, 255) }, // TERRAIN_M12L2_ID
	{ 187, core::RGBA(239, 111, 3, 255) }, // TERRAIN_M12L3_ID
	{ 188, core::RGBA(70, 164, 186, 51) }, // TERRAIN_M12L4_ID
	{ 189, core::RGBA(70, 164, 186, 51) }, // TERRAIN_M12L5_ID
	{ 190, core::RGBA(116, 144, 154, 255) }, // TERRAIN_M13L2_ID
	{ 191, core::RGBA(239, 111, 3, 255) }, // TERRAIN_M13L3_ID
	{ 192, core::RGBA(70, 164, 186, 51) }, // TERRAIN_M13L4_ID
	{ 193, core::RGBA(70, 164, 186, 51) }, // TERRAIN_M13L5_ID
	{ 194, core::RGBA(153, 106, 92, 255) }, // TERRAIN_M14L2_ID
	{ 195, core::RGBA(239, 111, 3, 255) }, // TERRAIN_M14L3_ID
	{ 196, core::RGBA(70, 164, 186, 51) }, // TERRAIN_M14L4_ID
	{ 197, core::RGBA(70, 164, 186, 51) }, // TERRAIN_M14L5_ID
	{ 198, core::RGBA(125, 112, 120, 255) }, // TERRAIN_M15L2_ID
	{ 199, core::RGBA(239, 111, 3, 255) }, // TERRAIN_M15L3_ID
	{ 200, core::RGBA(70, 164, 186, 51) }, // TERRAIN_M15L4_ID
	{ 201, core::RGBA(70, 164, 186, 51) }, // TERRAIN_M15L5_ID
	{ 202, core::RGBA(96, 95, 99, 255) }, // TERRAIN_M16L2_ID
	{ 203, core::RGBA(41, 41, 41, 255) }, // TERRAIN_M16L3_ID
	{ 204, core::RGBA(118, 112, 108, 255) }, // TERRAIN_M16L4_ID
	{ 205, core::RGBA(27, 27, 27, 255) }, // TERRAIN_M16L5_ID
	{ 206, core::RGBA(187, 133, 62, 255) }, // TERRAIN_NEGACIDE_ID
	{ 207, core::RGBA(95, 58, 33, 255) }, // TERRAIN_QUANTACIDE_ID
	{ 208, core::RGBA(187, 133, 62, 255) }, // TERRAIN_NEGAGATE_ID
	{ 209, core::RGBA(187, 133, 62, 255) }, // TERRAIN_METATE_ID
	{ 210, core::RGBA(187, 133, 62, 255) }, // TERRAIN_INSANIUM_ID
	{ 211, core::RGBA(60, 59, 54, 255) }, // FACTORY_INPUT_ID
	{ 212, core::RGBA(52, 51, 46, 255) }, // FACTORY_INPUT_ENH_ID
	{ 213, core::RGBA(54, 58, 59, 255) }, // FACTORY_POWER_CELL_ID
	{ 214, core::RGBA(52, 51, 46, 255) }, // FACTORY_POWER_CELL_ENH_ID
	{ 215, core::RGBA(54, 58, 59, 255) }, // FACTORY_POWER_COIL_ID
	{ 216, core::RGBA(52, 51, 46, 255) }, // FACTORY_POWER_COIL_ENH_ID
	{ 217, core::RGBA(60, 57, 53, 255) }, // FACTORY_POWER_BLOCK_ID
	{ 218, core::RGBA(52, 51, 46, 255) }, // FACTORY_POWER_BLOCK_ENH_ID
	{ 219, core::RGBA(35, 35, 34, 166) }, // POWER_CELL_ID
	{ 220, core::RGBA(35, 35, 34, 166) }, // POWER_COIL_ID
	{ 222, core::RGBA(54, 58, 59, 255) }, // FACTORY_PARTICLE_PRESS
	{ 223, core::RGBA(49, 77, 54, 255) }, // MAN_SD1000_CAP
	{ 224, core::RGBA(8, 63, 28, 154) }, // MAN_SD2000_CAP
	{ 225, core::RGBA(6, 50, 16, 165) }, // MAN_SD3000_CAP
	{ 226, core::RGBA(74, 50, 50, 255) }, // MAN_SD1000_FLUX
	{ 227, core::RGBA(66, 11, 9, 154) }, // MAN_SD2000_FLUX
	{ 228, core::RGBA(50, 17, 7, 165) }, // MAN_SD3000_FLUX
	{ 229, core::RGBA(50, 69, 77, 255) }, // MAN_SD1000_MICRO
	{ 230, core::RGBA(6, 49, 66, 154) }, // MAN_SD2000_MICRO
	{ 231, core::RGBA(6, 38, 50, 165) }, // MAN_SD3000_MICRO
	{ 232, core::RGBA(73, 73, 73, 255) }, // MAN_SD1000_DELTA
	{ 233, core::RGBA(60, 60, 60, 154) }, // MAN_SD2000_DELTA
	{ 234, core::RGBA(220, 210, 205, 155) }, // MAN_SD3000_DELTA
	{ 235, core::RGBA(77, 75, 49, 255) }, // MAN_SD1000_MEM
	{ 236, core::RGBA(50, 57, 3, 154) }, // MAN_SD2000_MEM
	{ 237, core::RGBA(63, 63, 7, 128) }, // MAN_SD3000_MEM
	{ 238, core::RGBA(144, 91, 37, 255) }, // MAN_SDPROTON
	{ 239, core::RGBA(126, 63, 55, 255) }, // MAN_RED
	{ 240, core::RGBA(110, 87, 128, 255) }, // MAN_PURP
	{ 241, core::RGBA(97, 72, 47, 255) }, // MAN_BROWN
	{ 242, core::RGBA(90, 112, 69, 255) }, // MAN_GREEN
	{ 243, core::RGBA(126, 115, 55, 255) }, // MAN_YELLOW
	{ 244, core::RGBA(80, 82, 83, 255) }, // MAN_BLACK
	{ 245, core::RGBA(156, 158, 159, 255) }, // MAN_WHITE
	{ 246, core::RGBA(66, 100, 114, 255) }, // MAN_BLUE
	{ 247, core::RGBA(77, 66, 49, 255) }, // MAN_P1000B
	{ 248, core::RGBA(58, 40, 6, 154) }, // MAN_P2000B
	{ 249, core::RGBA(54, 37, 3, 165) }, // MAN_P3000B
	{ 250, core::RGBA(65, 54, 70, 255) }, // MAN_P10000A
	{ 251, core::RGBA(49, 19, 53, 154) }, // MAN_P20000A
	{ 252, core::RGBA(35, 12, 62, 165) }, // MAN_P30000A
	{ 253, core::RGBA(220, 210, 205, 155) }, // MAN_P40000A
	{ 254, core::RGBA(50, 50, 50, 255) }, // MAN_YHOLE_NUC
	{ 255, core::RGBA(54, 58, 59, 255) }, // FACTORY_SD10000
	{ 256, core::RGBA(54, 58, 59, 255) }, // FACTORY_SD20000
	{ 257, core::RGBA(54, 58, 59, 255) }, // FACTORY_SD30000
	{ 258, core::RGBA(54, 58, 59, 255) }, // FACTORY_SDADV
	{ 259, core::RGBA(60, 56, 53, 255) }, // FACTORY_SD1000
	{ 260, core::RGBA(54, 58, 59, 255) }, // FACTORY_SD2000
	{ 261, core::RGBA(54, 58, 59, 255) }, // FACTORY_SD3000
	{ 262, core::RGBA(54, 58, 59, 255) }, // FACTORY_MINERAL
	{ 272, core::RGBA(17, 17, 17, 154) }, // MAN_GLASS_BOTTLE
	{ 273, core::RGBA(28, 28, 28, 165) }, // MAN_SCIENCE_BOTTLE
	{ 274, core::RGBA(79, 119, 137, 255) }, // TERRAIN_ICEPLANET_SURFACE
	{ 275, core::RGBA(79, 119, 137, 255) }, // TERRAIN_ICEPLANET_ROCK
	{ 276, core::RGBA(51, 39, 26, 255) }, // TERRAIN_ICEPLANET_WOOD
	{ 277, core::RGBA(31, 47, 19, 255) }, // TERRAIN_ICEPLANET_LEAVES
	{ 278, core::RGBA(58, 122, 135, 255) }, // TERRAIN_ICEPLANET_SPIKE_SPRITE
	{ 279, core::RGBA(92, 158, 167, 255) }, // TERRAIN_ICEPLANET_ICECRAG_SPRITE
	{ 280, core::RGBA(48, 95, 103, 255) }, // TERRAIN_ICEPLANET_ICECORAL_SPRITE
	{ 281, core::RGBA(29, 83, 93, 255) }, // TERRAIN_ICEPLANET_ICEGRASS_SPRITE
	{ 282, core::RGBA(135, 69, 57, 255) }, // LIGHT_RED
	{ 283, core::RGBA(52, 113, 139, 255) }, // LIGHT_BLUE
	{ 284, core::RGBA(79, 139, 53, 255) }, // LIGHT_GREEN
	{ 285, core::RGBA(122, 128, 63, 255) }, // LIGHT_YELLOW
	{ 286, core::RGBA(70, 180, 210, 255) }, // TERRAIN_ICEPLANET_CRYSTAL
	{ 287, core::RGBA(51, 39, 26, 255) }, // TERRAIN_REDWOOD
	{ 288, core::RGBA(31, 47, 19, 255) }, // TERRAIN_REDWOOD_LEAVES
	{ 289, core::RGBA(62, 62, 61, 255) }, // FIXED_DOCK_ID
	{ 290, core::RGBA(62, 66, 61, 255) }, // FIXED_DOCK_ID_ENHANCER
	{ 291, core::RGBA(90, 134, 80, 255) }, // FACTION_BLOCK
	{ 292, core::RGBA(90, 134, 80, 255) }, // FACTION_HUB_BLOCK
	{ 331, core::RGBA(66, 92, 80, 255) }, // POWER_HOLDER_ID
	{ 332, core::RGBA(135, 86, 117, 255) }, // POWER_DRAIN_BEAM_COMPUTER
	{ 333, core::RGBA(78, 59, 73, 255) }, // POWER_DRAIN_BEAM_MODULE
	{ 334, core::RGBA(74, 136, 125, 255) }, // POWER_SUPPLY_BEAM_COMPUTER
	{ 335, core::RGBA(59, 78, 70, 255) }, // POWER_SUPPLY_BEAM_MODULE
	{ 336, core::RGBA(65, 67, 68, 255) }, // DECORATIVE_PANEL_1
	{ 337, core::RGBA(65, 68, 69, 255) }, // DECORATIVE_PANEL_2
	{ 338, core::RGBA(71, 84, 78, 255) }, // DECORATIVE_PANEL_3
	{ 339, core::RGBA(69, 87, 97, 255) }, // DECORATIVE_PANEL_4
	{ 340, core::RGBA(143, 132, 55, 255) }, // LIGHT_BULB_YELLOW
	{ 341, core::RGBA(224, 110, 4, 255) }, // SPRITE_BRONZE
	{ 342, core::RGBA(173, 191, 194, 255) }, // SPRITE_SILVER
	{ 343, core::RGBA(253, 181, 3, 255) }, // SPRITE_GOLD
	{ 344, core::RGBA(50, 49, 49, 255) }, // PULSE_CONTROLLER_ID
	{ 345, core::RGBA(59, 57, 54, 255) }, // PULSE_ID
	{ 346, core::RGBA(61, 62, 62, 255) }, // FRACTION_EXCEPTION_ID
	{ 347, core::RGBA(61, 103, 142, 255) }, // SHOP_BLOCK_ID
	{ 329, core::RGBA(99, 171, 219, 56) }, // GLASS_WEDGE_ID
	{ 330, core::RGBA(99, 171, 219, 56) }, // GLASS_CORNER_ID
	{ 367, core::RGBA(99, 171, 219, 56) }, // GLASS_PENTA_ID
	{ 368, core::RGBA(99, 171, 219, 56) }, // GLASS_TETRA_ID
	{ 5, core::RGBA(60, 60, 60, 255) }, // HULL_COLOR_GREY_ID
	{ 69, core::RGBA(54, 35, 71, 255) }, // HULL_COLOR_PURPLE_ID
	{ 70, core::RGBA(70, 50, 28, 255) }, // HULL_COLOR_BROWN_ID
	{ 75, core::RGBA(13, 13, 13, 255) }, // HULL_COLOR_BLACK_ID
	{ 76, core::RGBA(76, 24, 14, 255) }, // HULL_COLOR_RED_ID
	{ 77, core::RGBA(24, 51, 75, 255) }, // HULL_COLOR_BLUE_ID
	{ 78, core::RGBA(28, 62, 36, 255) }, // HULL_COLOR_GREEN_ID
	{ 79, core::RGBA(123, 106, 7, 255) }, // HULL_COLOR_YELLOW_ID
	{ 81, core::RGBA(141, 141, 141, 255) }, // HULL_COLOR_WHITE_ID
	{ 426, core::RGBA(114, 68, 14, 255) }, // HULL_COLOR_ORANGE_ID
	{ 293, core::RGBA(60, 60, 60, 255) }, // HULL_COLOR_WEDGE_GREY_ID
	{ 294, core::RGBA(54, 35, 71, 255) }, // HULL_COLOR_WEDGE_PURPLE_ID
	{ 295, core::RGBA(70, 50, 28, 255) }, // HULL_COLOR_WEDGE_BROWN_ID
	{ 296, core::RGBA(13, 13, 13, 255) }, // HULL_COLOR_WEDGE_BLACK_ID
	{ 297, core::RGBA(76, 24, 14, 255) }, // HULL_COLOR_WEDGE_RED_ID
	{ 298, core::RGBA(24, 51, 75, 255) }, // HULL_COLOR_WEDGE_BLUE_ID
	{ 299, core::RGBA(28, 62, 36, 255) }, // HULL_COLOR_WEDGE_GREEN_ID
	{ 300, core::RGBA(123, 106, 7, 255) }, // HULL_COLOR_WEDGE_YELLOW_ID
	{ 301, core::RGBA(141, 141, 141, 255) }, // HULL_COLOR_WEDGE_WHITE_ID
	{ 427, core::RGBA(114, 68, 14, 255) }, // HULL_COLOR_WEDGE_ORANGE_ID
	{ 302, core::RGBA(60, 60, 60, 255) }, // HULL_COLOR_CORNER_GREY_ID
	{ 303, core::RGBA(54, 35, 71, 255) }, // HULL_COLOR_CORNER_PURPLE_ID
	{ 304, core::RGBA(70, 50, 28, 255) }, // HULL_COLOR_CORNER_BROWN_ID
	{ 305, core::RGBA(13, 13, 13, 255) }, // HULL_COLOR_CORNER_BLACK_ID
	{ 306, core::RGBA(76, 24, 14, 255) }, // HULL_COLOR_CORNER_RED_ID
	{ 307, core::RGBA(24, 51, 75, 255) }, // HULL_COLOR_CORNER_BLUE_ID
	{ 308, core::RGBA(28, 62, 36, 255) }, // HULL_COLOR_CORNER_GREEN_ID
	{ 309, core::RGBA(123, 106, 7, 255) }, // HULL_COLOR_CORNER_YELLOW_ID
	{ 310, core::RGBA(141, 141, 141, 255) }, // HULL_COLOR_CORNER_WHITE_ID
	{ 428, core::RGBA(114, 68, 14, 255) }, // HULL_COLOR_CORNER_ORANGE_ID
	{ 357, core::RGBA(60, 60, 60, 255) }, // HULL_COLOR_PENTA_GREY_ID
	{ 385, core::RGBA(13, 13, 13, 255) }, // HULL_COLOR_PENTA_BLACK_ID
	{ 386, core::RGBA(76, 24, 14, 255) }, // HULL_COLOR_PENTA_RED_ID
	{ 387, core::RGBA(54, 35, 71, 255) }, // HULL_COLOR_PENTA_PURPLE_ID
	{ 388, core::RGBA(24, 51, 75, 255) }, // HULL_COLOR_PENTA_BLUE_ID
	{ 389, core::RGBA(28, 62, 36, 255) }, // HULL_COLOR_PENTA_GREEN_ID
	{ 403, core::RGBA(70, 50, 28, 255) }, // HULL_COLOR_PENTA_BROWN_ID
	{ 391, core::RGBA(123, 106, 7, 255) }, // HULL_COLOR_PENTA_YELLOW_ID
	{ 392, core::RGBA(141, 141, 141, 255) }, // HULL_COLOR_PENTA_WHITE_ID
	{ 429, core::RGBA(114, 68, 14, 255) }, // HULL_COLOR_PENTA_ORANGE_ID
	{ 348, core::RGBA(60, 60, 60, 255) }, // HULL_COLOR_TETRA_GREY_ID
	{ 393, core::RGBA(13, 13, 13, 255) }, // HULL_COLOR_TETRA_BLACK_ID
	{ 394, core::RGBA(76, 24, 14, 255) }, // HULL_COLOR_TETRA_RED_ID
	{ 395, core::RGBA(54, 35, 71, 255) }, // HULL_COLOR_TETRA_PURPLE_ID
	{ 396, core::RGBA(24, 51, 75, 255) }, // HULL_COLOR_TETRA_BLUE_ID
	{ 397, core::RGBA(28, 62, 36, 255) }, // HULL_COLOR_TETRA_GREEN_ID
	{ 398, core::RGBA(123, 106, 7, 255) }, // HULL_COLOR_TETRA_YELLOW_ID
	{ 400, core::RGBA(141, 141, 141, 255) }, // HULL_COLOR_TETRA_WHITE_ID
	{ 404, core::RGBA(70, 50, 28, 255) }, // HULL_COLOR_TETRA_BROWN_ID
	{ 430, core::RGBA(114, 68, 14, 255) }, // HULL_COLOR_TETRA_ORANGE_ID
	{ 263, core::RGBA(61, 61, 61, 255) }, // POWERHULL_COLOR_GREY
	{ 264, core::RGBA(18, 18, 18, 255) }, // POWERHULL_COLOR_BLACK
	{ 265, core::RGBA(77, 26, 16, 255) }, // POWERHULL_COLOR_RED
	{ 266, core::RGBA(50, 33, 65, 255) }, // POWERHULL_COLOR_PURPLE
	{ 267, core::RGBA(21, 48, 71, 255) }, // POWERHULL_COLOR_BLUE
	{ 268, core::RGBA(33, 64, 37, 255) }, // POWERHULL_COLOR_GREEN
	{ 269, core::RGBA(68, 47, 26, 255) }, // POWERHULL_COLOR_BROWN
	{ 270, core::RGBA(121, 102, 0, 255) }, // POWERHULL_COLOR_YELLOW
	{ 271, core::RGBA(139, 139, 139, 255) }, // POWERHULL_COLOR_WHITE
	{ 431, core::RGBA(113, 66, 5, 255) }, // POWERHULL_COLOR_ORANGE
	{ 311, core::RGBA(61, 61, 61, 255) }, // POWERHULL_COLOR_WEDGE_GREY
	{ 312, core::RGBA(18, 18, 18, 255) }, // POWERHULL_COLOR_WEDGE_BLACK
	{ 313, core::RGBA(77, 26, 16, 255) }, // POWERHULL_COLOR_WEDGE_RED
	{ 314, core::RGBA(50, 33, 65, 255) }, // POWERHULL_COLOR_WEDGE_PURPLE
	{ 315, core::RGBA(21, 48, 71, 255) }, // POWERHULL_COLOR_WEDGE_BLUE
	{ 316, core::RGBA(33, 64, 37, 255) }, // POWERHULL_COLOR_WEDGE_GREEN
	{ 317, core::RGBA(68, 47, 26, 255) }, // POWERHULL_COLOR_WEDGE_BROWN
	{ 318, core::RGBA(121, 102, 0, 255) }, // POWERHULL_COLOR_WEDGE_YELLOW
	{ 319, core::RGBA(139, 139, 139, 255) }, // POWERHULL_COLOR_WEDGE_WHITE
	{ 432, core::RGBA(113, 66, 5, 255) }, // POWERHULL_COLOR_WEDGE_ORANGE
	{ 320, core::RGBA(61, 61, 61, 255) }, // POWERHULL_COLOR_CORNER_GREY
	{ 321, core::RGBA(18, 18, 18, 255) }, // POWERHULL_COLOR_CORNER_BLACK
	{ 322, core::RGBA(77, 26, 16, 255) }, // POWERHULL_COLOR_CORNER_RED
	{ 323, core::RGBA(50, 33, 65, 255) }, // POWERHULL_COLOR_CORNER_PURPLE
	{ 324, core::RGBA(21, 48, 71, 255) }, // POWERHULL_COLOR_CORNER_BLUE
	{ 325, core::RGBA(33, 64, 37, 255) }, // POWERHULL_COLOR_CORNER_GREEN
	{ 326, core::RGBA(68, 47, 26, 255) }, // POWERHULL_COLOR_CORNER_BROWN
	{ 327, core::RGBA(121, 102, 0, 255) }, // POWERHULL_COLOR_CORNER_YELLOW
	{ 328, core::RGBA(139, 139, 139, 255) }, // POWERHULL_COLOR_CORNER_WHITE
	{ 433, core::RGBA(113, 66, 5, 255) }, // POWERHULL_COLOR_CORNER_ORANGE
	{ 369, core::RGBA(18, 18, 18, 255) }, // POWERHULL_COLOR_PENTA_BLACK
	{ 370, core::RGBA(77, 26, 16, 255) }, // POWERHULL_COLOR_PENTA_RED
	{ 371, core::RGBA(50, 33, 65, 255) }, // POWERHULL_COLOR_PENTA_PURPLE
	{ 372, core::RGBA(21, 48, 71, 255) }, // POWERHULL_COLOR_PENTA_BLUE
	{ 373, core::RGBA(33, 64, 37, 255) }, // POWERHULL_COLOR_PENTA_GREEN
	{ 374, core::RGBA(68, 47, 26, 255) }, // POWERHULL_COLOR_PENTA_BROWN
	{ 375, core::RGBA(121, 102, 0, 255) }, // POWERHULL_COLOR_PENTA_YELLOW
	{ 376, core::RGBA(139, 139, 139, 255) }, // POWERHULL_COLOR_PENTA_WHITE
	{ 401, core::RGBA(61, 61, 61, 255) }, // POWERHULL_COLOR_PENTA_GREY
	{ 434, core::RGBA(113, 66, 5, 255) }, // POWERHULL_COLOR_PENTA_ORANGE
	{ 377, core::RGBA(18, 18, 18, 255) }, // POWERHULL_COLOR_TETRA_BLACK
	{ 378, core::RGBA(77, 26, 16, 255) }, // POWERHULL_COLOR_TETRA_RED
	{ 379, core::RGBA(50, 33, 65, 255) }, // POWERHULL_COLOR_TETRA_PURPLE
	{ 380, core::RGBA(21, 48, 71, 255) }, // POWERHULL_COLOR_TETRA_BLUE
	{ 381, core::RGBA(33, 64, 37, 255) }, // POWERHULL_COLOR_TETRA_GREEN
	{ 382, core::RGBA(68, 47, 26, 255) }, // POWERHULL_COLOR_TETRA_BROWN
	{ 383, core::RGBA(121, 102, 0, 255) }, // POWERHULL_COLOR_TETRA_YELLOW
	{ 384, core::RGBA(139, 139, 139, 255) }, // POWERHULL_COLOR_TETRA_WHITE
	{ 402, core::RGBA(61, 61, 61, 255) }, // POWERHULL_COLOR_TETRA_GREY
	{ 435, core::RGBA(113, 66, 5, 255) }, // POWERHULL_COLOR_TETRA_ORANGE
	{ 405, core::RGBA(43, 53, 63, 255) }, // ACTIVAION_BLOCK_ID
	{ 406, core::RGBA(53, 95, 134, 255) }, // SIGNAL_DELAY_FORWARD_ID
	{ 407, core::RGBA(53, 95, 134, 255) }, // SIGNAL_DELAY_BLOCK_ID
	{ 408, core::RGBA(55, 96, 134, 255) }, // SIGNAL_AND_BLOCK_ID
	{ 409, core::RGBA(56, 96, 135, 255) }, // SIGNAL_OR_BLOCK_ID
	{ 410, core::RGBA(55, 96, 135, 255) }, // SIGNAL_NOT_BLOCK_ID
	{ 411, core::RGBA(72, 71, 61, 50) }, // SIGNAL_TRIGGER_AREA
	{ 412, core::RGBA(52, 78, 103, 255) }, // SIGNAL_TRIGGER_STEPON
	{ 413, core::RGBA(46, 69, 94, 255) }, // SIGNAL_TRIGGER_AREA_CONTROLLER
	{ 414, core::RGBA(49, 50, 49, 255) }, // DAMAGE_BEAM_COMPUTER
	{ 415, core::RGBA(58, 126, 132, 255) }, // DAMAGE_BEAM_MODULE
	{ 416, core::RGBA(49, 50, 49, 255) }, // DAMAGE_PULSE_COMPUTER
	{ 417, core::RGBA(63, 62, 58, 255) }, // DAMAGE_PULSE_MODULE
	{ 418, core::RGBA(49, 50, 49, 255) }, // EFFECT_PIERCING_COMPUTER
	{ 419, core::RGBA(68, 93, 64, 255) }, // EFFECT_PIERCING_MODULE
	{ 420, core::RGBA(49, 50, 49, 255) }, // EFFECT_EXPLOSIVE_COMPUTER
	{ 421, core::RGBA(97, 82, 59, 255) }, // EFFECT_EXPLOSIVE_MODULE
	{ 422, core::RGBA(49, 49, 49, 255) }, // EFFECT_PUNCHTHROUGH_COMPUTER
	{ 423, core::RGBA(68, 60, 98, 255) }, // EFFECT_PUNCHTHROUGH_MODULE
	{ 424, core::RGBA(49, 49, 50, 255) }, // EFFECT_EMP_COMPUTER
	{ 425, core::RGBA(59, 94, 95, 255) }, // EFFECT_EMP_MODULE
	{ 436, core::RGBA(59, 52, 11, 255) }, // HULL_HAZARD_YELLOW
	{ 437, core::RGBA(59, 52, 11, 255) }, // HULL_HAZARD_WEDGE_YELLOW
	{ 438, core::RGBA(91, 106, 94, 255) }, // HULL_HAZARD_GREEN
	{ 439, core::RGBA(91, 106, 94, 255) }, // HULL_HAZARD_WEDGE_GREEN
	{ 440, core::RGBA(87, 87, 87, 255) }, // METAL_MESH
	{ 441, core::RGBA(87, 87, 87, 255) }, // METAL_MESH_WEDGE
	{ 442, core::RGBA(66, 66, 66, 255) }, // METAL_GRID
	{ 443, core::RGBA(66, 66, 66, 255) }, // METAL_GRD_WEDGE
	{ 444, core::RGBA(70, 180, 210, 255) }, // ICEPLANET_CRYSTAL_WEDGE
	{ 445, core::RGBA(127, 133, 131, 255) }, // MEDICAL_SUPPLIES
	{ 446, core::RGBA(144, 146, 146, 255) }, // MEDICAL_CABINET
	{ 447, core::RGBA(106, 64, 58, 255) }, // DECO_SCREEN_RED
	{ 448, core::RGBA(61, 103, 142, 255) }, // DECO_SCREEN_BLUE
	{ 449, core::RGBA(86, 133, 76, 255) }, // DECO_COMPUTER_GREEN
	{ 450, core::RGBA(137, 106, 67, 255) }, // DECO_COMPUTER_ORANGE
	{ 451, core::RGBA(49, 50, 49, 255) }, // DECO_PC_BLUE
	{ 452, core::RGBA(101, 49, 185, 255) }, // TERRAIN_CRYSTAL_PURPLE
	{ 453, core::RGBA(43, 31, 49, 255) }, // TERRAIN_CRYSTAL_BLACK
	{ 454, core::RGBA(151, 154, 159, 255) }, // TERRAIN_CRYSTAL_WHITE
	{ 455, core::RGBA(165, 144, 33, 255) }, // TERRAIN_CRYSTAL_YELLOW
	{ 456, core::RGBA(124, 35, 13, 255) }, // TERRAIN_CRYSTAL_RED
	{ 457, core::RGBA(169, 99, 14, 255) }, // TERRAIN_CRYSTAL_ORANGE
	{ 458, core::RGBA(18, 129, 49, 255) }, // TERRAIN_CRYSTAL_GREEN
	{ 459, core::RGBA(32, 119, 184, 255) }, // TERRAIN_CRYSTAL_BLUE
	{ 460, core::RGBA(50, 49, 49, 255) }, // EFFECT_STOP_COMPUTER
	{ 461, core::RGBA(99, 67, 62, 255) }, // EFFECT_STOP_MODULE
	{ 462, core::RGBA(49, 49, 49, 255) }, // EFFECT_PUSH_COMPUTER
	{ 463, core::RGBA(98, 61, 94, 255) }, // EFFECT_PUSH_MODULE
	{ 464, core::RGBA(50, 49, 49, 255) }, // EFFECT_PULL_COMPUTER
	{ 465, core::RGBA(97, 96, 61, 255) }, // EFFECT_PULL_MODULE
	{ 466, core::RGBA(49, 49, 50, 255) }, // EFFECT_ION_COMPUTER
	{ 467, core::RGBA(61, 85, 99, 255) }, // EFFECT_ION_MODULE
	{ 468, core::RGBA(95, 71, 124, 255) }, // INGOT_PURPLE
	{ 469, core::RGBA(18, 18, 18, 255) }, // INGOT_BLACK
	{ 470, core::RGBA(85, 85, 84, 255) }, // INGOT_WHITE
	{ 471, core::RGBA(159, 99, 8, 255) }, // INGOT_YELLOW
	{ 472, core::RGBA(148, 45, 20, 255) }, // INGOT_RED
	{ 473, core::RGBA(187, 81, 20, 255) }, // INGOT_ORANGE
	{ 474, core::RGBA(43, 83, 41, 255) }, // INGOT_GREEN
	{ 475, core::RGBA(32, 80, 146, 255) }, // INGOT_BLUE
	{ 476, core::RGBA(49, 50, 49, 255) }, // EFFECT_OVERDRIVE_COMPUTER
	{ 477, core::RGBA(61, 99, 77, 255) }, // EFFECT_OVERDRIVE_MODULE
	{ 478, core::RGBA(80, 74, 69, 255) }, // SHIELD_CAPACITY
	{ 479, core::RGBA(51, 51, 51, 255) }, // TEXT_BOX
	{ 480, core::RGBA(171, 154, 1, 255) }, // RESS_CRYS_HATTEL
	{ 481, core::RGBA(216, 139, 10, 255) }, // RESS_CRYS_SINTYR
	{ 482, core::RGBA(181, 54, 22, 255) }, // RESS_CRYS_MATTISE
	{ 483, core::RGBA(163, 80, 210, 255) }, // RESS_CRYS_RAMMET
	{ 484, core::RGBA(74, 172, 208, 255) }, // RESS_CRYS_VARAT
	{ 485, core::RGBA(76, 187, 114, 255) }, // RESS_CRYS_BASTYN
	{ 486, core::RGBA(204, 204, 199, 255) }, // RESS_CRYS_PARSEN
	{ 487, core::RGBA(78, 56, 89, 255) }, // RESS_CRYS_NOCX
	{ 488, core::RGBA(123, 113, 54, 255) }, // RESS_ORE_THRENS
	{ 489, core::RGBA(104, 60, 27, 255) }, // RESS_ORE_JISPER
	{ 490, core::RGBA(97, 34, 29, 255) }, // RESS_ORE_ZERCANER
	{ 491, core::RGBA(75, 55, 101, 255) }, // RESS_ORE_SERTISE
	{ 492, core::RGBA(39, 71, 98, 255) }, // RESS_ORE_HITAL
	{ 493, core::RGBA(63, 109, 48, 255) }, // RESS_ORE_FERTIKEEN
	{ 494, core::RGBA(151, 165, 155, 255) }, // RESS_ORE_PARSTUN
	{ 495, core::RGBA(51, 44, 49, 255) }, // RESS_ORE_NACHT
	{ 496, core::RGBA(98, 61, 130, 255) }, // PURPLE_LIGHT
	{ 497, core::RGBA(135, 93, 57, 255) }, // ORANGE_LIGHT
	{ 498, core::RGBA(57, 64, 70, 255) }, // BLACK_LIGHT
	{ 499, core::RGBA(114, 114, 114, 255) }, // WHITE_POLE_LIGHT
	{ 500, core::RGBA(38, 33, 43, 255) }, // BLACK_POLE_LIGHT
	{ 501, core::RGBA(144, 60, 55, 255) }, // RED_POLE_LIGHT
	{ 502, core::RGBA(143, 96, 55, 255) }, // ORANGE_POLE_LIGHT
	{ 503, core::RGBA(114, 114, 114, 255) }, // YELLOW_POLE_LIGHT
	{ 504, core::RGBA(70, 140, 60, 255) }, // GREEN_POLE_LIGHT
	{ 505, core::RGBA(55, 116, 143, 255) }, // BLUE_POLE_LIGHT
	{ 506, core::RGBA(77, 54, 144, 255) }, // PURPLE_POLE_LIGHT
	{ 507, core::RGBA(158, 158, 158, 55) }, // GLASS_BLOCK_WHITE
	{ 508, core::RGBA(158, 158, 158, 55) }, // GLASS_WEDGE_WHITE
	{ 509, core::RGBA(158, 158, 158, 55) }, // GLASS_CORNER_WHITE
	{ 510, core::RGBA(158, 158, 158, 55) }, // GLASS_PENTA_WHITE
	{ 511, core::RGBA(158, 158, 158, 55) }, // GLASS_TETRA_WHITE
	{ 512, core::RGBA(207, 111, 111, 99) }, // GLASS_BLOCK_RED
	{ 513, core::RGBA(207, 111, 111, 99) }, // GLASS_WEDGE_RED
	{ 514, core::RGBA(207, 111, 111, 99) }, // GLASS_CORNER_RED
	{ 515, core::RGBA(207, 111, 111, 99) }, // GLASS_PENTA_RED
	{ 516, core::RGBA(207, 111, 111, 99) }, // GLASS_TETRA_RED
	{ 517, core::RGBA(207, 150, 111, 99) }, // GLASS_BLOCK_ORANGE
	{ 518, core::RGBA(207, 150, 111, 99) }, // GLASS_WEDGE_ORANGE
	{ 519, core::RGBA(207, 150, 111, 99) }, // GLASS_CORNER_ORANGE
	{ 520, core::RGBA(207, 150, 111, 99) }, // GLASS_PENTA_ORANGE
	{ 521, core::RGBA(207, 150, 111, 99) }, // GLASS_TETRA_ORANGE
	{ 522, core::RGBA(207, 198, 111, 99) }, // GLASS_BLOCK_YELLOW
	{ 523, core::RGBA(207, 198, 111, 99) }, // GLASS_WEDGE_YELLOW
	{ 524, core::RGBA(207, 198, 111, 99) }, // GLASS_CORNER_YELLOW
	{ 525, core::RGBA(207, 198, 111, 99) }, // GLASS_PENTA_YELLOW
	{ 526, core::RGBA(207, 198, 111, 99) }, // GLASS_TETRA_YELLOW
	{ 527, core::RGBA(111, 207, 111, 99) }, // GLASS_BLOCK_GREEN
	{ 528, core::RGBA(111, 207, 111, 99) }, // GLASS_WEDGE_GREEN
	{ 529, core::RGBA(111, 207, 111, 99) }, // GLASS_CORNER_GREEN
	{ 530, core::RGBA(111, 207, 111, 99) }, // GLASS_PENTA_GREEN
	{ 531, core::RGBA(111, 207, 111, 99) }, // GLASS_TETRA_GREEN
	{ 532, core::RGBA(99, 172, 219, 99) }, // GLASS_BLOCK_BLUE
	{ 533, core::RGBA(99, 172, 219, 99) }, // GLASS_WEDGE_BLUE
	{ 534, core::RGBA(99, 172, 219, 99) }, // GLASS_CORNER_BLUE
	{ 535, core::RGBA(99, 172, 219, 99) }, // GLASS_PENTA_BLUE
	{ 536, core::RGBA(99, 172, 219, 99) }, // GLASS_TETRA_BLUE
	{ 537, core::RGBA(126, 111, 207, 99) }, // GLASS_BLOCK_PURPLE
	{ 538, core::RGBA(126, 111, 207, 99) }, // GLASS_WEDGE_PURPLE
	{ 539, core::RGBA(126, 111, 207, 99) }, // GLASS_CORNER_PURPLE
	{ 540, core::RGBA(126, 111, 207, 99) }, // GLASS_PENTA_PURPLE
	{ 541, core::RGBA(126, 111, 207, 99) }, // GLASS_TETRA_PURPLE
	{ 542, core::RGBA(58, 67, 68, 255) }, // WARP_GATE_CONTROLLER
	{ 543, core::RGBA(39, 39, 38, 255) }, // WARP_GATE_MODULE
	{ 544, core::RGBA(60, 65, 66, 255) }, // JUMP_DRIVE_CONTROLLER
	{ 545, core::RGBA(62, 62, 62, 255) }, // JUMP_DRIVE_MODULE
	{ 546, core::RGBA(87, 87, 87, 255) }, // Scrap_Alloy
	{ 547, core::RGBA(35, 35, 34, 166) }, // Scrap_Composite
	{ 548, core::RGBA(43, 31, 49, 255) }, // Nocx_Crystal_Wedge
	{ 549, core::RGBA(151, 154, 159, 255) }, // Parsen_Crystal_Wedge
	{ 550, core::RGBA(101, 49, 185, 255) }, // Rammet_Crystal_Wedge
	{ 551, core::RGBA(32, 119, 184, 255) }, // Varat_Crystal_Wedge
	{ 552, core::RGBA(18, 129, 49, 255) }, // Bastyn_Crystal_Wedge
	{ 553, core::RGBA(165, 144, 33, 255) }, // Hattel_Crystal_Wedge
	{ 554, core::RGBA(169, 99, 14, 255) }, // Sintyr_Crystal_Wedge
	{ 555, core::RGBA(124, 35, 13, 255) }, // Mattise_Crystal_Wedge
	{ 556, core::RGBA(18, 18, 18, 255) }, // Nacht_Ingot_Wedge
	{ 557, core::RGBA(85, 85, 84, 255) }, // Parstun_Ingot_Wedge
	{ 558, core::RGBA(95, 71, 124, 255) }, // Sertise_Ingot_Wedge
	{ 559, core::RGBA(32, 80, 146, 255) }, // Hittal_Ingot_Wedge
	{ 560, core::RGBA(43, 83, 41, 255) }, // Fertikeen_Ingot_Wedge
	{ 561, core::RGBA(159, 99, 8, 255) }, // Threns_Ingot_Wedge
	{ 562, core::RGBA(187, 81, 20, 255) }, // Jisper_Ingot_Wedge
	{ 563, core::RGBA(148, 45, 20, 255) }, // Zercaner_Ingot_Wedge
	{ 564, core::RGBA(17, 17, 17, 154) }, // Nocx_Circuit_Wedge
	{ 565, core::RGBA(60, 60, 60, 154) }, // Parsen_Circuit_Wedge
	{ 566, core::RGBA(49, 19, 53, 154) }, // Rammet_Circuit_Wedge
	{ 567, core::RGBA(6, 49, 66, 154) }, // Varat_Circuit_Wedge
	{ 568, core::RGBA(8, 63, 28, 154) }, // Bastyn_Circuit_Wedge
	{ 569, core::RGBA(50, 57, 3, 154) }, // Hattel_Circuit_Wedge
	{ 570, core::RGBA(58, 40, 6, 154) }, // Sintyr_Circuit_Wedge
	{ 571, core::RGBA(66, 11, 9, 154) }, // Mattise_Circuit_Wedge
	{ 572, core::RGBA(50, 50, 50, 255) }, // Nacht_Motherboard_Wedge
	{ 573, core::RGBA(73, 73, 73, 255) }, // Parstun_Motherboard_Wedge
	{ 574, core::RGBA(65, 54, 70, 255) }, // Sertise_Motherboard_Wedge
	{ 575, core::RGBA(50, 69, 77, 255) }, // Hittal_Motherboard_Wedge
	{ 576, core::RGBA(49, 77, 54, 255) }, // Fertikeen_Motherboard_Wedge
	{ 577, core::RGBA(77, 75, 49, 255) }, // Threns_Motherboard_Wedge
	{ 578, core::RGBA(77, 66, 49, 255) }, // Jisper_Motherboard_Wedge
	{ 579, core::RGBA(74, 50, 50, 255) }, // Zercaner_Motherboard_Wedge
	{ 580, core::RGBA(28, 28, 28, 165) }, // Nocx_Charged_Circuit_Wedge
	{ 581, core::RGBA(220, 210, 205, 155) }, // Parsen_Charged_Circuit_Wedge
	{ 582, core::RGBA(35, 12, 62, 165) }, // Rammet_Charged_Circuit_Wedge
	{ 583, core::RGBA(6, 38, 50, 165) }, // Varat_Charged_Circuit_Wedge
	{ 584, core::RGBA(6, 50, 16, 165) }, // Bastyn_Charged_Circuit_Wedge
	{ 585, core::RGBA(63, 63, 7, 128) }, // Hattel_Charged_Circuit_Wedge
	{ 586, core::RGBA(54, 37, 3, 165) }, // Sintyr_Charged_Circuit_Wedge
	{ 587, core::RGBA(50, 17, 7, 165) }, // Mattise_Charged_Circuit_Wedge
	{ 588, core::RGBA(28, 27, 27, 255) }, // Plex_Door_Wedge
	{ 589, core::RGBA(99, 171, 219, 56) }, // Glass_Door
	{ 590, core::RGBA(99, 171, 219, 56) }, // Glass_Door_Wedge
	{ 591, core::RGBA(66, 66, 66, 255) }, // Blast_Door
	{ 592, core::RGBA(66, 66, 66, 255) }, // Blast_Door_Wedge
	{ 593, core::RGBA(95, 95, 95, 133) }, // Black_Crystal_Armor
	{ 594, core::RGBA(95, 95, 95, 133) }, // Black_Crystal_Armor_Wedge
	{ 595, core::RGBA(95, 95, 95, 133) }, // Black_Crystal_Armor_Corner
	{ 596, core::RGBA(95, 95, 95, 133) }, // Black_Crystal_Armor_Penta
	{ 597, core::RGBA(95, 95, 95, 133) }, // Black_Crystal_Armor_Tetra
	{ 598, core::RGBA(83, 83, 83, 255) }, // Grey_Hull
	{ 599, core::RGBA(83, 83, 83, 255) }, // Grey_Hull_Wedge
	{ 600, core::RGBA(83, 83, 83, 255) }, // Grey_Hull_Corner
	{ 601, core::RGBA(83, 83, 83, 255) }, // Grey_Hull_Penta
	{ 602, core::RGBA(83, 83, 83, 255) }, // Grey_Hull_Tetra
	{ 603, core::RGBA(18, 18, 18, 255) }, // Black_Hull
	{ 604, core::RGBA(18, 18, 18, 255) }, // Black_Hull_Wedge
	{ 605, core::RGBA(18, 18, 18, 255) }, // Black_Hull_Corner
	{ 606, core::RGBA(18, 18, 18, 255) }, // Black_Hull_Penta
	{ 607, core::RGBA(18, 18, 18, 255) }, // Black_Hull_Tetra
	{ 608, core::RGBA(172, 172, 172, 255) }, // White_Hull
	{ 609, core::RGBA(172, 172, 172, 255) }, // White_Hull_Wedge
	{ 610, core::RGBA(172, 172, 172, 255) }, // White_Hull_Corner
	{ 611, core::RGBA(172, 172, 172, 255) }, // White_Hull_Penta
	{ 612, core::RGBA(172, 172, 172, 255) }, // White_Hull_Tetra
	{ 613, core::RGBA(74, 43, 92, 255) }, // Purple_Hull
	{ 614, core::RGBA(74, 43, 92, 255) }, // Purple_Hull_Wedge
	{ 615, core::RGBA(74, 43, 92, 255) }, // Purple_Hull_Corner
	{ 616, core::RGBA(74, 43, 92, 255) }, // Purple_Hull_Penta
	{ 617, core::RGBA(74, 43, 92, 255) }, // Purple_Hull_Tetra
	{ 618, core::RGBA(25, 79, 97, 255) }, // Blue_Hull
	{ 619, core::RGBA(25, 79, 97, 255) }, // Blue_Hull_Wedge
	{ 620, core::RGBA(25, 79, 97, 255) }, // Blue_Hull_Corner
	{ 621, core::RGBA(25, 79, 97, 255) }, // Blue_Hull_Penta
	{ 622, core::RGBA(25, 79, 97, 255) }, // Blue_Hull_Tetra
	{ 623, core::RGBA(31, 96, 55, 255) }, // Green_Hull
	{ 624, core::RGBA(31, 96, 55, 255) }, // Green_Hull_Wedge
	{ 625, core::RGBA(31, 96, 55, 255) }, // Green_Hull_Corner
	{ 626, core::RGBA(31, 96, 55, 255) }, // Green_Hull_Penta
	{ 627, core::RGBA(31, 96, 55, 255) }, // Green_Hull_Tetra
	{ 628, core::RGBA(147, 124, 19, 255) }, // Yellow_Hull
	{ 629, core::RGBA(147, 124, 19, 255) }, // Yellow_Hull_Wedge
	{ 630, core::RGBA(147, 124, 19, 255) }, // Yellow_Hull_Corner
	{ 631, core::RGBA(147, 124, 19, 255) }, // Yellow_Hull_Penta
	{ 632, core::RGBA(147, 124, 19, 255) }, // Yellow_Hull_Tetra
	{ 633, core::RGBA(157, 83, 6, 255) }, // Orange_Hull
	{ 634, core::RGBA(157, 83, 6, 255) }, // Orange_Hull_Wedge
	{ 635, core::RGBA(157, 83, 6, 255) }, // Orange_Hull_Corner
	{ 636, core::RGBA(157, 83, 6, 255) }, // Orange_Hull_Penta
	{ 637, core::RGBA(157, 83, 6, 255) }, // Orange_Hull_Tetra
	{ 638, core::RGBA(118, 36, 29, 255) }, // Red_Hull
	{ 639, core::RGBA(118, 36, 29, 255) }, // Red_Hull_Wedge
	{ 640, core::RGBA(118, 36, 29, 255) }, // Red_Hull_Corner
	{ 641, core::RGBA(118, 36, 29, 255) }, // Red_Hull_Penta
	{ 642, core::RGBA(118, 36, 29, 255) }, // Red_Hull_Tetra
	{ 643, core::RGBA(69, 51, 37, 255) }, // Brown_Hull
	{ 644, core::RGBA(69, 51, 37, 255) }, // Brown_Hull_Wedge
	{ 645, core::RGBA(69, 51, 37, 255) }, // Brown_Hull_Corner
	{ 646, core::RGBA(69, 51, 37, 255) }, // Brown_Hull_Penta
	{ 647, core::RGBA(69, 51, 37, 255) }, // Brown_Hull_Tetra
	{ 648, core::RGBA(59, 52, 11, 255) }, // Yellow_Hazard_Armor_Corner
	{ 649, core::RGBA(59, 52, 11, 255) }, // Yellow_Hazard_Armor_Penta
	{ 650, core::RGBA(59, 52, 11, 255) }, // Yellow_Hazard_Armor_Tetra
	{ 651, core::RGBA(91, 106, 94, 255) }, // Green_Hazard_Armor_Corner
	{ 652, core::RGBA(91, 106, 94, 255) }, // Green_Hazard_Armor_Penta
	{ 653, core::RGBA(91, 106, 94, 255) }, // Green_Hazard_Armor_Tetra
	{ 654, core::RGBA(50, 50, 49, 255) }, // SCANNER_COMPUTER
	{ 655, core::RGBA(61, 61, 61, 255) }, // SCANNER_MODULE
};
static const BlockColor BLOCKEMITCOLOR[]{
	{ 55, core::RGBA(255, 255, 255, 255) }, // emit for LIGHT_ID
	{ 62, core::RGBA(255, 255, 255, 255) }, // emit for LIGHT_BEACON_ID
	{ 80, core::RGBA(255, 51, 51, 255) }, // emit for TERRAIN_LAVA_ID
	{ 129, core::RGBA(255, 102, 0, 13) }, // emit for TERRAIN_OCTOGEN_ID
	{ 134, core::RGBA(225, 225, 225, 13) }, // emit for TERRAIN_SUCCUMITE_ID
	{ 136, core::RGBA(83, 0, 218, 13) }, // emit for TERRAIN_AWESOMITE_ID
	{ 146, core::RGBA(255, 127, 0, 62) }, // emit for TERRAIN_M2L2_ID
	{ 166, core::RGBA(255, 255, 255, 62) }, // emit for TERRAIN_M7L2_ID
	{ 174, core::RGBA(255, 0, 255, 62) }, // emit for TERRAIN_M9L2_ID
	{ 182, core::RGBA(255, 255, 0, 62) }, // emit for TERRAIN_M11L2_ID
	{ 186, core::RGBA(0, 255, 0, 62) }, // emit for TERRAIN_M12L2_ID
	{ 190, core::RGBA(0, 127, 255, 62) }, // emit for TERRAIN_M13L2_ID
	{ 194, core::RGBA(255, 0, 0, 62) }, // emit for TERRAIN_M14L2_ID
	{ 198, core::RGBA(127, 0, 255, 62) }, // emit for TERRAIN_M15L2_ID
	{ 206, core::RGBA(255, 255, 110, 13) }, // emit for TERRAIN_NEGACIDE_ID
	{ 207, core::RGBA(0, 204, 51, 13) }, // emit for TERRAIN_QUANTACIDE_ID
	{ 208, core::RGBA(0, 91, 255, 13) }, // emit for TERRAIN_NEGAGATE_ID
	{ 209, core::RGBA(204, 0, 0, 13) }, // emit for TERRAIN_METATE_ID
	{ 210, core::RGBA(11, 0, 20, 13) }, // emit for TERRAIN_INSANIUM_ID
	{ 225, core::RGBA(0, 255, 0, 62) }, // emit for MAN_SD3000_CAP
	{ 228, core::RGBA(255, 0, 0, 62) }, // emit for MAN_SD3000_FLUX
	{ 231, core::RGBA(0, 127, 255, 62) }, // emit for MAN_SD3000_MICRO
	{ 234, core::RGBA(255, 255, 255, 62) }, // emit for MAN_SD3000_DELTA
	{ 237, core::RGBA(255, 255, 127, 62) }, // emit for MAN_SD3000_MEM
	{ 249, core::RGBA(255, 127, 0, 62) }, // emit for MAN_P3000B
	{ 252, core::RGBA(255, 0, 255, 62) }, // emit for MAN_P30000A
	{ 253, core::RGBA(255, 255, 255, 64) }, // emit for MAN_P40000A
	{ 273, core::RGBA(127, 0, 255, 62) }, // emit for MAN_SCIENCE_BOTTLE
	{ 282, core::RGBA(255, 0, 0, 255) }, // emit for LIGHT_RED
	{ 283, core::RGBA(0, 127, 255, 255) }, // emit for LIGHT_BLUE
	{ 284, core::RGBA(0, 255, 0, 255) }, // emit for LIGHT_GREEN
	{ 285, core::RGBA(255, 255, 0, 255) }, // emit for LIGHT_YELLOW
	{ 286, core::RGBA(0, 255, 255, 127) }, // emit for TERRAIN_ICEPLANET_CRYSTAL
	{ 331, core::RGBA(0, 5, 0, 255) }, // emit for POWER_HOLDER_ID
	{ 340, core::RGBA(255, 255, 0, 127) }, // emit for LIGHT_BULB_YELLOW
	{ 405, core::RGBA(76, 76, 204, 25) }, // emit for ACTIVAION_BLOCK_ID
	{ 408, core::RGBA(76, 76, 204, 25) }, // emit for SIGNAL_AND_BLOCK_ID
	{ 409, core::RGBA(76, 76, 204, 25) }, // emit for SIGNAL_OR_BLOCK_ID
	{ 410, core::RGBA(76, 76, 204, 25) }, // emit for SIGNAL_NOT_BLOCK_ID
	{ 444, core::RGBA(0, 202, 255, 255) }, // emit for ICEPLANET_CRYSTAL_WEDGE
	{ 452, core::RGBA(255, 0, 255, 127) }, // emit for TERRAIN_CRYSTAL_PURPLE
	{ 453, core::RGBA(127, 0, 255, 127) }, // emit for TERRAIN_CRYSTAL_BLACK
	{ 454, core::RGBA(255, 255, 255, 127) }, // emit for TERRAIN_CRYSTAL_WHITE
	{ 455, core::RGBA(255, 255, 0, 127) }, // emit for TERRAIN_CRYSTAL_YELLOW
	{ 456, core::RGBA(255, 0, 0, 127) }, // emit for TERRAIN_CRYSTAL_RED
	{ 457, core::RGBA(255, 127, 0, 127) }, // emit for TERRAIN_CRYSTAL_ORANGE
	{ 458, core::RGBA(0, 255, 0, 127) }, // emit for TERRAIN_CRYSTAL_GREEN
	{ 459, core::RGBA(0, 127, 255, 127) }, // emit for TERRAIN_CRYSTAL_BLUE
	{ 496, core::RGBA(255, 0, 255, 255) }, // emit for PURPLE_LIGHT
	{ 497, core::RGBA(255, 127, 0, 255) }, // emit for ORANGE_LIGHT
	{ 498, core::RGBA(127, 0, 255, 255) }, // emit for BLACK_LIGHT
	{ 499, core::RGBA(255, 255, 255, 127) }, // emit for WHITE_POLE_LIGHT
	{ 500, core::RGBA(127, 0, 255, 127) }, // emit for BLACK_POLE_LIGHT
	{ 501, core::RGBA(255, 0, 0, 127) }, // emit for RED_POLE_LIGHT
	{ 502, core::RGBA(255, 127, 0, 127) }, // emit for ORANGE_POLE_LIGHT
	{ 503, core::RGBA(255, 255, 255, 127) }, // emit for YELLOW_POLE_LIGHT
	{ 504, core::RGBA(0, 255, 0, 127) }, // emit for GREEN_POLE_LIGHT
	{ 505, core::RGBA(0, 127, 255, 127) }, // emit for BLUE_POLE_LIGHT
	{ 506, core::RGBA(255, 0, 255, 127) }, // emit for PURPLE_POLE_LIGHT
	{ 548, core::RGBA(127, 0, 255, 255) }, // emit for Nocx_Crystal_Wedge
	{ 549, core::RGBA(255, 255, 255, 127) }, // emit for Parsen_Crystal_Wedge
	{ 550, core::RGBA(255, 0, 255, 127) }, // emit for Rammet_Crystal_Wedge
	{ 551, core::RGBA(0, 127, 255, 127) }, // emit for Varat_Crystal_Wedge
	{ 552, core::RGBA(0, 255, 0, 127) }, // emit for Bastyn_Crystal_Wedge
	{ 553, core::RGBA(255, 255, 0, 127) }, // emit for Hattel_Crystal_Wedge
	{ 554, core::RGBA(255, 127, 0, 127) }, // emit for Sintyr_Crystal_Wedge
	{ 555, core::RGBA(255, 0, 0, 127) }, // emit for Mattise_Crystal_Wedge
	{ 580, core::RGBA(127, 0, 255, 62) }, // emit for Nocx_Charged_Circuit_Wedge
	{ 581, core::RGBA(255, 255, 255, 62) }, // emit for Parsen_Charged_Circuit_Wedge
	{ 582, core::RGBA(255, 0, 255, 62) }, // emit for Rammet_Charged_Circuit_Wedge
	{ 583, core::RGBA(0, 127, 255, 62) }, // emit for Varat_Charged_Circuit_Wedge
	{ 584, core::RGBA(0, 255, 0, 62) }, // emit for Bastyn_Charged_Circuit_Wedge
	{ 585, core::RGBA(255, 255, 0, 62) }, // emit for Hattel_Charged_Circuit_Wedge
	{ 586, core::RGBA(255, 127, 0, 62) }, // emit for Sintyr_Charged_Circuit_Wedge
	{ 587, core::RGBA(255, 0, 0, 62) }, // emit for Mattise_Charged_Circuit_Wedge
};
}; // namespace voxelformat
