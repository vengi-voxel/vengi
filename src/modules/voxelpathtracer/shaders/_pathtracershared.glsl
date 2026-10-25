#pragma constant BrickSize 8
#pragma constant MaxBounces 12
#pragma constant MaxSamplesThisPass 16
#pragma constant MaxSteps 8192
#pragma constant MaterialRows 4
// Brick voxel payload: bits 0-13 materialId (1..N), bit 14 transmits, bit 15 emit. Zero = empty.
#pragma constant VoxelMaterialIdMask 16383
#pragma constant VoxelFlagTransmits 16384
#pragma constant VoxelFlagEmit 32768

// Must match palette::MaterialType in src/modules/palette/Material.h
#pragma constant MaterialDiffuse 0
#pragma constant MaterialMetal 1
#pragma constant MaterialGlass 2
#pragma constant MaterialEmit 3
#pragma constant MaterialBlend 4
#pragma constant MaterialMedia 5
