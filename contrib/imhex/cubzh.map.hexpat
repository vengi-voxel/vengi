struct Color {
	u8 r;
	u8 g;
	u8 b;
	u8 a;
};

struct ChunkMap {
	double scale;
	u32 len;
	char name[len];
};

struct Field {
	char id[3];
	if (id == "ssc") {
		Color sky_color;
	} else if (id == "shc") {
		Color sky_horizonColor;
	} else if (id == "sac") {
		Color sky_abyssColor;
	} else if (id == "slc") {
		Color sky_lightColor;
	} else if (id == "sli") {
		float sky_lightIntensity;
	} else if (id == "foc") {
		Color fog_color;
	} else if (id == "fon") {
		float fog_near;
	} else if (id == "fof") {
		float fog_far;
	} else if (id == "foa") {
		float light_absorbtion;
	} else if (id == "suc") {
		Color sun_color;
	} else if (id == "sui") {
		float sun_intensity;
	} else if (id == "sur") {
		float sun_rotation[2];
	} else if (id == "asl") {
		float ambient_skyLightFactor;
	} else if (id == "adl") {
		float ambient_dirLightFactor;
	} else if (id == "txt") {
		u8 len;
		char txt[len];
	}
};

struct Ambience {
	u16 size;
	u8 nFields;
	Field fields[nFields];
};

struct Block {
	u16 keySize;
	char key[keySize];
	// 1 == add, 0 == remove
	u8 blockAction;
	if (blockAction == 1) {
		Color color;
	}
};

struct Blocks {
	u32 chunk_len;
	u16 nBlocks;
	Block blocks[nBlocks];
};

struct ObjectGroupEntry3 {
	char id[2];
	if (id == "id") {
		u8 uuidLen;
		char uuid[uuidLen];
	} else if (id == "po") {
		float pos[3];
	} else if (id == "ro") {
		float rot[3];
	} else if (id == "sc") {
		float scale[3];
	} else if (id == "na") {
		u8 nameLen;
		char name[nameLen];
	} else if (id == "pm") {
		// phsicsmode
		// RigidbodyMode_Disabled = 0,
		// RigidbodyMode_Trigger = 1,
		// RigidbodyMode_TriggerPerBlock = 2,
		// RigidbodyMode_Static = 3,
		// RigidbodyMode_StaticPerBlock = 4,
		// RigidbodyMode_Dynamic = 5,
		u32 byte;
	} else {
		u8 byte;
	}
};

struct ObjectGroup3 {
	u8 nbFields;
	ObjectGroupEntry3 entries[nbFields];
};

struct Object3 {
	u16 fullnameLen;
	char fullname[fullnameLen];
	u16 nInstances;
	ObjectGroup3 groups[nInstances];
};

struct Objects3 {
	u32 chunk_len;
	u16 nObjects;
	Object3 objects[191];
	Object3 fail;
};

struct ObjectGroupEntry2 {
	char id[2];
	if (id == "id") {
		u8 uuidLen;
		char uuid[uuidLen];
	} else if (id == "po") {
		float pos[3];
	} else if (id == "ro") {
		float rot[3];
	} else if (id == "sc") {
		float scale[3];
	} else if (id == "na") {
		u8 nameLen;
		char name[nameLen];
	} else if (id == "de") {
		u16 len;
		char base64[len];
	} else if (id == "pm") {
		// physicsmode - unknown
	}
};

struct ObjectGroup2 {
	u8 nbFields;
	ObjectGroupEntry2 entries[nbFields];
};

struct Object2 {
	u16 fullnameLen;
	char fullname[fullnameLen];
	u16 nInstances;
	ObjectGroup2 groups[nInstances];
};

struct Objects2 {
	u32 chunk_len;
	u16 nObjects;
	Object2 objects[nObjects];
};

struct Chunk3 {
	u8 chunkId;
	if (chunkId == 0 ) {
		ChunkMap map;
	} else if (chunkId == 1) {
		Ambience ambience;
	} else if (chunkId == 2) {
		Objects3 objects;
	} else if (chunkId == 3) {
		Blocks blocks;
	}
};

struct Chunk2 {
	u8 chunkId;
	if (chunkId == 0 ) {
		ChunkMap map;
	} else if (chunkId == 1) {
		Ambience ambience;
	} else if (chunkId == 2) {
		Objects2 objects;
	} else if (chunkId == 3) {
		Blocks blocks;
	}
};

struct Header {
	u8 version;
	if (version == 1) {
		u8 chunkByte;
		ChunkMap map;
		u8 chunkByte2;
		Ambience ambience;
		u8 chunkByte3;
		Blocks blocks;
	} else if (version == 2) {
		Chunk2 chunks[2];
	} else if (version == 3) {
		Chunk3 chunks[3];
	} else {
		// just a table
	}
};

Header hdr @0x00;
