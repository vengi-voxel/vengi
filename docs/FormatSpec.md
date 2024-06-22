# VENGI File Format Specification

Everything is stored in little endian.

## Overview

The VENGI format is designed to store scene graph data including node properties, animations, voxel data, and palette information. The data is stored in a compressed format using ZIP compression.

## File Structure

A VENGI file consists of the following main sections:

1. **Magic Number**: A 4-byte identifier `VENG`.
2. **Zip data**
    * **Version**: A 4-byte version number. The current supported version is `3`.
    * **Scene Graph Data**: Contains information about the scene graph nodes.

## Chunk Structure

Each chunk in the file is identified by a 4-byte FourCC code, followed by its data. The main chunk types are:

* `NODE`: Indicates the beginning of a scene graph node.
* `PROP`: Contains properties of a node.
* `DATA`: Contains voxel data of a node.
* `PALC`: Contains palette colors.
* `PALI`: Contains a palette identifier.
* `ANIM`: Contains animation data for a node.
* `KEYF`: Contains keyframe data for an animation.
* `ENDA`: Marks the end of an animation chunk.
* `ENDN`: Marks the end of a node chunk.

## Detailed Format Description

### Magic Number and Version

* **Magic Number**: `0x56454E47` (`'VENG'`)
* **Version**: 4-byte unsigned integer (current version: `3` - already part of the compressed data)
* **Root node**: The scene graph root node

### Scene Graph Nodes

Each node chunk begins with the `NODE` FourCC and includes the following information:

* **Node Name**: String (16-bit length prefix, followed by UTF-8 encoded string)
* **Node Type**: String (16-bit length prefix, followed by UTF-8 encoded string)
* **Node ID**: 4-byte signed integer
* **Reference Node ID**: 4-byte signed integer (for referenced nodes - `-1` if no node is referenced)
* **Visibility**: 1-byte boolean
* **Lock State**: 1-byte boolean
* **Color**: 4-byte RGBA value
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
    * **Color**: 1-byte unsigned integer (only if not air)

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
* **Color Count**: 4-byte unsigned integer
* **Colors**: For each color:
    * **RGBA**: 4-byte unsigned integer
* **Emit Colors**: For each color (deprecated):
    * **RGBA**: 4-byte unsigned integer (always 0)
* **Indices**: For each color:
    * **Index**: 1-byte unsigned integer
* **Material Count**: 4-byte unsigned integer
* **Materials**: For each material:
    * **Type**: 4-byte unsigned integer
    * **Property Count**: 1-byte unsigned integer
    * **Properties**: For each property:
        * **Name**: String (16-bit length prefix, followed by UTF-8 encoded string)
        * **Value**: 4-byte float

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
    * **Local Matrix**: Sixteen 4-byte floats (4x4 matrix in row-major order)

The end of the animation chunk is marked by the `ENDA` FourCC.
