# VENGI File Format Specification

Everything is stored in little endian.

## Overview

The VENGI format is designed to store scene graph data including node properties, animations, voxel data, and palette information. The data is stored in a compressed format using ZIP compression.

## File Structure

A VENGI file consists of the following main sections:

1. **Magic Number**: A 4-byte identifier `VENG`.
2. **Zip data**: zlib header (0x78, 0xDA)
    * **Version**: A 4-byte version number. The current supported version is `7`.
    * **Scene Graph Data**: Contains information about the scene graph nodes.

## Node Structure

Nodes are composed of data chunks that each start with a FourCC code.

* `NODE`: Indicates the beginning of a scene graph node.
    * `PROP`: Contains properties of a node (only present if there are properties).
    * `DATA`: Contains voxel data of a node (only if type is `Model`).
    * `PALC`: Contains palette colors (only present if PALI is not).
    * `PALN`: Contains palette normals
    * `PALI`: Contains a palette identifier (only present if PALC is not).
    * `ANIM`: Contains animation data for a node.
        * `KEYF[]`: Contains keyframe data for an animation.
        * `ENDA`: Marks the end of an animation chunk.
    * `IKCO`: Contains inverse kinematics constraint data for a node (since version 7).
    * `NODE[]`: child nodes (only present if there are child nodes)
    * `ENDN`: Marks the end of a node chunk.

## Detailed Format Description

### Magic Number and Version

* **Magic Number**: `0x56454E47` (`'VENG'`)
* **Version**: 4-byte unsigned integer (already part of the compressed data)
* **Root node**: The scene graph root node

### Scene Graph Nodes

Each node chunk begins with the `NODE` FourCC and includes the following information:

* **Node Name**: String (16-bit length prefix, followed by UTF-8 encoded string)
* **Node Type**: String (16-bit length prefix, followed by UTF-8 encoded string)
* **Node UUID**: two 64 bit unsigned integers in little endian
* **Node ID**: 4-byte signed integer
* **Reference Node ID**: 4-byte signed integer (for referenced nodes - `-1` if no node is referenced)
* **Visibility**: 1-byte boolean
* **Lock State**: 1-byte boolean
* **Color**: 4-byte ABGR value
* **Pivot**: Three 4-byte floats (x, y, z)
* **Properties**: Properties chunk - optional if the node doesn't have any properties
* **Palette**: Palette chunk
* **Data**: Data chunk
* **Animations**: n-Animation chunks
* **Child nodes**: Node chunk

Node types are:

* `Root`
* `Model`
* `ModelReference`
* `Group`
* `Camera`
* `Point`

Each node has the FourCC `ENDN` at its end

> You should not rely on the order of chunks when loading a `vengi` file.

#### Node Properties

Node properties are stored in the `PROP` chunk.

> Note: This chunk is only available if the node has properties.

* **FourCC**: `PROP`
* **Property Count**: 4-byte unsigned integer
* **Properties**: For each property:
    * **Key**: String (16-bit length prefix, followed by UTF-8 encoded string)
    * **Value**: String (16-bit length prefix, followed by UTF-8 encoded string)

#### Voxel Data

Voxel data is stored in the `DATA` chunk.

> Note: This chunk is only available if the node is a model node.

* **FourCC**: `DATA`
* **Region**: Six 4-byte signed integers (lowerX, lowerY, lowerZ, upperX, upperY, upperZ)
* **Voxel Information**: For each voxel in the region:
    * **Air**: 1-byte boolean (true if air, false if solid)
    * **Color**: 1-byte unsigned integer (only if not air) (Color palette index)
    * **Normal**: 1-byte unsigned integer (only if not air) (Normal palette index)

The voxel data is stored like this:

```c
for(x = mins.x; x <= maxs.x; ++x)
 for(y = mins.y; y <= maxs.y; ++y)
  for(z = mins.z; z <= maxs.z; ++z)
   writeVoxelInformation(x, y, z)
```

#### Palette Colors

Palette colors are stored in the `PALC` chunk (or in `PALI` - see below):

* **FourCC**: `PALC`
* **Palette Name**: String (16-bit length prefix, followed by UTF-8 encoded string)
* **Color Count**: 4-byte unsigned integer
* **Colors**: For each color:
    * **ABGR**: 4-byte unsigned integer
* **Emit Colors**: For each color (deprecated):
    * **ABGR**: 4-byte unsigned integer (always 0)
* **Indices**: For each color:
    * **Index**: 1-byte unsigned integer
* **Color Names**: For each color:
	* **Name**: String (16-bit length prefix, followed by UTF-8 encoded string)
* **Material Count**: 4-byte unsigned integer
* **Materials**: For each material:
    * **Type**: 4-byte unsigned integer
    * **Property Count**: 1-byte unsigned integer
    * **Properties**: For each property:
        * **Name**: String (16-bit length prefix, followed by UTF-8 encoded string)
        * **Value**: 4-byte float

#### Palette Normals

Palette normals are stored in the `PALN` chunk:

* **FourCC**: `PALN`
* **Normal Count**: 4-byte unsigned integer
* **Normals**: For each color:
    * **ABGR**: 4-byte unsigned integer

#### Palette Identifier

Palette identifier is stored in the `PALI` chunk.

> Note: This is only used if the palette is a built-in vengi [palette] (Palette.md) - otherwise the `PALC` chunk is used.

* **FourCC**: `PALI`
* **Palette Name**: String (16-bit length prefix, followed by UTF-8 encoded string)

This is used for internal [palettes](Palette.md).

#### Animations

Animations are stored in the `ANIM` chunk:

* **FourCC**: `ANIM`
* **Animation Name**: String (16-bit length prefix, followed by UTF-8 encoded string)
* **Keyframes**: Each keyframe starts with the `KEYF` chunk:
    * **FourCC**: `KEYF`
    * **Frame Index**: 4-byte unsigned integer
    * **Long Rotation**: 1-byte boolean
    * **Interpolation Type**: String (16-bit length prefix, followed by UTF-8 encoded string)
    * **Local Matrix**: Sixteen 4-byte floats (4x4 matrix in column-major order)

The end of the animation chunk is marked by the `ENDA` FourCC.

#### IK Constraints

Inverse kinematics constraints are stored in the `IKCO` chunk (since version 7):

> Note: This chunk is only present if the node has an IK constraint.

* **FourCC**: `IKCO`
* **Effector Node ID**: 4-byte signed integer (node id of the IK end-effector target, `-1` if none)
* **Roll Min**: 4-byte float (minimum roll angle in radians)
* **Roll Max**: 4-byte float (maximum roll angle in radians)
* **Visible**: 1-byte boolean
* **Anchor**: 1-byte boolean
* **Swing Limit Count**: 4-byte unsigned integer
* **Swing Limits**: For each swing limit:
    * **Center X**: 4-byte float
    * **Center Y**: 4-byte float
    * **Radius**: 4-byte float
