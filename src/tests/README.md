# Visual Tests

These tests are dedicated test applications. They are no unit tests.

Unit tests are in their own module and available via
```
make tests
```

Run these test applications with
```
make __directoryname__-run
```

# testimgui

Test the dearimgui integration

# testglslgeom

Test geometry shader integration

# testnuklear

Test the nuklear imgui integration

# testcomputetexture3d

Test the OpenCL 3d texture integration of a 3d voxel volume (rendered as 2d side view)

# testvoxelgpu

OpenCL mesh extraction - not finished yet.

# testdepthbuffer

Test the depth buffer integration with shadow maps and debug renderings

# testtexture

Renders a test 2d image.

# testmesh

Render a mesh with shadows and bones and normals (NOTE: there is no mesh in the repo to display - was removed in b33dd4c853514addeb6f09d1a3c009e9549ffd2c).

# testcamera

Test camera options and collisions.

# testvoxelfont
# testplane
# testshapebuilder
# testoctree

Renders the octree internals.

# testglslcomp
# testgpumc
# testturbobadger
# testluaui
# testoctreevisit
# testtemplate

Just an empty template for new test applications.

# Notes

## Currently broken

- The imgui free look mode is currently broken - usually you should be able to hit esc to toggle the free look and camera move mode.
- The mesh rendering is currently broken - testmesh and testdepthbuffer are affected.
- testglslgeom throws an invalid enum error on my notebook.
