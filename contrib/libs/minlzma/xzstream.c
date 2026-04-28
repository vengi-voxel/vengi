/*++

Copyright (c) Alex Ionescu.  All rights reserved.

Module Name:

    xzstream.c

Abstract:

    This module implements the XZ stream format decoding, including support for
    parsing the stream header and block header, and then handing off the block
    decoding to the LZMA2 decoder. Finally, if "meta checking" is enabled, then
    the index and stream footer are also parsed and validated. Optionally, each
    of these component structures can be checked against its CRC32 checksum, if
    "integrity checking" has been enabled. Note that this library only supports
    single-stream, single-block XZ files that have CRC32 (or None) set as their
    block checking algorithm. Finally, no BJC filters are supported, and files
    with a compressed/uncompressed size metadata indicator are not handled.

Author:

    Alex Ionescu (@aionescu) 15-Apr-2020 - Initial version

Environment:

    Windows & Linux, user mode and kernel mode.

--*/

#include "minlzlib.h"
#include "xzstream.h"

#ifdef _WIN32
void __security_check_cookie(_In_ uintptr_t _StackCookie) { (void)(_StackCookie); }
#endif

#ifdef MINLZ_META_CHECKS
//
// XZ Stream Container State
//
typedef struct _CONTAINER_STATE
{
    //
    // Size of the XZ header and the index, used to validate against footer
    //
    uint32_t HeaderSize;
    uint32_t IndexSize;
    //
    // Size of the uncompressed block
    //
    uint32_t UncompressedBlockSize;
    uint32_t UnpaddedBlockSize;
    //
    // Checksum data
    //
    uint32_t ChecksumSize;
    uint8_t ChecksumType;
    bool ChecksumError;
} CONTAINER_STATE, * PCONTAINER_STATE;
CONTAINER_STATE Container;
#endif

#ifdef MINLZ_META_CHECKS
bool
XzDecodeVli (
    vli_type* Vli
    )
{
    uint8_t vliByte;
    uint32_t bitPos;

    //
    // Read the initial VLI byte (might be the value itself)
    //
    if (!BfRead(&vliByte))
    {
        return false;
    }
    *Vli = vliByte & 0x7F;

    //
    // Check if this was a complex VLI (and we have space for it)
    //
    bitPos = 7;
    while ((vliByte & 0x80) != 0)
    {
        //
        // Read the next byte
        //
        if (!BfRead(&vliByte))
        {
            return false;
        }

        //
        // Make sure we're not decoding an invalid VLI
        //
        if ((bitPos == (7 * VLI_BYTES_MAX)) || (vliByte == 0))
        {
            return false;
        }

        //
        // Decode it and move to the next 7 bits
        //
        *Vli |= (vli_type)((vliByte & 0x7F) << bitPos);
        bitPos += 7;
    }
    return true;
}

bool
XzDecodeIndex (
    void
    )
{
    uint32_t vli;
    const uint8_t* indexStart;
    const uint8_t* indexEnd;
    const uint32_t* pCrc32;
    uint8_t indexByte;

    //
    // Remember where the index started so we can compute its size
    //
    BfSeek(0, &indexStart);

    //
    // The index always starts out with an empty byte
    //
    if (!BfRead(&indexByte) || (indexByte != 0))
    {
        return false;
    }

    //
    // Then the count of blocks, which we expect to be 1
    //
    if (!XzDecodeVli(&vli) || (vli != 1))
    {
        return false;
    }

    //
    // Then the unpadded block size, which should match
    //
    if (!XzDecodeVli(&vli) || (Container.UnpaddedBlockSize != vli))
    {
        return false;
    }

    //
    // Then the uncompressed block size, which should match
    //
    if (!XzDecodeVli(&vli) || (Container.UncompressedBlockSize != vli))
    {
        return false;
    }

    //
    // Then we pad to the next multiple of 4
    //
    if (!BfAlign())
    {
        return false;
    }

    //
    // Store the index size with padding to validate the footer later
    //
    BfSeek(0, &indexEnd);
    Container.IndexSize = (uint32_t)(indexEnd - indexStart);

    //
    // Read the CRC32, which is not part of the index size
    //
    if (!BfSeek(sizeof(*pCrc32), (const uint8_t**)&pCrc32))
    {
        return false;
    }
#ifdef MINLZ_INTEGRITY_CHECKS
    //
    // Make sure the index is not corrupt
    //
    if (Crc32(indexStart, Container.IndexSize) != *pCrc32)
    {
        Container.ChecksumError = true;
    }
#endif
    return true;
}

