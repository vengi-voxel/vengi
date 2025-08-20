# Devlog 08/2024

I am starting to write a devlog to keep track of the development progress and changes in the vengi project as well as other projects I've been working on. This will help document the evolution of the codebase and provide insights into the development process. I usually don't write devlogs and this is the first time I am doing it, so please bear with me.

With 0.1.0 I've bumped the minor version for the first time. The project is now in a more stable state, with many features implemented and a focus on improving the user experience. The next steps will include refining the existing features, fixing bugs, and enhancing performance. This only took a few years ...

With the release of version 0.1.0 you have the chance to import all your minecraft skin png files - including extensions.

![image showing minecraft skin without extensions](https://raw.githubusercontent.com/wiki/vengi-voxel/vengi/images/voxedit-minecraft-skin-no-ext_2025-07-09.png#gallery) ![image showing minecraft skin with extensions](https://raw.githubusercontent.com/wiki/vengi-voxel/vengi/images/voxedit-minecraft-skin-ext_2025-07-09.png#gallery)

Being not to pedantic, as these changes didn't happen in august, but in the last few months, I want to highlight some of the changes that have been made in that version: Like in 0.2.0 the performance and memory consumption was improved a lot. A lot of formats are now loading the scene by spreading the work over the available cpus. `vengi-voxconvert` can now print scene images to a text console - which is maybe useful - but definitely cool. The main goal for me was that I can print the voxels in headless mode when running unit tests. So instead of trying to guess which voxel is wrong, I can now see the error. This pixel image is also used to add thumbnails to formats that support it. The `vengi-voxconvert` tool doesn't have a graphical window or GL context available to use the renderer - that's why this simplified image generator was added.

The version 0.2.0 will include a few new features like animation support for cameras. You are now able to fly through the scene and more or less record the camera movement as an `AVI` or `mpeg` in the viewport. This will be useful for creating videos or presentations of the scenes.

New is also the support for the latest Magicavoxel format changes. The `META` tag is now supported and you can save in versions other than `150`. We should also be more resilient against corrupted files - the new `ogt_vox` version got a way to only log warnings on unexpected data instead of throwing an error.

The `fbx` format was buggy when saving files in the ascii format (the binary format is not even supported yet). Turns out the file needs a particular ascii header to be recognized by the Autodesk FBX tools. This is now fixed and the ascii format can be used again.

The Aseprite format also got support for sprite stacking [youtube](https://youtu.be/-bN8D_QrmJI) with two new cvars: `voxformat_imagesliceoffsetaxis` and `voxformat_imagesliceoffset`.

Next to this some more fixes regarding the symlink handling in the file browser on linux have been made. But the more interesting part is most likely the memory consumption improvements while voxelizing meshes. This is of course always an ongoing task, but it's already worth to mention it here. The reduction came mostly by reducing the `SharedPtr` instance size by 50%. I've tried this multiple times before - but the ASAN always said no to memory alignment. Besides the `SharedPtr` size reduction that `MeshTri` class itself was also reduced by a few bytes - this only sounds like a small win, but taking into account that the `MeshTri` class is used for every triangle in the mesh, this adds up quickly - I learned to love the `perf` framework in the linux kernel - but especially the gui applications on top of it: `hotspot` and `heaptrack`.

As I am a one-man-show, I need to make updates as automated as possible. I've worked on that side of the project, too by extending the `prepare-release` makefile target to also include a check for missing documentation. Since the `ogt_vox` library was missing in the `update-libs` target, I've added it there, too.

I've created and merged pull requests in jpaver's excellent library `ogt_vox`. These included `META` chunk support - scene metadata (as the version of the file), but also the `ogt_assert_warn` macro to be more resilient against corrupted files (see above).

Also worth to mention is the transform of thumbnails between format conversions in `vengi-voxconvert`. Previously, you got a new simplified screenshot (added in 0.1.0) for formats that support thumbnails. Now you get the original thumbnail from the source file, which is more useful for formats that support it.

Last but not least the version updates of the dependencies also happened a few times. I've noticed that the mesh import could benefit from `meshoptimizers` simplification features. But right now this is not yet done.
