$constant BrickSize 8
$constant MaxBounces 12
$constant MaxSamplesThisPass 16
$constant MaxSteps 8192
$constant MaterialRows 4
// Brick voxel payload: bits 0-13 materialId (1..N), bit 14 transmits, bit 15 emit. Zero = empty.
$constant VoxelMaterialIdMask 16383
$constant VoxelFlagTransmits 16384
$constant VoxelFlagEmit 32768

// Must match palette::MaterialType in src/modules/palette/Material.h
$constant MaterialDiffuse 0
$constant MaterialMetal 1
$constant MaterialGlass 2
$constant MaterialEmit 3
$constant MaterialBlend 4
$constant MaterialMedia 5