bool
XzDecodeStreamFooter (
    void
    )
{
    PXZ_STREAM_FOOTER streamFooter;

    //
    // Seek past the footer, making sure we have space in the input stream
    //
    if (!BfSeek(sizeof(*streamFooter), (const uint8_t**)&streamFooter))
    {
        return false;
    }

    //
    // Validate the footer magic
    //
    if (streamFooter->Magic != k_XzStreamFooterMagic)
    {
        return false;
    }

    //
    // Validate no flags other than checksum type are set
    //
    if ((streamFooter->u.s.ReservedFlags != 0) ||
        (streamFooter->u.s.ReservedType != 0) ||
        (streamFooter->u.s.CheckType != Container.ChecksumType))
    {
        return false;
    }

    //
    // Validate if the footer accurately describes the size of the index
    //
    if (Container.IndexSize != (streamFooter->BackwardSize * 4))
    {
        return false;
    }
#ifdef MINLZ_INTEGRITY_CHECKS
    //
    // Compute the footer's CRC32 and make sure it's not corrupted
    //
    if (Crc32(&streamFooter->BackwardSize,
              sizeof(streamFooter->BackwardSize) +
              sizeof(streamFooter->u.Flags)) !=
        streamFooter->Crc32)
    {
        Container.ChecksumError = true;
    }
#endif
    return true;
}
#endif

#if MINLZ_INTEGRITY_CHECKS
bool
XzCrc (
    uint8_t* OutputBuffer,
    uint32_t BlockSize,
    const uint8_t* InputEnd
    )
{
    //
    // Compute the appropriate checksum and compare it with the expected result
    //
    switch (Container.ChecksumType)
    {
    case XzCheckTypeCrc32:
        return Crc32(OutputBuffer, BlockSize) != *(uint32_t*)InputEnd;
    case XzCheckTypeCrc64:
        return Crc64(OutputBuffer, BlockSize) != *(uint64_t*)InputEnd;
    default:
        return false;
    }
}
#endif

bool
XzDecodeBlock (
    uint8_t* OutputBuffer,
    uint32_t* BlockSize
    )
{
#ifdef MINLZ_META_CHECKS
    const uint8_t *inputStart, *inputEnd;
#endif
    //
    // Decode the LZMA2 stream. If full integrity checking is enabled, also
    // save the offset before and after decoding, so we can save the block
    // sizes and compare them against the footer and index after decoding.
    //
#ifdef MINLZ_META_CHECKS
    BfSeek(0, &inputStart);
#endif
    if (!Lz2DecodeStream(BlockSize, OutputBuffer == NULL))
    {
        return false;
    }
#ifdef MINLZ_META_CHECKS
    BfSeek(0, &inputEnd);
    Container.UnpaddedBlockSize = Container.HeaderSize +
                                  (uint32_t)(inputEnd - inputStart);
    Container.UncompressedBlockSize = *BlockSize;
#endif
    //
    // After the block data, we need to pad to 32-bit alignment
    //
    if (!BfAlign())
    {
        return false;
    }
#ifdef MINLZ_META_CHECKS
    //
    // Finally, move past the size of the checksum if any, then compare it with
    // with the actual checksum of the block, if integrity checks are enabled.
    // If meta checks are enabled, update the block size so the index checking
    // can validate it.
    //
    if (!BfSeek(Container.ChecksumSize, &inputEnd))
    {
        return false;
    }
#endif
    (void)(OutputBuffer);
#ifdef MINLZ_INTEGRITY_CHECKS
    if ((OutputBuffer != NULL) && !(XzCrc(OutputBuffer, *BlockSize, inputEnd)))
    {
        Container.ChecksumError = true;
    }
#endif
#ifdef MINLZ_META_CHECKS
    Container.UnpaddedBlockSize += Container.ChecksumSize;
#endif
    return true;
}

bool
XzDecodeStreamHeader (
    void
    )
{
    PXZ_STREAM_HEADER streamHeader;

    //
    // Seek past the header, making sure we have space in the input stream
    //
    if (!BfSeek(sizeof(*streamHeader), (const uint8_t**)&streamHeader))
    {
        return false;
    }
#ifdef MINLZ_META_CHECKS
    //
    // Validate the header magic
    //
    if ((*(uint32_t*)&streamHeader->Magic[1] != k_XzStreamHeaderMagic1) ||
        (streamHeader->Magic[0] != k_XzStreamHeaderMagic0) ||
        (streamHeader->Magic[5] != k_XzStreamHeaderMagic5))
    {
        return false;
    }

    //
    // Validate the header flags
    //
    if ((streamHeader->u.s.ReservedFlags != 0) ||
        (streamHeader->u.s.ReservedType != 0))
    {
        return false;
    }

    //
    // Save checksum type and compute pre-defined size for it
    //
    Container.ChecksumType = streamHeader->u.s.CheckType;
    Container.ChecksumSize = k_XzBlockCheckSizes[streamHeader->u.s.CheckType];
    if ((Container.ChecksumType != XzCheckTypeNone) &&
        (Container.ChecksumType != XzCheckTypeCrc32) &&
        (Container.ChecksumType != XzCheckTypeCrc64))
    {
        Container.ChecksumError = true;
    }
#endif
#ifdef MINLZ_INTEGRITY_CHECKS
    //
    // Compute the header's CRC32 and make sure it's not corrupted
    //
    if (Crc32(&streamHeader->u.Flags, sizeof(streamHeader->u.Flags)) !=
        streamHeader->Crc32)
    {
        Container.ChecksumError = true;
    }
#endif
    return true;
}

