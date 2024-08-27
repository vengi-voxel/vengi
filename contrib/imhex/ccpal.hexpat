// Command and conquer palette format

struct Color {
	u8 r;
	u8 g;
	u8 b;
};

struct Palette {
	Color colors[256];
};

Palette pal @0x00;
