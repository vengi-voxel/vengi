# Scripting api

There is a console command (called `xs`) in [voxedit](voxedit/Index.md) and a command line parameter in [voxconvert](voxconvert/Index.md) to execute lua scripts for generating voxels. This command expects the lua script filename (`.lua` can be omitted) and the additional arguments for the `main()` method.

---

> **voxedit**
>
> Calling `xs <script> help` (in the script console) will print the supported arguments for the given script file in voxedit.

---

> **voxconvert**
>
> ```
> ./vengi-voxconvert --script "<script> help" --input in.qb --output out.qb
> ```

---

By default the script files will be searched in a `scripts` folder next to where the binary is located and in the usual search paths (see [configuration](Configuration.md) for more details). You can also give the full path to the script file.

There are two functions in each script. One is called `arguments` and one `main`. `arguments` returns a list of parameters for the `main` function. The default parameters for `main` are `node`, `region` and `color`. `color` is the palette index starting from `0`.

## Examples

### Without parameters

```lua
function main(node, region, color)
	local volume = node:volume()
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

### With one parameter

```lua
function arguments()
	return {
		{ name = 'n', desc = 'height level delta', type = 'int', default = '2' }
	}
end

function main(node, region, color, n)
	[...]
end
```

Execute this via console `xs scriptfile 1` where `1` will be the value of `n`. Omitting the `1` will add the `default` value from the argument list.

## Arguments

Supported `type`s are:

* `int`: `min`, `max` values are supported, too
* `float`: `min`, `max` values are supported, too
* `enum`: `enum` as a property specifies a list of string separated by `,`
* `str`: string input
* `colorindex`: a color index from current palette (clamped)
* `bool`:

The `desc`ription field is just for the user interface of the script parameter list.

A `default` value can get set, too.

The order in the arguments table defines the order in which the arguments are passed over to the script.

## SceneGraph

`scenegraph` lets you access different nodes or create new ones.

The functions are:

* `new(name, region[, visible])`: Creates a new node with the given `name`, the size and position according to the `region` and an optional `visible` parameter.

* `get([nodeId])`: Returns the `node` for the given `nodeId` - if the `nodeId` is not given, it will return the current active node. Which by default is the node for the volume the script is currently executed for.

## SceneGraphNode

* `name()`: Returns the current name of the node.

* `setName(string)`: Set the name of the node.

* `volume()`: Gives you access to the volume of the node.

## Color

`palette` has several methods to work with colors. E.g. to find a closest possible match for the given palette index.

The functions are:

* `color(paletteIndex)`: Pushes the vec4 of the color behind the palette index (`0-255`) as float values between `0.0` and `1.0`.

* `colors()`: Returns the palette RGBA colors as vec4 values.

* `match(r, g, b)`: Returns the closest possible palette color match for the given RGB (`0-255`) color. The returned palette index is in the range `0-255`. This value can then be used for the `setVoxel` method.

* `similar(paletteindex, [coloramount])`: Return a table with similar colors given by their palette index.

## Noise

`noise` supports a few noise generators:

* `noise2(v)`, `noise3(v)`, `noise4(v)`: Simplex noise. Uses the given `vec2`, `vec3` or `vec4` and returns a float value between `0.0` and `1.0`.

* `fBm2(v)`, `fBm3(v)`, `fBm4(v)`: Simplex noise fractal brownian motion sum. Uses the given `vec2`, `vec3` or `vec4` and returns a float value between `0.0` and `1.0`.

* `ridgedMF2(v)`, `ridgedMF3(v)`, `ridgedMF4(v)`: Simplex ridged multi-fractal noise sum. Uses the given `vec2`, `vec3` or `vec4` and returns a float value between `0.0` and `1.0`.

* `worley2(v)`, `worley3(v)`: Simplex cellular/worley noise. Uses the given `vec2` or `vec3` and returns a float value between `0.0` and `1.0`.

## Region

* `mins()`: The lower boundary of the region (inclusive).

* `maxs()`: The upper boundary of the region (inclusive).

* `setMins(mins)`: The lower boundary of the region - given as `ivec3`.

* `setMaxs(maxs)`: The upper boundary of the region - given as `ivec3`.

* `x()`: The lower x boundary of the region.

* `y()`: The lower y boundary of the region.

* `z()`: The lower z bounary of the region.

* `width()`: The width of the region measured in voxels.

* `height()`: The height of the region measured in voxels.

* `depth()`: The depth of the region measured in voxels.

## Volume

* `voxel(x, y, z)`: Returns the palette index of the voxel at the given position in the volume `[0-255]`. Or `-1` if there is no voxel.

* `region()`: Return the region of the volume.

* `translate(x, [y, z])`: Translates the region of the volume. Keep in mind that this is not supported by every output format.

* `resize(x, [y, z, extendMins])`: Resize the volume by the given sizes. If `extendsMins` is `true` the region dimensions are also increased on the lower corner.

* `setVoxel(x, y, z, color)`: Set the given color at the given coordinates in the volume. `color` must be in the range `[0-255]`.

## Vectors

Available vector types are `vec2`, `vec3`, `vec4` and their integer types `ivec2`, `ivec3`, `ivec4`.

```lua
local v1 = ivec3.new(1, 1, 1)
```

There are 3 possible components for this vector. You can also call `ivec3.new(1)` to fill all three values with a one. Or call it like this: `ivec3.new(1, 2)` to create a vector with the three components of `1, 2, 2`.

## Other

* `y` going upwards.

You have access to the `cvar` and `cmd` lua bindings, too. This means that you can access any `cvar` value or execute any command like `layeradd` or `layermerge` to modify the whole scene.

```lua
cmd.execute("echo test")
var.int("cl_gamma")
```

To get a full list of commands and cvars use the console command `cmdlist` and `cvarlist`.

## Available scripts

### cover.lua

Generates a new voxel on top of others with the current selected color and the specified height.

![cover](img/lua-cover.png)

`xs cover.lua 1`

### grass.lua

Generate grass on top of voxels.

![grass](img/grass.png)

`xs grass.lua`

### grid.lua

Generates a grid with given color, thickness and size.

![grid](img/lua-grid.png)

`xs grid.lua 1 1 5 5 5`

### noise.lua

Generates perlin noise with the frequency and amplitude as parameters with the current selected color.

![noise](img/lua-noise.png)

`xs noise.lua 0.3 1.0`

### pyramid.lua

Generates a pyramid with the current selected color and with each level being 3 voxels high.

![pyramid](img/lua-pyramid.png)

`xs pyramid.lua 3`

### thicken.lua

Thickens the voxel - take 1 voxel and convert to 8 voxels (creates a new node for the result).

![thickenbefore](img/lua-thicken-before.png) ![thickenafter](img/lua-thicken-after.png)

`xs thicken.lua 1`