bool
XzDecodeBlockHeader (
    void
    )
{
    PXZ_BLOCK_HEADER blockHeader;
#ifdef MINLZ_META_CHECKS
    uint32_t dictionarySize;
#endif
    //
    // Seek past the header, making sure we have space in the input stream. If
    // the header indicates a size of 0, then this is a blockless (empty) file
    // and this is actually an index. Undo the seek so we can parse the index.
    //
    if (!BfSeek(sizeof(*blockHeader), (const uint8_t**)&blockHeader) ||
        (blockHeader->Size == 0))
    {
        BfSeek((uint32_t)(-(uint16_t)sizeof(*blockHeader)),
               (const uint8_t**)&blockHeader);
        return false;
    }
#ifdef MINLZ_META_CHECKS
    //
    // Validate that the size of the header is what we expect
    //
    Container.HeaderSize = (blockHeader->Size + 1) * 4;
    if (Container.HeaderSize != sizeof(*blockHeader))
    {
        return false;
    }

    //
    // Validate that no additional flags or filters are enabled
    //
    if (blockHeader->u.Flags != 0)
    {
        return false;
    }

    //
    // Validate that the only filter is the LZMA2 filter
    //
    if (blockHeader->LzmaFlags.Id != k_XzLzma2FilterIdentifier)
    {
        return false;
    }

    //
    // With the expected number of property bytes
    //
    if (blockHeader->LzmaFlags.Size
        != sizeof(blockHeader->LzmaFlags.u.Properties))
    {
        return false;
    }

    //
    // The only property is the dictionary size, make sure it is valid.
    //
    // We don't actually need to store or compare the size with anything since
    // the library expects the caller to always put in a buffer that's large
    // enough to contain the full uncompressed file (or calling it in "get size
    // only" mode to get this information).
    //
    // This output buffer can thus be smaller than the size of the dictionary
    // which is absolutely OK as long as that's actually the size of the output
    // file. If callers pass in a buffer size that's too small, decoding will
    // fail at later stages anyway, and that's incorrect use of minlzlib.
    //
    dictionarySize = blockHeader->LzmaFlags.u.s.DictionarySize;
    if (dictionarySize > 39)
    {
        return false;
    }
#ifdef MINLZ_INTEGRITY_CHECKS
    //
    // Compute the header's CRC32 and make sure it's not corrupted
    //
    if (Crc32(blockHeader,
              Container.HeaderSize - sizeof(blockHeader->Crc32)) !=
        blockHeader->Crc32)
    {
        Container.ChecksumError = true;
    }
#endif
#endif
    return true;
}

bool
XzDecode (
    const uint8_t* InputBuffer,
    uint32_t InputSize,
    uint8_t* OutputBuffer,
    uint32_t* OutputSize
    )
{
    //
    // Initialize the input buffer descriptor and history buffer (dictionary)
    //
    BfInitialize(InputBuffer, InputSize);
    DtInitialize(OutputBuffer, *OutputSize, 0);

    //
    // Decode the stream header to check for validity
    //
    if (!XzDecodeStreamHeader())
    {
        return false;
    }

    //
    // Decode the block header to check for validity. If it appears valid, go
    // decode the block. Otherwise, this may be a blockless (empty input) file.
    //
    if (XzDecodeBlockHeader())
    {
        if (!XzDecodeBlock(OutputBuffer, OutputSize))
        {
            return false;
        }
    }
    else
    {
        *OutputSize = 0;
    }
#ifdef MINLZ_META_CHECKS
    //
    // Decode the index for validity checks
    //
    if (!XzDecodeIndex())
    {
        return false;
    }

    //
    // And finally decode the footer as a final set of checks
    //
    if (!XzDecodeStreamFooter())
    {
        return false;
    }
#endif
    return true;
}

bool
XzChecksumError (
    void
    )
{
    //
    // Return to an external caller if a checksum error was encountered
    //
#ifdef MINLZ_INTEGRITY_CHECKS
    return Container.ChecksumError;
#else
    return false;
#endif
}
