#pragma once

#include <stdint.h> //C99 fixed size data types.

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the CUBIQUITYC_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// CUBIQUITYC_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#if defined _WIN32 || defined __CYGWIN__
#ifdef CUBIQUITYC_EXPORTS
#define CUBIQUITYC_API __declspec(dllexport)
#else
#define CUBIQUITYC_API __declspec(dllimport)
#endif
#else
#define CUBIQUITYC_API __attribute__ ((visibility("default")))
#endif

const int32_t CU_OK = 0;

// Standard exceptions, based on list here: http://www.cplusplus.com/reference/exception/exception/
const int32_t CU_EXCEPTION = 10;
const int32_t CU_BAD_ALLOC = 20;
const int32_t CU_BAD_CAST = 30;
const int32_t CU_BAD_EXCEPTION = 40;
const int32_t CU_BAD_FUNCTION_CALL = 50;
const int32_t CU_BAD_TYPEID = 60;
const int32_t CU_BAD_WEAK_PTR = 70;
//const int32_t ios_base::failure // Included below
const int32_t CU_LOGIC_ERROR = 80;
const int32_t CU_RUNTIME_ERROR = 90;

const int32_t CU_DOMAIN_ERROR = 100;
const int32_t CU_FUTURE_ERROR = 110;
const int32_t CU_INVALID_ARGUMENT = 120;
const int32_t CU_LENGTH_ERROR = 130;
const int32_t CU_OUT_OF_RANGE = 140;

const int32_t CU_OVERFLOW_ERROR = 150;
const int32_t CU_RANGE_ERROR = 160;
const int32_t CU_SYSTEM_ERROR = 170;
const int32_t CU_UNDERFLOW_ERROR = 180;

const int32_t CU_BAD_ARRAY_NEW_LENGTH = 190;

const int32_t CU_IOS_BASE_FAILURE = 200;

// Non-standard exceptions
const int32_t CU_DATABASE_ERROR = 210;
const int32_t CU_COMPRESSION_ERROR = 220;

// Unknown error (caught by ...)
const int32_t CU_UNKNOWN_ERROR = 1000;

// C version of Cubiquity::WritePermissions
// Fixme - should this be a C enum?
const uint32_t CU_READONLY = 0;
const uint32_t CU_READWRITE = 1;

// Fixme - should this be a C enum?
// Also, apped '_VOLUME_TYPE'?
const uint32_t CU_COLORED_CUBES = 0;
const uint32_t CU_TERRAIN = 1;
const uint32_t CU_UNKNOWN = 0xFFFFFFFF;

struct CuColor_s {
	uint32_t data;
};
typedef struct CuColor_s CuColor;

struct CuMaterialSet_s {
	uint64_t data;
};
typedef struct CuMaterialSet_s CuMaterialSet;

struct CuTerrainVertex_s {
public:
	uint16_t encodedPosX;
	uint16_t encodedPosY;
	uint16_t encodedPosZ;
	uint16_t encodedNormal;
	uint8_t material0;
	uint8_t material1;
	uint8_t material2;
	uint8_t material3;
	uint8_t material4;
	uint8_t material5;
	uint8_t material6;
	uint8_t material7;
};
typedef struct CuTerrainVertex_s CuTerrainVertex;

struct CuColoredCubesVertex_s {
public:
	uint8_t encodedPosX;
	uint8_t encodedPosY;
	uint8_t encodedPosZ;
	uint8_t dummy;
	uint32_t data;
};
typedef struct CuColoredCubesVertex_s CuColoredCubesVertex;

struct CuOctreeNode_s {
public:
	int32_t posX;
	int32_t posY;
	int32_t posZ;

	uint32_t structureLastChanged;
	uint32_t propertiesLastChanged;
	uint32_t meshLastChanged;
	uint32_t nodeOrChildrenLastChanged;

	uint32_t childHandles[2][2][2];

	uint8_t hasMesh;
	uint8_t renderThisNode;
	uint8_t height;
};
typedef struct CuOctreeNode_s CuOctreeNode;

// Version functions
CUBIQUITYC_API int32_t cuGetVersionNumber(uint32_t* majorVersion, uint32_t* minorVersion, uint32_t* patchVersion, uint32_t* buildVersion);

// Error handling functions - Don't return error code as they are too simple to fail.
CUBIQUITYC_API const char* cuGetErrorCodeAsString(int32_t errorCode);
CUBIQUITYC_API const char* cuGetLastErrorMessage(void);

// Color functions - these don't return error codes because they are too simple to fail.
CUBIQUITYC_API uint8_t cuGetRed(CuColor color);
CUBIQUITYC_API uint8_t cuGetGreen(CuColor color);
CUBIQUITYC_API uint8_t cuGetBlue(CuColor color);
CUBIQUITYC_API uint8_t cuGetAlpha(CuColor color);
CUBIQUITYC_API void cuGetAllComponents(CuColor color, uint8_t* red, uint8_t* green, uint8_t* blue, uint8_t* alpha);

CUBIQUITYC_API CuColor cuMakeColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha);

