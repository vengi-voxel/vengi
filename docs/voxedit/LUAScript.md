# Scripting api

There is a console command in voxedit to execute lua scripts for generating voxels. This command expects the lua script filename and the additional arguments for the `main()` method.

There are two functions in each script. One is called `arguments` and one `main`. `arguments` returns a list of parameters for the `main` function. The default parameters for `main` are `volume`, `region` and `color`. `color` is the palette index starting from `0`.

# Example without parameters

```lua
function main(volume, region, color)
	local mins = region:mins()
	local maxs = region:maxs()
	for x = mins.x, maxs.x do
		for y = mins.y, maxs.y do
			for z = mins.z, maxs.z do
				volume:setVoxel(x, y, z, color)
			end
		end
	end
end
```

Execute this via console `xs scriptfile`

# Example with one parameter

```lua
function arguments()
	return {
		{ name = 'n', desc = 'height level delta', type = 'int' }
	}
end

function main(volume, region, color, n)
	[...]
end
```

Execute this via console `xs scriptfile 1` where `1` will be the value of `n`.

# Color

`palette` has several methods to work with colors. E.g. to find a closest possible match for the given palette index.

The functions are:

* `color(paletteIndex)`: pushes the vec4 of the color behind the palette index (`0-255`) as float values between `0.0` and `1.0`.

* `colors()`: returns the palette RGBA colors as vec4 values.

* `match(r, g, b)`: returns the closest possible palette color match for the given RGB (`0-255`) color (). The returned palette index is in the range `0-255`. This value can then be used for the `setVoxel` method.

# Region

* `mins()`: the lower boundary of the region (inclusive)

* `maxs()`: the upper boundary of the region (inclusive)

* `x()`: the lower x boundary of the region

* `y()`: the lower y boundary of the region

* `z()`: the lower z bounary of the region

* `width()`: the width of the region measured in voxels

* `height()`: the height of the region measured in voxels

* `depth()`: the depth of the region measured in voxels

# Volume

* `voxel(x, y, z)`: returns the palette index of the voxel at the given position in the volume `[0-255]`. Or -1 if there is no voxel

* `region()`: return the region of the volume

* `setVoxel(x, y, z, color)`: set the given color to the given coordinates in the volume. `color` must be in the range `[0-255]`

# Other

* `y` going upwards

# Available scripts

## noise.lua

Generates perlin noise with the frequency and amplitude as parameters with the current selected color.

![noise](lua-noise.png)

`xs noise.lua 0.3 1.0`

## cover.lua

Generates a new voxel on top of others with the current selected color and the specified height.

![cover](lua-cover.png)

`xs cover.lua 1`

## pyramid.lua

Generates a pyramid with the current selected color and with each level being 3 voxels high.

![pyramid](lua-pyramid.png)

`xs pyramid.lua 3`
