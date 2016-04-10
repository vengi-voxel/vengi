#pragma once

#include "CubiquityForwardDeclarations.h"
#include "CRegion.h"
#include "CVector.h"

namespace Cubiquity {

void sculptTerrainVolume(TerrainVolume* terrainVolume, const Vector3F& centre, const Brush& brush);
void blurTerrainVolume(TerrainVolume* terrainVolume, const Vector3F& centre, const Brush& brush);
void blurTerrainVolume(TerrainVolume* terrainVolume, const Region& region);
void paintTerrainVolume(TerrainVolume* terrainVolume, const Vector3F& centre, const Brush& brush, uint32_t materialIndex);

}