// Volume functions
CUBIQUITYC_API int32_t cuNewEmptyColoredCubesVolume(int32_t lowerX, int32_t lowerY, int32_t lowerZ, int32_t upperX, int32_t upperY, int32_t upperZ,
		const char* pathToNewVoxelDatabase, uint32_t baseNodeSize, uint32_t* result);
CUBIQUITYC_API int32_t cuNewColoredCubesVolumeFromVDB(const char* pathToExistingVoxelDatabase, uint32_t writePermissions, uint32_t baseNodeSize, uint32_t* result);
CUBIQUITYC_API int32_t cuUpdateVolume(uint32_t volumeHandle, float eyePosX, float eyePosY, float eyePosZ, float lodThreshold, uint32_t* isUpToDate);
CUBIQUITYC_API int32_t cuDeleteVolume(uint32_t volumeHandle);

CUBIQUITYC_API int32_t cuGetEnclosingRegion(uint32_t volumeHandle, int32_t* lowerX, int32_t* lowerY, int32_t* lowerZ, int32_t* upperX, int32_t* upperY, int32_t* upperZ);

CUBIQUITYC_API int32_t cuAcceptOverrideChunks(uint32_t volumeHandle);
CUBIQUITYC_API int32_t cuDiscardOverrideChunks(uint32_t volumeHandle);

CUBIQUITYC_API int32_t cuNewEmptyTerrainVolume(int32_t lowerX, int32_t lowerY, int32_t lowerZ, int32_t upperX, int32_t upperY, int32_t upperZ, const char* pathToNewVoxelDatabase,
		uint32_t baseNodeSize, uint32_t* result);
CUBIQUITYC_API int32_t cuNewTerrainVolumeFromVDB(const char* pathToExistingVoxelDatabase, uint32_t writePermissions, uint32_t baseNodeSize, uint32_t* result);

CUBIQUITYC_API int32_t cuGetVolumeType(uint32_t volumeHandle, uint32_t* result);
CUBIQUITYC_API uint32_t cuGetVolumeTypeRet(uint32_t volumeHandle);

// Voxel functions
CUBIQUITYC_API int32_t cuGetVoxel(uint32_t volumeHandle, int32_t x, int32_t y, int32_t z, void* result);
CUBIQUITYC_API int32_t cuSetVoxel(uint32_t volumeHandle, int32_t x, int32_t y, int32_t z, void* value);

// Octree functions
CUBIQUITYC_API int32_t cuHasRootOctreeNode(uint32_t volumeHandle, uint32_t* result);
CUBIQUITYC_API int32_t cuGetRootOctreeNode(uint32_t volumeHandle, uint32_t* result);
CUBIQUITYC_API int32_t cuGetOctreeNode(uint32_t nodeHandle, CuOctreeNode* result);

// Mesh functions
CUBIQUITYC_API int32_t cuSetLodRange(uint32_t volumeHandle, int32_t minimumLOD, int32_t maximumLOD);
CUBIQUITYC_API int32_t cuGetMesh(uint32_t nodeHandle, uint16_t* noOfVertices, void** vertices, uint32_t* noOfIndices, uint16_t** indices);

// Clock functions
CUBIQUITYC_API int32_t cuGetCurrentTime(uint32_t* result);

// Raycasting functions
CUBIQUITYC_API int32_t cuPickFirstSolidVoxel(uint32_t volumeHandle, float rayStartX, float rayStartY, float rayStartZ, float rayDirX, float rayDirY, float rayDirZ,
		int32_t* resultX, int32_t* resultY, int32_t* resultZ, uint32_t* result);
CUBIQUITYC_API int32_t cuPickLastEmptyVoxel(uint32_t volumeHandle, float rayStartX, float rayStartY, float rayStartZ, float rayDirX, float rayDirY, float rayDirZ, int32_t* resultX,
		int32_t* resultY, int32_t* resultZ, uint32_t* result);

CUBIQUITYC_API int32_t cuPickTerrainSurface(uint32_t volumeHandle, float rayStartX, float rayStartY, float rayStartZ, float rayDirX, float rayDirY, float rayDirZ, float* resultX,
		float* resultY, float* resultZ, uint32_t* result);

// Editing functions
CUBIQUITYC_API int32_t cuSculptTerrainVolume(uint32_t volumeHandle, float brushX, float brushY, float brushZ, float brushInnerRadius, float brushOuterRadius, float opacity);
CUBIQUITYC_API int32_t cuBlurTerrainVolume(uint32_t volumeHandle, float brushX, float brushY, float brushZ, float brushInnerRadius, float brushOuterRadius, float opacity);
CUBIQUITYC_API int32_t cuBlurTerrainVolumeRegion(uint32_t volumeHandle, int32_t lowerX, int32_t lowerY, int32_t lowerZ, int32_t upperX, int32_t upperY, int32_t upperZ);
CUBIQUITYC_API int32_t cuPaintTerrainVolume(uint32_t volumeHandle, float brushX, float brushY, float brushZ, float brushInnerRadius, float brushOuterRadius, float opacity,
		uint32_t materialIndex);

// Volume generation functions
CUBIQUITYC_API int32_t cuGenerateFloor(uint32_t volumeHandle, int32_t lowerLayerHeight, uint32_t lowerLayerMaterial, int32_t upperLayerHeight, uint32_t upperLayerMaterial);
