# Animation

> **IMPORTANT**: Animations are only saved in the `vengi` format or when exported to [formats](../Formats.md) that support animations. You should always save to `vengi` and then export from here to any other format that supports animations, too.

## Videos

* [youtu.be/5aC3-QumPcw](https://youtu.be/5aC3-QumPcw) (animation scripting via lua - see [scripts](../LUAScript.md))
* [youtu.be/NL5aEO_an1w](https://youtu.be/NL5aEO_an1w) (loading the `hmec` animation from command & conquer)
* [youtu.be/vXTOkH3HwIo](https://youtu.be/vXTOkH3HwIo) (showing the animation of a windmill)
* [youtu.be/mynzgoaoxXo](https://youtu.be/mynzgoaoxXo) (export as gltf with animations and re-import into vengi)

## Details

There is a timeline for creating keyframes and you can transform the objects with the gizmo if the editor is in scene mode.

Check out the options in the animation timeline to e.g. create keyframes automatically whenever you change something in the transform data.

Enable **Long rotation** beside the keyframe's interpolation settings in the node inspector to take the longer rotation path to the next keyframe. For example,
a 90 degree rotation becomes a 270 degree rotation in the opposite direction. Identical endpoint orientations stay still because they do not specify an axis
for a full turn; add an intermediate keyframe to animate a full turn.

**CatmullRom** interpolates translation and scale with a cubic spline through the neighboring keyframe values. Rotation still uses slerp between the two keys of the current segment. The first and last segments duplicate the endpoint when a neighbor is missing.

## Scripting

You can also create animations from [scripts](../LUAScript.md). There is an example available with `animate.lua` where you can create a few character animations based on node names.
