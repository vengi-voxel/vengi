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
#pragma once

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ENKI_MI_REGION_CHUNKS_NUMBER 1024

// http://web.archive.org/web/20110723210920/http://www.minecraft.net/docs/NBT.txt
typedef enum
{
	enkiNBTTAG_End = 0,
	enkiNBTTAG_Byte = 1,
	enkiNBTTAG_Short = 2,
	enkiNBTTAG_Int = 3,
	enkiNBTTAG_Long = 4,
	enkiNBTTAG_Float = 5,
	enkiNBTTAG_Double = 6,
	enkiNBTTAG_Byte_Array = 7,
	enkiNBTTAG_String = 8,
	enkiNBTTAG_List = 9,
	enkiNBTTAG_Compound = 10,
	enkiNBTTAG_Int_Array = 11,
	enkiNBTTAG_Long_Array = 12,
	enkiNBTTAG_SIZE,
} enkiNBTTAG_ID;


typedef struct enkiNBTTagHeader_s
{
	char* pName;

	// if the tag is a list, we need the following variables
	int32_t listNumItems;
	int32_t listCurrItem;
	uint8_t listItemTagId;

	// the tagId of type enkiNBTTAG_ID
	uint8_t tagId;
} enkiNBTTagHeader;

// Get enkiNBTTAG_ID as string
const char* enkiGetNBTTagIDAsString( uint8_t tagID_ );

// Shorthand for enkiGetNBTTagIDAsString( tagID_.tagId );
const char* enkiGetNBTTagHeaderIDAsString( enkiNBTTagHeader tagID_ );

typedef struct enkiNBTAllocation_s
{
	void*                       pAllocation;
	struct enkiNBTAllocation_s* pNext;
} enkiNBTAllocation;

typedef struct enkiNBTDataStream_s
{
	enkiNBTTagHeader parentTags[ 512 ];
	enkiNBTTagHeader currentTag;
	uint8_t* pCurrPos;
	uint8_t* pDataEnd;
	uint8_t* pData;
	uint8_t* pNextTag;
	enkiNBTAllocation* pAllocations;
	uint32_t dataLength;
	int32_t  level;
} enkiNBTDataStream;

// Initialize stream from memory pointer.
// pData_ and it's contents should remain valid until
// after enkiNBTDataStream no longer needed.
// Contents of buffer will be modified for easier reading,
// namely tag name strings will moved down a byte, null terminated,
// and prefixed with 0xFF instead of int16_t string length.
// Make a copy if you need to use the buffer in another lib.
// Other strings in file will not be altered.
// pUnCompressedData_ should be freed by caller.
// FreeMemoryAllocated() should still be called to free any internal allocations.
void enkiNBTInitFromMemoryUncompressed( enkiNBTDataStream* pStream_, uint8_t* pUnCompressedData_, uint32_t dataSize_ );

// Initialize stream from memory pointer to compressed content.
// This function will allocate space for uncompressed stream and decompress it with zlib.
// If uncompressedSizeHint_ > compressedDataSize_ it will be used as the starting hint size for allocating
// the uncompressed size.
// returns 1 if successfull, 0 if not.
int enkiNBTInitFromMemoryCompressed( enkiNBTDataStream* pStream_, uint8_t* pCompressedData_,
									    uint32_t compressedDataSize_, uint32_t uncompressedSizeHint_ );


// returns 0 if no next tag, 1 if there was
int enkiNBTReadNextTag( enkiNBTDataStream* pStream_ );


// Rewind stream so it can be read again from beginning
void enkiNBTRewind( enkiNBTDataStream* pStream_ );

// Frees any internally allocated memory.
void enkiNBTFreeAllocations( enkiNBTDataStream* pStream_ );

