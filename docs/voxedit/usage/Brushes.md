# Brushes

![voxedit-brushes](../../img/voxedit-brushes.png)

VoxEdit provides several specialized brushes for different types of voxel editing.

### Shape brush

The shape brush creates geometric primitives by spanning an axis-aligned bounding box (AABB) in the viewport. Click and drag to define the size of the shape. The orientation is determined by which face you clicked on initially.

**Available shapes:**

- **AABB (Cube)**: Creates rectangular volumes. Can be hollow or filled depending on the modifier.
- **Torus**: Creates donut shapes with configurable radii.
- **Cylinder**: Generates circular columns along the axis perpendicular to the face you clicked.
- **Cone**: Creates tapered cylinders with the base on the clicked face.
- **Dome**: Generates hemisphere or half-ellipse shapes.
- **Ellipse**: Creates stretched spheres that fill the entire bounding box.

The shape is automatically oriented based on the surface normal of the face where you started dragging.
This allows you to naturally create shapes aligned with your viewing direction.

### Paint brush

The paint brush provides various methods for coloring and modifying existing voxels. The brush only affects existing voxels and won't place new ones, making it ideal for recoloring models without changing their geometry.

**Paint modes:**

- **Replace**: Changes all voxels in the region to the cursor color
- **Brighten**: Makes colors lighter by a configurable factor
- **Darken**: Makes colors darker by a configurable factor
- **Random**: Replaces voxels with random colors from the palette
- **Variation**: Randomly brightens or darkens voxels for natural-looking surfaces

**Special modes:**

- **Plane**: Fills all connected voxels of the same color on the clicked surface
- **Gradient**: Creates smooth color transitions across the region from the hit color to the cursor color

**Options:**

- **Factor**: Controls the brightness adjustment for Brighten/Darken modes (1.0 = no change)
- **Variation Threshold**: For Variation mode, sets the 1 in N chance to modify each voxel

### Plane brush

The plane brush creates or extrudes entire planar surfaces in a single action. When you click a face, it generates voxels across that entire plane. If you drag after clicking, you can extrude the plane outward to create thickness.

This brush is particularly useful for quickly creating walls, floors, or large flat surfaces without having to manually span regions.

### Stamp brush

The stamp brush allows you to place pre-loaded voxel volumes repeatedly into your scene. Think of it as a copy-paste tool that keeps the copied content ready for multiple placements.

**Loading stamps:**

- Drag and drop any supported voxel file onto the viewport
- Right-click in the asset panel to select a stamp
- Create a stamp from a single voxel type with configurable dimensions

**Stamp modes:**

- **Center Mode**: The stamp is centered on the cursor position
- **Corner Mode**: The stamp's corner aligns with the cursor position
- **Continuous Mode**: Keep placing the same stamp with each click

**Offset**: You can configure an offset vector to shift where the stamp is placed relative to the cursor. This is useful for precise alignment or creating regular patterns.

### Line brush

The line brush draws straight lines of voxels from the reference position to wherever you click. The reference position (shown as a blue dot in edit mode) must be set first before using this brush.

**Line options:**

- **Stipple Pattern**: A 9-bit pattern that controls which voxels are placed along the line. Each bit represents one step - set bits place voxels, cleared bits skip them. This allows creating dashed or dotted lines.
- **Continuous Mode**: When enabled, the end point of each line becomes the reference position for the next line, allowing you to chain line segments together without manually updating the reference position.

Lines are drawn using raycasting, ensuring a clean voxel path between the two points.

### Path brush

The path brush creates paths that follow existing voxel surfaces from the reference position to the cursor position. Unlike the line brush which draws straight lines through empty space, the path brush walks along solid voxels.

**Connectivity modes:**

- **Six Connected**: Paths can only move along cardinal directions (up, down, left, right, forward, back)
- **Eighteen Connected**: Paths can move diagonally along edges
- **Twenty-Six Connected**: Paths can move in all directions including diagonal corners

This brush requires existing voxels to walk on and won't work in empty volumes. It's useful for creating roads, pipes, or other features that need to follow terrain.

### Text brush

The text brush voxelizes TrueType fonts into your volume, allowing you to add text labels or create text-based voxel art.

**Text options:**

- **Font**: Choose any TrueType font file (.ttf) from your system
- **Size**: Controls the font size, which determines the height and width of generated characters
- **Spacing**: Sets the distance between individual characters
- **Thickness**: Defines how deep/thick the text appears in 3D space
- **Input**: The text string to generate

The text is placed at the cursor position.

### Texture brush

The texture brush projects image textures onto voxel surfaces, automatically mapping colors from the image to the closest matching colors in your palette.

**Texture options:**

- **Image**: Load any supported image format (PNG, JPG, etc.)
- **UV Coordinates**: Define which portion of the texture to use (UV0 to UV1)
- **Project onto surface**: When enabled, the texture is projected onto existing voxel surfaces. When disabled, it fills the spanned AABB volume.

The texture brush is excellent for adding detailed patterns, logos, or textures to models by converting pixel art into voxels.

### Script brush

Execute custom [Lua scripts](../../LUAScript.md) with this brush. Scripts can perform complex operations that aren't possible with the standard brushes, like procedural generation, mathematical transformations, or automated editing tasks.

Check the scripts panel to see available scripts and their parameters. Many scripts use the reference position, cursor position, or current selection to define where they operate.

### Select brush

The select brush doesn't modify voxels directly. Instead, it creates selection regions that limit where other operations can affect voxels (like the [script](../../LUAScript.md) execution) or for copy/pasting.

> Don't forget to unselect (__Select__ -> __Select none__) before being able to operate on the whole volume again.

**Using selections:**

1. Switch to the select modifier (or use the select brush)
2. Drag to span a selection box
3. Operations like painting, scripts, or other brushes will only affect voxels inside the selection
4. Use **Select -> Select none** to clear the selection and work on the entire volume again

Selections are useful for protecting parts of your model while editing other areas, or for applying operations to precise regions.

### Color picker

You can either use it from the modifiers panel or by default with the key `p` to pick the color from the current selected voxel.
