# Shape

Global: `g_shape`

## Functions

| Function | Description |
| -------- | ----------- |
| `bezier(volume, start, end, control, color, thickness)` | Draw a quadratic bezier curve in the volume. |
| `cone(volume, centerBottom, axis, negative, width, height, depth, color)` | Create a cone shape in the volume. |
| `cube(volume, position, width, height, depth, color)` | Create a cube shape in the volume. |
| `cylinder(volume, centerBottom, axis, radius, height, color)` | Create a cylinder shape in the volume. |
| `dome(volume, centerBottom, axis, negative, width, height, depth, color)` | Create a dome (half ellipsoid) shape in the volume. |
| `ellipse(volume, centerBottom, axis, width, height, depth, color)` | Create an ellipse (filled oval) shape in the volume. |
| `line(volume, start, end, color, thickness)` | Draw a line between two points in the volume. |
| `torus(volume, center, minorRadius, majorRadius, color)` | Create a torus shape in the volume. |

## Detailed Documentation

### bezier

Draw a quadratic bezier curve in the volume.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `volume` | `volume` | The volume to draw in. |
| `start` | `ivec3` | The start position. |
| `end` | `ivec3` | The end position. |
| `control` | `ivec3` | The control point. |
| `color` | `integer` | The color index (optional, default 1). |
| `thickness` | `integer` | The line thickness (optional, default 1). |

### cone

Create a cone shape in the volume.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `volume` | `volume` | The volume to draw in. |
| `centerBottom` | `ivec3` | The center bottom position. |
| `axis` | `string` | The axis: 'x', 'y', or 'z' (default 'y'). |
| `negative` | `boolean` | Flip the cone direction (optional, default false). |
| `width` | `integer` | The width of the cone base. |
| `height` | `integer` | The height of the cone. |
| `depth` | `integer` | The depth of the cone base. |
| `color` | `integer` | The color index (optional, default 1). |

### cube

Create a cube shape in the volume.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `volume` | `volume` | The volume to draw in. |
| `position` | `ivec3` | The corner position. |
| `width` | `integer` | The width of the cube. |
| `height` | `integer` | The height of the cube. |
| `depth` | `integer` | The depth of the cube. |
| `color` | `integer` | The color index (optional, default 1). |

### cylinder

Create a cylinder shape in the volume.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `volume` | `volume` | The volume to draw in. |
| `centerBottom` | `vec3` | The center bottom position. |
| `axis` | `string` | The axis: 'x', 'y', or 'z' (default 'y'). |
| `radius` | `integer` | The radius of the cylinder. |
| `height` | `integer` | The height of the cylinder. |
| `color` | `integer` | The color index (optional, default 1). |

### dome

Create a dome (half ellipsoid) shape in the volume.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `volume` | `volume` | The volume to draw in. |
| `centerBottom` | `ivec3` | The center bottom position. |
| `axis` | `string` | The axis: 'x', 'y', or 'z' (default 'y'). |
| `negative` | `boolean` | Flip the dome direction (optional, default false). |
| `width` | `integer` | The width of the dome. |
| `height` | `integer` | The height of the dome. |
| `depth` | `integer` | The depth of the dome. |
| `color` | `integer` | The color index (optional, default 1). |

### ellipse

Create an ellipse (filled oval) shape in the volume.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `volume` | `volume` | The volume to draw in. |
| `centerBottom` | `ivec3` | The center bottom position. |
| `axis` | `string` | The axis: 'x', 'y', or 'z' (default 'y'). |
| `width` | `integer` | The width of the ellipse. |
| `height` | `integer` | The height of the ellipse. |
| `depth` | `integer` | The depth of the ellipse. |
| `color` | `integer` | The color index (optional, default 1). |

### line

Draw a line between two points in the volume.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `volume` | `volume` | The volume to draw in. |
| `start` | `ivec3` | The start position. |
| `end` | `ivec3` | The end position. |
| `color` | `integer` | The color index (optional, default 1). |
| `thickness` | `integer` | The line thickness (optional, default 1). |

### torus

Create a torus shape in the volume.

**Parameters:**

| Name | Type | Description |
| ---- | ---- | ----------- |
| `volume` | `volume` | The volume to draw in. |
| `center` | `ivec3` | The center position. |
| `minorRadius` | `integer` | The minor (tube) radius. |
| `majorRadius` | `integer` | The major (ring) radius. |
| `color` | `integer` | The color index (optional, default 1). |

