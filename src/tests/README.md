# Visual Tests

These tests are dedicated test applications. They are no unit tests.

Unit tests are in their own module and available via

```bash
make tests
```

Run these test applications with

```bash
make __directoryname__-run
```

## testanimation

Test the voxel animation module with a default character and items of the stock module.

See [README.md](testanimation/README.md) for more details.

## testimgui

Test the dearimgui integration

## testglslgeom

Test geometry shader integration

## testnuklear

Test the nuklear imgui integration

## testcomputetexture3d

Test the OpenCL 3d texture integration of a 3d voxel volume (rendered as 2d side view)

## testvoxelgpu

OpenCL mesh extraction - not finished yet.

## testdepthbuffer

Test the depth buffer integration with shadow maps and debug renderings

## testtexture

Renders a test 2d image.

## testmesh

Render a mesh with shadows and bones and normals.

## testcamera

Test camera options and collisions.

## testvoxelfont

Display a true type font as 3d voxel volume.

## testplane

Renders a plane object.

## testshapebuilder

## testoctree

Renders the octree internals.

## testglslcomp

Uses GLSL compute shader to render a circle.

## testgpumc

Conversion of OpenCL marching cubes taken from: <https://github.com/smistad/GPU-Marching-Cubes.git>

## testturbobadger

Renders the turbobadger demo.

## testluaui

Test the nuklear lua ui binding

## testoctreevisit

Visit the frustum in the octree.

## testtemplate

Just an empty template for new test applications.

## testtraze

* [traze client](testtraze/README.md)

## testhttpserver

A test application around the http module server for e.g. fuzzy testing purposes.

## testbiomes

A test application that just visualizes the biomes.
