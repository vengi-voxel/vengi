struct V6ChunkHeader {
    u32 chunk_size;
    u8 compressed;
    u32 num_bytes_uncompressed;
    if (compressed != 0) {
    }
};

struct Chunk {
    u8 chunk_id [[color("FF0000")]];
    if (chunk_id == 1) {
        u32 num_bytes;
        u8 png[num_bytes];
    } else if (chunk_id == 2 || chunk_id == 16) { 
        // Palette
        V6ChunkHeader header;
        u8 paletteZip[header.chunk_size];
    } else if (chunk_id == 15) { 
        // Palette id
        V6ChunkHeader header;
        u8 paletteId[header.chunk_size];
    } else if (chunk_id == 3) { 
        // shape
        V6ChunkHeader header;
        u8 shapeZip[header.chunk_size];
    } else {
       u32 num_bytes;
       u8 unknownData[num_bytes];
    }
};

struct Header {
    char magic[11];
    u32 version;
    u8 compressionType;
    u32 totalSize;
    Chunk chunk[8];
};

Header hdr @0x00;
