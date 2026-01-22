# January 2026

A new year and quite a lot has happened already! The first few weeks of January 2026 have been packed with improvements, optimizations, and new features. Here are the highlights:

## New Greedy Texture Mesher

One of the biggest additions this month is the new greedy texture mesher [#709](https://github.com/vengi-voxel/vengi/issues/709) (find images linked in the ticket). This advanced mesher uses a texture atlas approach instead of vertex colors, which dramatically reduces vertex counts for scenes with flat surfaces containing different colors. Instead of needing many vertices per surface, you now only get four vertices and a corresponding section in the texture atlas.

The new mesher is currently supported by OBJ and GLTF output formats, with more formats to follow.

## Offline Documentation

The editor now includes offline documentation [#713](https://github.com/vengi-voxel/vengi/issues/713)! A new help panel was added to [vengi-voxedit](../voxedit/Index.md) that renders markdown documentation directly in the application. This required integrating the `imgui_markdown` library and extending it with support for tables and code blocks - features that were contributed back to the upstream project (see below).

The markdown rendering system includes proper link handling with hand cursor feedback, making the documentation experience smooth and intuitive even when working without an internet connection.

## Keyboard Layout Detection

Added automatic keyboard layout detection, which is particularly important for supporting different regional keyboard configurations. This ensures that keybindings work correctly regardless of whether you're using QWERTY, QWERTZ, AZERTY, or other keyboard layouts.

## Mimetype Improvements

The mimetype handling system was refactored and improved. The mimetype code was moved into the `FormatDescription` class, providing better organization and making it easier to specify mimetypes for different file [formats](../Formats.md). For the first time, I'm not inventing new mimetypes for formats that already have an official one.

## File Dialog

On Linux, the file dialog now properly displays mount points, making it easier to access external drives and network shares. The feature is still missing for Mac - there is an [open ticket](https://github.com/vengi-voxel/vengi/issues/701) if anyone wants to jump in.

## Format Updates

Several file [format](../Formats.md) improvements were implemented - but those were primarily about camera and [material](../Material.md) handling.

## Camera Movement Updates

Camera movement received significant attention with improvements to both the traditional rotation-based camera and the new eye-based WASD movement mode. There is a new keybinding to switch the rotation types (`CTRL+r`) - WASD-based movement is possible in all modes now - even though there is a bug ticket left for orthographic movement.

## Bake Transforms and Node Merging

Implemented the long-requested feature to apply (bake) transforms to voxels. This allows you to take nodes that have been rotated or translated and permanently apply those transformations to the actual voxel data (the handling of the scale value from the transform matrices is still missing though). The feature uses trilinear sampling for smooth interpolation and backward mapping for accurate results. This means that rotations will no longer leave holes in the rotated volume.

## Physics and Rendering Optimizations

Several performance optimizations were implemented throughout the codebase:

- Parallelized the quad merging process in the cubic surface extractor, which provides major speedups for large scenes (some scenes have hundreds of thousands of quads to merge)
- Improved physics collision detection performance
- Optimized render state management by deferring uniform state changes and program binding

## Pipe Support

I've added named pipe support to allow external tools to send commands to [vengi-voxedit](../voxedit/Index.md) (requires `app_pipe` to be set to `true` - see [configuration](../Configuration.md)).

You can e.g. run [lua scripts](../LUAScript.md) from the outside, or trigger any of the existing commands to modify or create scenes.

## imgui_markdown

While adding the offline documentation to [vengi-voxedit](../voxedit/Index.md), I decided to use `imgui_markdown` and since I use many tables in the documentation under `docs/`, I extended the library with table support and sent a [pull request](https://github.com/enkisoftware/imgui_markdown/pull/43) upstream - let's see if it is accepted.
