struct point3d {
	float x;
	float y;
	float z;
};

struct hingetype {
	s32 parenthinge;
	point3d childPoint;
	point3d parentPoint;
	point3d childAxisRotationVector;
	point3d parentAxisRotationVector;
	s16 vmin;
	s16 vmax;
	s8 htype;
	s8 filler[7];
};

struct seqtyp {
	s32 timeMillis;
	s32 frm;
};

struct Header {
	u32 magic;
	u32 stringlen;
	u8 filename[stringlen];
	u32 numhinge;
	hingetype hinge[numhinge];
	u32 numfrm;
	// These are the hinge angles. 0 is 0 degrees, 16384 is 90 degrees, -16384 is -90 degrees
	s16 frmval[numfrm * numhin]; //[numfrm][numhin]
	u32 seqnum;
	seqtyp seq[seqnum];
};

Header hdr @0x00;
