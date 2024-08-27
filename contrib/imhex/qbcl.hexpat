// Qubicle QBCL format

struct String {
	u32 len;
	char string[len];
};

struct Matrix {
	u32 sizex;
	u32 sizey;
	u32 sizez;
	s32 tx;
	s32 ty;
	s32 tz;
	float pivotx;
	float pivoty;
	float pivotz;
	u32 compressedDataSize;
	u8 zip[compressedDataSize];
};

struct Matrices {
	u8 mat1[9];
	u8 mat2[9];
	u8 mat3[9];
	u8 mat4[9];
};

using Node;

struct Model {
	Matrices rotation;
	u32 childCount;
	Node nodes[childCount];
};

struct Compound {
	Matrix matrix;
	u32 childCount;
	Node nodes[childCount];
};

struct Digest {
	u8 first[4]; // changes on each re-save
	u8 second[4]; // changes if the model changes
	u8 third[4]; // chnages on each re-save
	u8 fourth[4]; // changes if the model changes
};

struct Node {
	u32 type;
	u32 unknown;
	String name;
	u8 visible;
	u8 unknown2;
	u8 locked;
	if (type == 0) {
		Matrix matrix;
	} else if (type == 1) {
		Model model;
	} else if (type == 2) {
		Compound compound;
	}
};

struct Header {
	u32 magic;
	u32 version;
	u32 fileversion;
	u32 thumbwidth;
	u32 thumbheight;
	char bgra[thumbwidth * thumbheight * 4];
	String title;
	String desc;
	String metadata;
	String author;
	String company;
	String website;
	String copyright;
	Digest digest;
	Node node;
};

Header hdr @0x00;
