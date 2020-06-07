# Voxel formats (not all supported yet)

## Binvox (binvox)

Format spec: <https://www.patrickmin.com/binvox/binvox.html>

### BINVOX voxel file format specification

This format uses simple compression (run-length encoding) to reduce filesize. The format was put together by Michael Kazhdan.

A `.binvox` file has a short ASCII header, followed by binary data.
The ASCII header
The header looks like this:

```c
#binvox 1
dim 128 128 128
translate -0.120158 -0.481158 -0.863158
scale 7.24632
data
```

The first line (with `#binvox 1`) is required (the `1` is a version number). The next line specifies the depth, width, and height of the voxel grid, which should all be equal. The next two lines specify the normalization transformation (see the next section) that was used to normalize the input mesh. The last header line must read `data`.

Normalization and Mesh Correspondence
Before voxelizing, binvox normalizes the mesh such that it fits inside a 1.0x1.0x1.0 cube with its origin at (0.0, 0.0, 0.0). This is done with a translation and a uniform scale. The unit cube is then voxelized. Three normalization transformation steps are printed to the terminal when you run binvox, e.g.:

```c
bounding box: [-4.26774, -4.46283, -3.51203, 1] - [3.73801, 4.85995, 3.53327, 1]
normalization transform:
(1) translate [4.26774, 4.46283, 3.51203, 1], (2) scale 0.107264, (3) translate [0, 0, 0]
```

Note that the third one can be ignored (it used to be relevant when normalization was not to the unit cube).

As a consequence, each voxel in the voxel model has coordinates inside the unit cube, which can be obtained as follows:

