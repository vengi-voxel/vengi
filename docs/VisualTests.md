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

or by directly executing the generated binary after compiling them.

## testbloom

A test application that applies bloom to an image

## testcamera

Test camera options and collisions.

## testcomputetexture3d

Test the OpenCL 3d texture integration of a 3d voxel volume (rendered as 2d side view)

## testglslcomp

Uses GLSL compute shader to render a circle.

## testglslgeom

Test geometry shader integration

## testgpumc

Conversion of OpenCL marching cubes taken from: <https://github.com/smistad/GPU-Marching-Cubes.git>

## testimgui

Test the dearimgui integration

## testoctree

Renders the octree internals.

## testoctreevisit

Visit the frustum in the octree.

## testplane

Renders a plane object.

## testshapebuilder

## testtemplate

Just an empty template for new test applications.

## testtexture

Renders a test 2d image.

## testvoxelfont

Display a true type font as 3d voxel volume.

## testvoxelgpu

OpenCL mesh extraction - not finished yet.
