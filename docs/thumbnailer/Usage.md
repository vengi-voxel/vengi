# Usage

Print a detailed description of the program parameters for your particular version.

```sh
vengi-thumbnailer --help
```

> You can use the bash completion or zsh completion
>
> Put this in your `.bashrc` or `.zshrc`
>
> `source <(vengi-thumbnailer --completion bash)` (or replace `bash` by `zsh`)

* `--angles <pitch:yaw:roll>`: Set the camera angles (pitch:yaw:roll) in degrees (default: 0:0:0)
* `--camera-mode <mode>`: Allow to change the camera positioning for rendering (default: Free)
* `--distance <distance>`: Set the camera distance to the target (default: -1)
* `--fallback`: Create a fallback thumbnail if an error occurs
* `--image`: Create a simple 2d side view image of the scene - doesn't take any camera settings or lighting settings into account
* `--input <file>`: The input file to create a thumbnail for
* `--isometric`: Create an isometric thumbnail of the input file when `--image` is used
* `--output <file>`: The output image file
* `--position <x:y:z>`: Set the camera position (default: 0:0:0)
* `--size <size>`: Size of the thumbnail in pixels (default: 128)
* `--sunazimuth <azimuth>`: Set the sun azimuth (default: 135)
* `--sunelevation <elevation>`: Set the sun elevation (default: 45)
* `--turntable`: Render in different angles (16 by default)
* `--use-scene-camera`: Use the first scene camera for rendering the thumbnail

## bash completion

You can also use the bash completion script by adding this to your `.bashrc`

```sh
source <(vengi-thumbnailer --completion bash)
```
