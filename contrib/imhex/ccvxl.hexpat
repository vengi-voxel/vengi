// Command and Conquer voxel model format

struct vec4_s {
    float x [[color("FF0000")]];
    float y [[color("00FF00")]];
    float z [[color("0000FF")]];
    float w [[color("FFFF00")]];
};

struct vec3_s {
    float x [[color("FF0000")]];
    float y [[color("00FF00")]];
    float z [[color("0000FF")]];
};

struct mat3x4_s {
    vec4_s row[3];
};

struct name_s {
	char buffer[16] [[color("EECCCC")]];
};

struct color_s {
	u8 r;
	u8 g;
	u8 b;
};

struct vxl_limb_header_s {
	name_s name;
	s32 limb_number;
	u32 _reserved1;
	u32 _reserved2;
};

struct vxl_limb_tailer_s {
	u32 span_start_offset;
	u32 span_end_offset;
	u32 span_data_offset;
	float scale;
	mat3x4_s matrix;
	vec3_s min_bounds;
	vec3_s max_bounds;
	u8 xsize;
	u8 ysize;
	u8 zsize;
	u8 normal_type;
};

struct vxl_s {
	name_s name;
	u32 _reserved;
	u32 limb_count;
	u32 _limb_count;
	u32 body_size;
	u8 remap_start_index;
	u8 remap_end_index;
	color_s internal_palette[256];
	vxl_limb_header_s _headers[limb_count];
	u8 body[body_size];
	vxl_limb_tailer_s _tailers[limb_count];
};

struct frame_s {
	mat3x4_s mat[hva.numNodes];
};

vxl_s vxl @0x00;
