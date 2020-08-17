# Scripting api

There is a console command in voxedit to execute lua scripts for generating voxels. This command expects the lua script filename and the additional arguments for the `main()` method.

There are two functions in each script. One is called `arguments` and one `main`. `arguments` returns a list of parameters for the `main` function. The default parameters for `main` are `volume`, `region` and `color`.

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

* `match(r, g, b)`: returns the closest possible palette color match for the given RGB (`0-255`) color (). The returned palette index is in the range `0-255`. This value can then be used for the `setVoxel` method.

# Region

* `mins()`:

* `maxs()`:

* `x()`:

* `y()`:

* `z()`:

* `width()`:

* `height()`:

* `depth()`:

# Volume

* `voxel(x, y, z)`: returns the palette index of the voxel at the given position in the volume

* `region()`: return the region of the volume

* `setVoxel(x, y, z, color)`: set the given color to the given coordinates in the volume
