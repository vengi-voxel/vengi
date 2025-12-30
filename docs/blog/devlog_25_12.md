# December 2025

The last few days of the year and no blog post yet... better late than never.

## Command & Conquer

Probably the biggest change to mention is the normal painting mode that I've just released a few minutes ago. You are now able to change the normals just as you would edit voxel colors. There is a new brush to change those normals. To enable this feature, you have to select the proper view mode:

![voxedit view mode](https://raw.githubusercontent.com/wiki/vengi-voxel/vengi/images/voxedit-normaledit1_2025-12-30.png#gallery)

Select _Command & Conquer_ here to enable the new brush as well as the normal palette panel.

![voxedit normal palette](https://raw.githubusercontent.com/wiki/vengi-voxel/vengi/images/voxedit-normalpanal_2025-12-30.png#gallery)

The following screenshots show the normal painting options, as well as a long-standing bug that was fixed. The `vxl`/`hva` loading was not applying the offsets properly. This is now hopefully fixed.

![normaledit](https://raw.githubusercontent.com/wiki/vengi-voxel/vengi/images/voxedit-normaledit2_2025-12-30.png#gallery)

Rendering the normals:

![normaledit-rendernormals](https://raw.githubusercontent.com/wiki/vengi-voxel/vengi/images/voxedit-normaledit3_2025-12-30.png#gallery)

Auto normals for the selected node:

![normaledit-autonormals](https://raw.githubusercontent.com/wiki/vengi-voxel/vengi/images/voxedit-normaledit4_2025-12-30.png#gallery)

The auto normals were also improved. If you encounter any issues with C&C support regarding `vxl` files, please open an issue.

## Optimizations

A blog post without mentioning that I was wasting CPU cycles in my previous implementation? Not possible - here we go. I've optimized some UI panels that were consuming excessive CPU cycles. Several voxel algorithms now utilize multithreading for faster execution.

But as always... this is ongoing.

## Removed UI component

A complete UI component was removed because the tree generators were converted to Lua. They now simply use the Lua script panel options for their parameters. This resolves a rather old bug ticket.

## Lindenmayer system

An improved Lindenmayer system integration has been implemented. It helps in creating individual rules and also performs some sanity checks:

![lsystem](https://raw.githubusercontent.com/wiki/vengi-voxel/vengi/images/voxedit-lsystem_2025-12-30.png#gallery)