int8_t  enkiNBTReadInt8(   enkiNBTDataStream* pStream_ );
int8_t  enkiNBTReadByte(   enkiNBTDataStream* pStream_ );
int16_t enkiNBTReadInt16(  enkiNBTDataStream* pStream_ );
int16_t enkiNBTReadShort(  enkiNBTDataStream* pStream_ );
int32_t enkiNBTReadInt32(  enkiNBTDataStream* pStream_ );
int32_t enkiNBTReadInt(    enkiNBTDataStream* pStream_ );
float   enkiNBTReadFloat(  enkiNBTDataStream* pStream_ );
int64_t enkiNBTReadInt64(  enkiNBTDataStream* pStream_ );
int64_t enkiNBTReadlong(   enkiNBTDataStream* pStream_ );
double  enkiNBTReadDouble( enkiNBTDataStream* pStream_ );

typedef struct enkiNBTString_s
{
	uint16_t    size;
	const char* pStrNotNullTerminated;
} enkiNBTString;

enkiNBTString enkiNBTReadString( enkiNBTDataStream* pStream_ );

typedef struct enkiRegionFile_s
{
	uint8_t* pRegionData;
	uint32_t regionDataSize;
} enkiRegionFile;

// enkiRegionFileInit simply zeros data
void enkiRegionFileInit( enkiRegionFile* pRegionFile_ );

enkiRegionFile enkiRegionFileLoad( FILE* fp_ );


// 1 for a chunk exists, 0 for does not.
uint8_t enkiHasChunk( enkiRegionFile regionFile_, int32_t chunkNr_ );

void enkiInitNBTDataStreamForChunk( enkiRegionFile regionFile_, int32_t chunkNr_, enkiNBTDataStream* pStream_ );

int32_t enkiGetTimestampForChunk( enkiRegionFile regionFile_, int32_t chunkNr_ );

// enkiFreeRegionFileData frees data allocated in enkiRegionFile
void enkiRegionFileFreeAllocations( enkiRegionFile* pRegionFile_ );

// Check if lhs_ and rhs_ are equal, return 1 if so, 0 if not.
// Safe to pass in NULL for either
// Note that both NULL gives 0.
int enkiAreStringsEqual( const char* lhs_, const char* rhs_ );

// World height changes (1.17 21w06a) increase num sections to a potential 256 (-128 to 127 as Y uses signed byte)
#define ENKI_MI_NUM_SECTIONS_PER_CHUNK 256
#define ENKI_MI_SECTIONS_Y_OFFSET 128
#define ENKI_MI_SIZE_SECTIONS 16

typedef struct enkiMICoordinate_s
{
	int32_t x;
	int32_t y; // height
	int32_t z;
} enkiMICoordinate;

typedef struct enkiMINamespaceAndBlockID_s
{
	const char* pNamespaceID; // e.g. "minecraft:stone"
	uint8_t     blockID;      // block ID returned by enkiGetChunkSectionVoxel and enkiGetChunkSectionVoxelData
	uint8_t     dataValue;    // dataValue returned by enkiGetChunkSectionVoxelData
} enkiMINamespaceAndBlockID;

typedef struct enkiMIProperty_s {
	char*         pName;
	enkiNBTString value;
} enkiMIProperty;

// ENKI_MI_MAX_PROPERTIES can be modified but 6 appears to be the maximum
#ifndef ENKI_MI_MAX_PROPERTIES
	#define ENKI_MI_MAX_PROPERTIES 6
#endif

typedef struct enkiMIProperties_s {
	uint32_t        size;      // capped to ENKI_MI_MAX_PROPERTIES
	enkiMIProperty  properties[ ENKI_MI_MAX_PROPERTIES ];
} enkiMIProperties;

typedef struct enkiChunkSectionPalette_s
{
	uint32_t       size;
	uint32_t       numBitsPerBlock;
	uint32_t       blockArraySize;
	int32_t*       pDefaultBlockIndex;  // lookup index into the default enkiMINamespaceAndBlockIDTable - these values may change with versions of enkiMI, <0 means not found
	enkiNBTString* pNamespaceIDStrings; // e.g. "minecraft:stone"
	enkiMIProperties* pBlockStateProperties; // pointer to start of stream properties
} enkiChunkSectionPalette;

