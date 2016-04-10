#pragma once

#include "CubiquityForwardDeclarations.h"
#include "CVector.h"

namespace Cubiquity {

void generateFloor(TerrainVolume* terrainVolume, int32_t lowerLayerHeight, uint32_t lowerLayerMaterial, int32_t upperLayerHeight, uint32_t upperLayerMaterial);

}
