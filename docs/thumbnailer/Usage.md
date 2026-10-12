# Usage

Print a detailed description of the program parameters for your particular version.

```sh
vengi-thumbnailer --help
```

> You can use the bash, zsh or powershell completion
>
> Put this in your `.bashrc`, `.zshrc` or PowerShell `$PROFILE`
>
> `source <(vengi-thumbnailer --completion bash)` (or replace `bash` by `zsh` or `powershell`)

* `--angles <pitch:yaw:roll>`: Set the camera angles (pitch:yaw:roll) in degrees (default: 0:0:0)
* `--camera-mode <mode>`: Allow to change the camera positioning for rendering (default: Free)
* `--distance <distance>`: Set the camera distance to the target (default: -1)
* `--fallback`: Create a fallback thumbnail if an error occurs
* `--image`: Create a simple 2d side view image of the scene - doesn't take any camera settings or lighting settings into account
* `--input <file>`: The input file to create a thumbnail for
* `--isometric`: Create an isometric thumbnail of the input file when `--image` is used
* `--output <file>`: The output image file
* `--pathtracer`: Render with the GPU pathtracer instead of the forward renderer (ignores embedded format screenshots)
* `--position <x:y:z>`: Set the camera position (default: 0:0:0)
* `--quality <default|preview|high>`: Pathtracer quality (default: `default`). Only used with `--pathtracer`
* `--samples <n>`: Override the pathtracer sample count from `--quality`. Only used with `--pathtracer`
* `--size <size>`: Size of the thumbnail in pixels (default: 128)
* `--sunazimuth <azimuth>`: Set the sun azimuth (default: 135)
* `--sunelevation <elevation>`: Set the sun elevation (default: 45)
* `--turntable`: Render in different angles (16 by default)
* `--use-scene-camera`: Use the first scene camera for rendering the thumbnail

### Pathtracer quality

| Quality | Samples | Bounces | Notes |
| --- | --- | --- | --- |
| `default` | 256 | 4 | Balanced quality |
| `preview` | 16 | 2 | Fast draft |
| `high` | 1024 | 8 | Rounded voxel edges and bloom |

These match voxedit's **Quality presets** (not the sun **Preset: Noon / Evening / Morning** shortcuts). `--samples` overrides the sample count after quality is applied. Camera, sun and size options still apply. See [Path tracer](../PathTracer.md) for scene-size limits.

## Shell completion

### bash

Add this to your `.bashrc`:

```sh
source <(vengi-thumbnailer --completion bash)
```

### zsh

Add this to your `.zshrc`:

```sh
source <(vengi-thumbnailer --completion zsh)
```

### PowerShell

Add this to your PowerShell profile (`$PROFILE`):

```powershell
vengi-thumbnailer --completion powershell | Invoke-Expression
```