typedef struct enkiChunkBlockData_s
{
	uint8_t* sections[ ENKI_MI_NUM_SECTIONS_PER_CHUNK ];
	uint8_t* dataValues[ ENKI_MI_NUM_SECTIONS_PER_CHUNK ];
	enkiChunkSectionPalette palette[ ENKI_MI_NUM_SECTIONS_PER_CHUNK ]; // if there is a palette[k].size, then sections[k] represents BlockStates
	int32_t xPos; // section coordinates
	int32_t zPos; // section coordinates
	int32_t countOfSections;
	int32_t dataVersion;
} enkiChunkBlockData;

// enkiChunkInit simply zeros data
void enkiChunkInit( enkiChunkBlockData* pChunk_ );

// enkiNBTReadChunk gets a chunk from an enkiNBTDataStream
// pStream_ mush be kept valid whilst chunk is in use.
enkiChunkBlockData enkiNBTReadChunk( enkiNBTDataStream* pStream_ );

// Extended parameters for enkiNBTReadChunkEx:
typedef enum
{
	enkiNBTReadChunkExFlags_None = 0,
	enkiNBTReadChunkExFlags_NoPaletteTranslation = 1 << 0, // when loading palette do not translate namespace strings to blockID & dataValue - faster if you want to do your own translation / conversion to internal data
} enkiNBTReadChunkExFlags;

typedef struct enkiNBTReadChunkExParams_s
{
	int32_t flags; // enkiNBTReadChunkExFlags defaults to enkiNBTReadChunkExFlags_None
} enkiNBTReadChunkExParams;

// call enkiGetDefaultNBTReadChunkExParams to set up default parameters - essential to maintain forwards compatibilty if new members are added to enkiNBTReadChunkExParams
enkiNBTReadChunkExParams enkiGetDefaultNBTReadChunkExParams();

// enkiNBTReadChunkEx is as enkiNBTReadChunk but with extended parameters.
enkiChunkBlockData enkiNBTReadChunkEx( enkiNBTDataStream* pStream_, enkiNBTReadChunkExParams params_ );


enkiMICoordinate enkiGetChunkOrigin( enkiChunkBlockData* pChunk_ );

// get the origin of a section (0 <-> ENKI_MI_NUM_SECTIONS_PER_CHUNK).
enkiMICoordinate enkiGetChunkSectionOrigin( enkiChunkBlockData* pChunk_, int32_t section_ );

// sectionOffset_ is the position from enkiGetChunkSectionOrigin
// Performs no safety checks.
// check pChunk_->sections[ section_ ] for NULL first in your code.
// and ensure sectionOffset_ coords with 0 to ENKI_MI_SIZE_SECTIONS
uint8_t enkiGetChunkSectionVoxel( enkiChunkBlockData* pChunk_, int32_t section_, enkiMICoordinate sectionOffset_ );

uint32_t* enkiGetMineCraftPalette(); //returns a 256 array of uint32_t's in uint8_t rgba order.

typedef struct enkiMIVoxelData_s {

	uint8_t        blockID;     // pre-flattening blockIDs values, as returned by enkiGetChunkSectionVoxel(), can use to index into enkiGetMineCraftPalette
	uint8_t        dataValue;   // pre-flattening data values, blockId::dataValue identifies block varients
	int32_t        paletteIndex; // if >=0 index into enkiChunkBlockData.palette[section].pDefaultBlockIndex and enkiChunkBlockData.palette[section].pNamespaceIDStrings
} enkiMIVoxelData;

enkiMIVoxelData enkiGetChunkSectionVoxelData( enkiChunkBlockData* pChunk_, int32_t section_, enkiMICoordinate sectionOffset_ );

typedef struct enkiMINamespaceAndBlockIDTable_s {

	uint32_t                   size;
	enkiMINamespaceAndBlockID* namespaceAndBlockIDs;
} enkiMINamespaceAndBlockIDTable;

enkiMINamespaceAndBlockIDTable enkiGetNamespaceAndBlockIDTable();

#ifdef __cplusplus
};
#endif