Given a voxel at (i, j, k) (with voxel index coordinates starting at (0, 0, 0), and voxel model dimension d, these coordinates are:

```c
(x_n, y_n, z_n) = ((i + 0.5) / d, (j + 0.5) / d, (k + 0.5) / d)
```

(the 0.5 is added to get the coordinates of the center of the voxel cell).
Next, there are two methods to compute the corresponding mesh coordinates from (`x_n`, `y_n`, `z_n`):

first method:

  binvox now includes two extra lines in the header (which may be omitted, viewvox and thinvox don't need them):
  `translate <t_x> <t_y> <t_z>`
  `scale <scale factor>`
  First scale (`x_n`, `y_n`, `z_n`) by the scale factor, then translate them by (`t_x`, `t_y`, `t_z`)

second method:

  Note the normalization transformation steps from the output of binvox, and apply these in reverse to (`x_n`, `y_n`, `z_n`)

The first and second method should have the same level of accuracy, but I still have to run some tests to verify this.

Voxel ordering

The y-coordinate runs fastest, then the z-coordinate, then the x-coordinate. To illustrate, here is the get_index function that computes the index in the 1D array of voxels of a voxel with indices (x, y, z):

```c
int
Voxels::get_index(int x, int y, int z)
{
  int index = x * wxh + z * width + y;  // wxh = width * height = d * d
  return index;

}  // Voxels::get_index
```

The binary voxel data

The binary data consists of pairs of bytes. The first byte of each pair is the value byte and is either 0 or 1 (1 signifies the presence of a voxel). The second byte is the count byte and specifies how many times the preceding voxel value should be repeated (so obviously the minimum count is 1, and the maximum is 255).

## Qubicle (qb, qbt)

Format spec: <https://getqubicle.com/learn/category.php?id=34>

## MagicaVoxel (vox)

Format spec: <https://github.com/ephtracy/voxel-model>

## SLAB6/Build-Engine (kvx,kv6)

Format spec <https://github.com/vuolen/slab6-mirror/blob/master/slab6.txt>

### VOX file format

Both SLABSPRI&SLAB6(D) support a simpler, uncompressed voxel format using
the VOX file extension. Here's some C pseudocode that describes the format:

```c
long xsiz, ysiz, zsiz;          //Variable declarations
char voxel[xsiz][ysiz][zsiz];
char palette[256][3];

fil = open("?.vox",...);
read(fil,&xsiz,4);              //Dimensions of 3-D array of voxels
read(fil,&ysiz,4);
read(fil,&zsiz,4);
read(fil,voxel,xsiz*ysiz*zsiz); //The 3-D array itself!
read(fil,palette,768);          //VGA palette (values range from 0-63)
close(fil);
```

VOX files use color 255 to define empty space (air). This is a limitation
of the VOX format. Fortunately, KVX doesn't have this problem. For
interior voxels (ones you can never see), do not use color 255, because
it will prevent SLABSPRI/SLAB6(D) from being able to take advantage of
back-face culling.

### KVX file format

I'm always interested in adding more sample voxels to my collection.
Because I'm such a nice guy, I am describing my KVX voxel format here so
you can use them in your own programs.

The KVX file format was designed to be compact, yet also renderable
directly from its format.  Storing a byte for every voxel is not efficient
for large models, so I use a form of run-length encoding and store only
the voxels that are visible - just the surface voxels.  The "runs" are
stored in the ceiling to floor direction because that is the best axis to
use for fast rendering in the Build Engine.

Each KVX file uses this structure for each of its mip-map levels:

```c
long xsiz, ysiz, zsiz, xpivot, ypivot, zpivot;
long xoffset[xsiz+1];
short xyoffset[xsiz][ysiz+1];
char rawslabdata[?];
```

The file can be loaded like this:

```c
if ((fil = open("?.kvx",O_BINARY|O_RDWR,S_IREAD)) == -1) return(0);
nummipmaplevels = 1;  //nummipmaplevels = 5 for unstripped KVX files
for(i=0;i<nummipmaplevels;i++)
{
  read(fil,&numbytes,4);
  read(fil,&xsiz,4);
  read(fil,&ysiz,4);
  read(fil,&zsiz,4);
  read(fil,&xpivot,4);
  read(fil,&ypivot,4);
  read(fil,&zpivot,4);
  read(fil,xoffset,(xsiz+1)*4);
  read(fil,xyoffset,xsiz*(ysiz+1)*2);
  read(fil,voxdata,numbytes-24-(xsiz+1)*4-xsiz*(ysiz+1)*2);
}
read(fil,palette,768);
```

`numbytes`: Total # of bytes (not including numbytes) in each mip-map level

`xsiz`, `ysiz`, `zsiz`: Dimensions of voxel. (`zsiz` is height)

`xpivot`, `ypivot`, `zpivot`: Centroid of voxel.  For extra precision, this
   location has been shifted up by 8 bits.

`xoffset`, `xyoffset`: For compression purposes, I store the column pointers
   in a way that offers quick access to the data, but with slightly more
   overhead in calculating the positions.  See example of usage in voxdata.
   NOTE: `xoffset[0] = (xsiz+1)*4 + xsiz*(ysiz+1)*2` (ALWAYS)

`voxdata`: stored in sequential format.  Here's how you can get pointers to
   the start and end of any (x, y) column:

```c
//pointer to start of slabs on column (x, y):
startptr = &voxdata[xoffset[x] + xyoffset[x][y]];

//pointer to end of slabs on column (x, y):
endptr = &voxdata[xoffset[x] + xyoffset[x][y+1]];
```

   Note: endptr is actually the first piece of data in the next column

   Once you get these pointers, you can run through all of the "slabs" in
   the column.  Each slab has 3 bytes of header, then an array of colors.
   Here's the format:

```c
   char slabztop;             //Starting z coordinate of top of slab
   char slabzleng;            //# of bytes in the color array - slab height
   char slabbackfacecullinfo; //Low 6 bits tell which of 6 faces are exposed
   char col[slabzleng];       //The array of colors from top to bottom
```

`palette`:
The last 768 bytes of the KVX file is a standard 256-color VGA palette.
The palette is in (Red:0, Green:1, Blue:2) order and intensities range
from 0-63.

Note: To keep this ZIP size small, I have stripped out the lower mip-map
levels.  KVX files from Shadow Warrior or Blood include this data.  To
get the palette data, I recommend seeking 768 bytes before the end of the
KVX file.

### KV6 file format

```c
//C pseudocode for loader:
typedef struct { long col; unsigned short z; char vis, dir; } kv6voxtype;
long xsiz, ysiz, zsiz;
float xpiv, ypiv, zpiv;
unsigned long xlen[xsiz];
unsigned short ylen[xsiz][ysiz];
long numvoxs;

FILE *fil = fopen("FILE.KV6","rb");

fread(&fileid,4,1,fil); //'Kvxl' (= 0x6c78764b in Little Endian)

//Voxel grid dimensions
fread(&xsiz,4,1,fil); fread(&ysiz,4,1,fil); fread(&zsiz,4,1,fil);

//Pivot point. Floating point format. Voxel units.
fread(&xpiv,4,1,fil); fread(&ypiv,4,1,fil); fread(&zpiv,4,1,fil);

fread(&numvoxs,4,1,fil); //Total number of surface voxels
for(i=0;i<numvoxs;i++) //8 bytes per surface voxel, Z's must be sorted
{
  red  [i]    = fgetc(fil); //Range: 0..255
  green[i]    = fgetc(fil); //"
  blue [i]    = fgetc(fil); //"
  dummy       = fgetc(fil); //Always 128. Ignore.
  height_low  = fgetc(fil); //Z coordinate of this surface voxel
  height_high = fgetc(fil); //"
  visibility  = fgetc(fil); //Low 6 bits say if neighbor is solid or air
  normalindex = fgetc(fil); //Uses 256-entry lookup table
}

// Number of surface voxels present in plane x (extra information)
for(x=0;x<xsiz;x++) fread(&xlen[x],4,1,fil);

// Number of surface voxels present in column (x,y)
for(x=0;x<xsiz;x++) for(y=0;y<ysiz;y++) fread(&ylen[x][y],2,1,fil);

// Added 06/30/2007: suggested palette (optional)
fread(&suggpalid,4,1,fil); //'SPal' (= 0x6C615053 in Little Endian)
fread(suggestedpalette,768,1,fil); //R,G,B,R,G,.. Value range: 0-63

fclose(fil);
```

KVX format: In versions previous to 12/25/2003, SLAB6 saved only
the first mip-map level. This results in a smaller file size. However,
if you want to use your voxel object in the Build Engine, you must save
all 5 mips. Also, be aware that the (classic) Build Engine can not load a
KVX voxel model larger than 128x128x200! Anything larger will likely
crash (classic) Build. SLAB6 can handle up to 256x256x255. It is up to
you to make sure the limits are not exceeded.

Note: If you load a KV6 file (written from a version of SLAB6 previous to
July 2007) directly from the command line, it will use the first 256 colors
it finds. If you load a KV6 file while inside the editor, it will convert
the colors to the current palette. As of July 2007, SLAB6 stores a
suggested palette at the end of KV6 files, so there is no need to also
write a KVX file to save the palette.

## Cubeworld (cub)

The first 12 bytes of the file are the width, depth and height of the volume (uint32_t little endian). The remaining parts are the RGB values (3 bytes).

## VoxEdit (vxm)

## Minecraft region file (mca)

Format spec: <https://minecraft.gamepedia.com/Region_file_format>

## Voxlap / Ace of Spades Map (vxl)

Format spec: <https://silverspaceship.com/aosmap/aos_file_format.html>

## Command & Conquer: Red Alert 2 (vxl)

Format "spec": <https://github.com/OpenRA/OpenRA/blob/master/OpenRA.Mods.Cnc/FileFormats/VxlReader.cs>

## SVX Format

Format spec: <https://abfab3d.com/svx-format/>

Example implementation: <https://gist.github.com/Eisenwave/799416ac162a4dddeb1312f357f1385c>
