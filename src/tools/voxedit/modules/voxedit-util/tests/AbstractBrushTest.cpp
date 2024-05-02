/**
 * @file
 */

#include "AbstractBrushTest.h"

namespace voxedit {

INSTANTIATE_TEST_SUITE_P(PlaceAndOverride, BrushTestParamTest,
						 ::testing::Values(BrushCombination{voxel::FaceNames::PositiveX, ModifierType::Place},
										   BrushCombination{voxel::FaceNames::NegativeX, ModifierType::Place},
										   BrushCombination{voxel::FaceNames::PositiveY, ModifierType::Place},
										   BrushCombination{voxel::FaceNames::NegativeY, ModifierType::Place},
										   BrushCombination{voxel::FaceNames::PositiveZ, ModifierType::Place},
										   BrushCombination{voxel::FaceNames::NegativeZ, ModifierType::Place},
										   BrushCombination{voxel::FaceNames::PositiveX, ModifierType::Override},
										   BrushCombination{voxel::FaceNames::NegativeX, ModifierType::Override},
										   BrushCombination{voxel::FaceNames::PositiveY, ModifierType::Override},
										   BrushCombination{voxel::FaceNames::NegativeY, ModifierType::Override},
										   BrushCombination{voxel::FaceNames::PositiveZ, ModifierType::Override},
										   BrushCombination{voxel::FaceNames::NegativeZ, ModifierType::Override}));

} // namespace voxedit
