# Physics

On the physics side, the editor now includes basic first-person walking, which is especially helpful when building maps for games like *Ace of Spades*. You can move through corridors and examine your work directly in the scene, and game-mode controls have been improved—including better stair navigation and proper handling of pivoted nodes. Editing while in game mode is now possible as well, smoothing out the creative workflow. You can tweak body height, velocity, and other parameters directly in the editor to mimic how other games behave.

![image voxedit game mode](https://raw.githubusercontent.com/wiki/vengi-voxel/vengi/images/vengi-physics-gamemode_2025-11-30.png)

[youtube video](https://www.youtube.com/watch?v=oprAEWN9rN8)

# Shadows

Shadow rendering continues to evolve, and this update includes another important refinement: improved culling ensures that mesh parts contributing to shadow maps are no longer dropped too aggressively. While shadow tuning remains an ongoing effort, this change noticeably reduces missing or incorrect shadows in many scenes.

# Performance

Performance also received some attention again. Several operations that previously triggered many small allocations now use larger, consolidated buffers, reducing overhead throughout the engine. A major slowdown in transparency sorting was resolved after profiling revealed costly iterations over empty objects, and additional optimizations—such as faster axis rotations—help the editor feel more responsive overall. While investigating, I also added a few more trace markers for the excellent Tracy profiler.

# vengi-palconvert

This release brings a broad set of improvements across the vengi ecosystem, starting with expanded palette capabilities. The palette tools can now work with more than 256 colors, making it easier to import richer palettes from various sources. To help keep workflows compatible with voxel formats, new options allow you to quantize palettes back down to 8-bit or optimize away unused or empty colors.

# Formats and Other Improvements

A number of formats and tools were expanded or fixed as well. Support for Tiberian Sun’s `vxl` format was improved (though pivot and scaling issues remain). Blockbench workflows benefit from better handling of external textures. AniVoxel `voxa` files are now supported (without bone handling yet), and the project’s GLTF pipeline now relies on its own filesystem abstraction.

Improvements were also made to hollowing, animation interpolation, culling, and several platform-specific issues. Additionally, the voxel editor’s brush system received updates to both the stamp brush and the line brush. The stamp brush now supports a user-provided offset to easily shift the placement of the stamp volume by a fixed distance. The line brush, in turn, gained a continuous mode and a stipple pattern that can be applied to each line.

[youtube video](https://www.youtube.com/watch?v=LhzUZAtqx-s)
