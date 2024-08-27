// Command and conquer voxel animation format

struct vec4_s {
	float x [[color("FF0000")]];
	float y [[color("00FF00")]];
	float z [[color("0000FF")]];
	float w [[color("FFFF00")]];
};

struct mat3x4_s {
	vec4_s row[3];
};

struct name_s {
	char buffer[16] [[color("EECCCC")]];
};

struct hva_s {
	name_s name;
	u32 numFrames;
	u32 numNodes;
	name_s nodeNames[numNodes];
};

struct frame_s {
	mat3x4_s mat[hva.numNodes];
};

hva_s hva @0x00;
frame_s frames[hva.numFrames] @sizeof(hva);
