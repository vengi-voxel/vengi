# Frequently asked questions

## I've found a bug or have a feature request.

Please either connect to our [discord server](https://vengi-voxel.de/discord) or open a ticket on the [vengi github](https://github.com/vengi-voxel/vengi/) page.

## What is a cvar?

I am a quake child and a cvar is a **c**onfiguration **var**iable. This allows you to change the behaviour of the program from outside. See the [configuration](Configuration.md) article for more details. You can also always see all cvars and options of a tool by calling it from the command line with `--help` as parameter.

## Does vengi support format XY?

We have a table with [formats](Formats.md). You can find here whether only reading or also writing is supported.

## Does vengi support animations?

Yes, vengi supports animations and [vengi-voxedit](voxedit/Index.md) has a timeline to manage key frames. You can even export those animations to some formats like `gltf` and re-import them afterwards.

> See this [youtube video](https://youtu.be/mynzgoaoxXo)

## Does vengi support shaders like magicavoxel does?

Sort of - vengi doesn't use shaders for this - but [lua](LUAScript.md) scripts. There are already quite a few example available in the `scripts` folder that comes bundles with your vengi application - from small helper scripts to full scene creation.

## No colors after voxelization - what's wrong?

Your model doesn't have the colors you want? This usually happens if your input model doesn't have properly set up the texture paths. Vengi is trying to perform some lookups automatically. But the texture names have to match. What does this mean? A file that specifies a material texture with the name `albedo.png` but only comes with a texture named `texture-albedo.png` won't get proper colors.

**Textures are stored in a subfolder:**

Sometimes the textures were moved into subfolders, e.g. a `textures` subfolder - but the `fbx`, `glb` (you name it) files were not adopted and only refer to `texture1.png` where they should have been pointed to `textures/texture1.png`. In this case you can specify the [cvar](Configuration.md) `voxformat_texturepath` to point to the path where the texture files are (but keep in mind, that the names still have to match).

Sometimes it's also just the case-sensitivity of your filesystem. This is true for both the path and the filename (including the extension).

## My model is very small/big after voxelization

Vengi uses 1 unit in your mesh for one voxel. So if your model is scaled down or up to fit the needs for your own workflow, you can scale the model up with the [cvar](Configuration.md) `voxformat_scale`. If your model is only shown very small/big in [vengi-voxedit](voxedit/Index.md) scene mode, this means that the node got a scaling applied. Switch to the node inspector and reset the scale value for the node. This is not related to the previously mentioned cvar - the cvar scales the geometry - not the transformation matrices in the scene.

## Some parts of a model are not available after vengi saved them

There are features that are not supported for all formats that vengi supports. E.g. most formats are not able to store animations. If you are going to create a scene in [vengi-voxedit](voxedit/Index.md), the best option to preserve all settings and scene features is to save in `vengi` format itself. You can later convert to any other supported [format](Formats.md) - but in case the target format doesn't support a feature, you still have the full scene file in `vengi` format.

## Is vengi available in my language?

Probably not - but vengi offers [translation](Translation.md) support. So if nobody was faster in translating vengi into your language, feel free to do so. And please also consider to send the translation.

## Which platforms are supported?

Windows, Linux, MacOS and web

## How to run the `vengi-voxconvert` command line tool on Windows

Open a terminal and navigate to the folder where you extracted the files. You can use `cmd.exe` or `PowerShell`.

To run the tool, use the command `vengi-voxconvert.exe` followed by the parameters you want to use.

If you just double click it from the explorer, it will open a console window and close it immediately after the tool has finished.

## It doesn't work on my system

Please fill a bug report on the [vengi github](https://github.com/vengi-voxel/vengi/) page. Please include as much information as possible, like your operating system, the version of vengi you are using and the steps to reproduce the issue. There are usually also files called `log.txt` and `crash.txt` in your [vengi data folder](Configuration.md#configuration-file) that you can attach to the issue.

## My voxedit UI layout is broken

This might happen when e.g. switching between versions or running a nightly builds. You can easily reset your layout by going to `Edit`->`Options`->`Reset layout`.
