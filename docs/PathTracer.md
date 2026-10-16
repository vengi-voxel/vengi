# Path tracer

The **Render** viewport in [voxedit](voxedit/Features.md) uses a GPU path tracer to light your voxels with soft shadows, reflections, glass, media and emissive materials. Open that viewport to start sampling. For panel layout and menu locations, see the [Renderer guide](voxedit/usage/Renderer.md). Material properties are documented in [Materials](Material.md).

The [thumbnailer](thumbnailer/Usage.md) can also render pathtraced previews with `--pathtracer`, `--quality` and `--samples`.

## What gets rendered

* Only **visible model nodes** are included.
* Each model keeps its **own palette**; colors and materials from every node are combined for shading.
* The camera starts from the last active edit or scene viewport, then moves on its own (mouse orbit and the view cube).
* Scene, camera or quality changes restart sampling. Exposure, contrast, saturation and vignette update the finished preview without discarding samples; bloom and tone mapping restart sampling.
* Use **Save image** in the viewport menu bar to export a PNG of the current preview.

## How large can a scene be?

The tracer stores your scene as a sparse set of **8x8x8 bricks**. Empty space is cheap; only bricks that contain voxels use significant GPU memory.

| Limit                     | Practical meaning                                                                                                                                                                                                                                                |
| ------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Scene bounding box        | Capped by your GPU's maximum 3D texture size (often **2048 voxels per axis** on desktop drivers; some GPUs allow more). The empty-space index is also capped at about **512 MiB**, which is roughly a **4096^3** voxel box when the GPU allows that many bricks. |
| Solid voxels              | Only occupied bricks are stored. On a typical 2048-limit GPU the 3D brick atlas holds about **16.7 million bricks** (~**8.5 billion** solid voxels in theory; memory is the practical limit). Larger GPUs allow more.                                              |
| Emissive voxels           | Up to **4096** emit voxels are used for direct lighting. If the scene has more, a random subset is kept so very dense light fields stay interactive.                                                                                                             |
| Unique colors / materials | Up to about **16,000** distinct palette colors that actually appear in the scene (across all node palettes).                                                                                                                                                     |
| Truncation warning        | If the scene is too large for these limits, the Render viewport menu bar shows **Volume truncated to GPU 3D texture / memory limits**, and parts of the scene outside the kept region are missing from the render.                                               |

Tips for large worlds: keep models packed near each other when possible, hide nodes you do not need in the render, and prefer fewer huge empty bounding boxes only when you accept that the index still has to cover that box.

## Settings (artist overview)

Defaults match **Reset all**. In voxedit these live under **Settings -> Quality presets** (not the sun **Preset: Noon / Evening / Morning** buttons under Lighting / Scene settings). The thumbnailer uses the same quality names via `--quality`.

### Quality presets

| Quality               | Samples | Bounces | Notes                                                                                                     |
| --------------------- | ------- | ------- | --------------------------------------------------------------------------------------------------------- |
| Reset all / `default` | 256     | 4       | Default look: 512^2 image, floor on (voxedit), ACES tone map, light vignette, no bloom / no rounded edges |
| Preview / `preview`   | 16      | 2       | Fast check of lighting and materials                                                                      |
| High quality / `high` | 1024    | 8       | Adds mild voxel-edge rounding (0.4) and bloom (0.35)                                                      |

### Quality

| Setting          | Range              | What it does                                                                                                       |
| ---------------- | ------------------ | ------------------------------------------------------------------------------------------------------------------ |
| Samples          | typically 16-4096+ | More samples = less noise, longer waits. The menu bar shows `Sample current / target`.                             |
| Bounces          | 1-**12**           | How many times light can bounce. Raise this for glass, mirrors and indirect light.                                 |
| Samples per pass | 1-**16**           | How much work each frame does. Higher values finish sooner but can hitch more while sampling.                      |
| Voxel edge       | 0-1                | Softens exposed corners. `0` is a hard cube; higher values look more rounded and cost more.                        |
| Render scale     | 0.25-1             | Traces a smaller image, then upsamples. `0.5` uses about one quarter of the pixels (including the saved PNG size). |

### Output

| Setting                          | Range / options                                       | What it does                                |
| -------------------------------- | ----------------------------------------------------- | ------------------------------------------- |
| Dimensions                       | square pixels (default **512**)                       | Width and height of the rendered image      |
| Exposure                         | -5 ... 5 stops                                        | Brightness after tone mapping               |
| Tone mapping                     | None, AgX, AgX Golden, AgX Punchy, **ACES** (default) | Film response                               |
| Contrast / saturation / vignette | contrast 0.5-1.5, saturation 0-2, vignette 0-1        | Display grading (does not restart sampling) |
| Bloom                            | 0-2                                                   | Soft glow on bright emit and sun highlights |

### Camera

| Setting        | Range  | What it does                             |
| -------------- | ------ | ---------------------------------------- |
| Depth of field | on/off | Enables lens blur                        |
| Aperture       | 0-0.5  | Blur amount (`0` is a pinhole)           |
| Focus distance | 1-256  | Distance that stays sharp when DOF is on |

### Lighting

| Setting                       | Notes                                                                                                                                                                             |
| ----------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Sun elevation / azimuth       | Shared with **Scene settings**. Sun shortcuts are labeled **Preset: Noon / Evening / Morning** (lighting only, not pathtracer quality). Softness is the sun disk size in degrees. |
| Sun intensity / color         | Direct sunlight strength and tint                                                                                                                                                 |
| Sky top / horizon / intensity | Gradient sky used as environment light                                                                                                                                            |
| Floor                         | Optional ground plane (color and Y height -256...256)                                                                                                                             |
| Background                    | Sky, solid color, or transparent (checker preview; useful for PNG export)                                                                                                         |
| Fog density / color           | Distance haze                                                                                                                                                                     |
| Atmosphere                    | Soft world haze that brightens toward the sun                                                                                                                                     |

## Faster previews and cleaner finals

* Start with the **Preview** quality preset, then raise samples for the final image.
* Lower **render scale** or **voxel edge** while framing the shot.
* Glass, media and many bounce paths take longer than diffuse scenes.
* Dense emit materials look brighter with **flux**; keep emitter counts reasonable if light sampling becomes noisy.
* A finished preview is kept until you change the scene, camera or a setting that forces a restart.

## Materials that matter in the path tracer

Palette materials drive the look: Diffuse, Metal, Glass, Emit, Blend and Media, plus roughness, metal, IOR, emit/flux, transparency and media density. The forward editor viewport mainly shows type and alpha; the path tracer uses the full material set. See [Materials](Material.md).

---

## For developers

Implementation lives in `src/modules/voxelpathtracer`, with shared sampling and shading helpers under `src/modules/voxelrender/shaders`.

### Algorithms and references

| Method                                                 | Use in the renderer                                                       | Reference                                                                                                                                                                                                        |
| ------------------------------------------------------ | ------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Amanatides-Woo voxel traversal (3D DDA)                | Voxel intersections, with empty-block skipping over the sparse brick grid | [A Fast Voxel Traversal Algorithm for Ray Tracing](https://doi.org/10.2312/egtp.19871000), 1987                                                                                                                  |
| Rounded-box signed distance function (SDF)             | Exposed voxel edges, neighbor-dependent radius                            | [Inigo Quilez: distance functions](https://iquilezles.org/articles/distfunctions/)                                                                                                                               |
| Sobol sequence with digital shifts                     | Pixel and lens samples                                                    | [PBRT: Sobol samplers](https://pbr-book.org/4ed/Sampling_and_Reconstruction/Sobol_Samplers)                                                                                                                      |
| Tent filter; Shirley-Chiu concentric disk mapping      | Pixel reconstruction and lens sampling                                    | [PBRT: tent sampling](https://pbr-book.org/4ed/Sampling_Algorithms/Sampling_1D_Functions), [disk mapping](https://pbr-book.org/4ed/Sampling_Algorithms/Sampling_Multidimensional_Functions)                      |
| PCG RXS-M-XS                                           | Random numbers for path scattering                                        | [Jarzynski and Olano: Hash Functions for GPU Rendering](https://jcgt.org/published/0009/03/02/), 2020                                                                                                            |
| Trowbridge-Reitz (GGX), Smith masking-shadowing, VNDF  | Rough reflection and refraction                                           | [PBRT: microfacet theory](https://pbr-book.org/4ed/Reflection_Models/Roughness_Using_Microfacet_Theory), [Heitz: Sampling the GGX Distribution of Visible Normals](https://jcgt.org/published/0007/04/01/), 2018 |
| Schlick's Fresnel approximation                        | Surface reflection                                                        | [Schlick: An Inexpensive BRDF Model for Physically-based Rendering](https://doi.org/10.1111/1467-8659.1330233), 1994                                                                                             |
| Malley's method                                        | Cosine-weighted diffuse directions                                        | [PBRT: hemisphere sampling](https://pbr-book.org/4ed/Sampling_Algorithms/Sampling_Multidimensional_Functions)                                                                                                    |
| Dielectric Fresnel equations                           | Glass reflection and transmission                                         | [PBRT: rough dielectric BSDF](https://pbr-book.org/4ed/Reflection_Models/Rough_Dielectric_BSDF)                                                                                                                  |
| Multiple importance sampling (MIS), balance heuristic  | Combines environment and surface samples                                  | [PBRT: Improving Efficiency](https://pbr-book.org/4ed/Monte_Carlo_Integration/Improving_Efficiency)                                                                                                              |
| Walker's alias method, Vose's table construction       | Selects emissive voxels by luminance                                      | [PBRT: The Alias Method](https://pbr-book.org/4ed/Sampling_Algorithms/The_Alias_Method)                                                                                                                          |
| Environment importance sampling                        | Samples the sky through a luminance-weighted distribution                 | [PBRT: Infinite Area Lights](https://pbr-book.org/4ed/Light_Sources/Infinite_Area_Lights)                                                                                                                        |
| Probabilistic path termination with refraction scaling | Terminates low-contribution paths without penalizing travel through glass | [PBRT: A Better Path Tracer](https://pbr-book.org/4ed/Light_Transport_I_Surface_Reflection/A_Better_Path_Tracer)                                                                                                 |
| Beer-Lambert law; Henyey-Greenstein phase function     | Attenuation and scattering in media                                       | [PBRT: Transmittance](https://pbr-book.org/4ed/Volume_Scattering/Transmittance), [Phase Functions](https://pbr-book.org/4ed/Volume_Scattering/Phase_Functions)                                                   |
| Separable Gaussian convolution                         | Bloom, horizontal and vertical seven-tap filters                          | [GPU Gems 2: Advanced High-Quality Filtering](https://developer.nvidia.com/gpugems/gpugems2/part-iii-high-quality-rendering/chapter-27-advanced-high-quality-filtering)                                          |

The sky is a custom color gradient in 16 equal-solid-angle bands. Emitter positions and visibility use voxel approximations. GPU storage uses a coarse indirection grid plus a 3D atlas of 8^3 bricks packed along X then Y then Z; brick voxels pack a dense material id with transmits/emit flags, and albedo/materials are flat tables over used colors.

### Benchmarking

Enable `USE_BENCHMARKS=ON` and build `benchmarks-voxelpathtracer`. Use an optimized build and repeat measurements on the same GPU and driver:

```sh
systemd-run --user --scope -p CPUQuota=200% -p MemoryMax=4G \
  build/vengi/vengi-benchmarks-voxelpathtracer \
  --benchmark_min_time=1s --benchmark_repetitions=5 \
  --benchmark_out=/tmp/pathtracer.json --benchmark_out_format=json
```

* `Trace` renders 16 samples with 4 bounces, excluding scene upload and display processing.
* `Preview` measures the full 16-sample render with display updates, comparing GPU display with image download/upload.
* `InteractiveFrame` measures an active or completed preview, with bloom on or off, for empty and opaque scenes. Active cases trace one sample; each case uses 32 iterations.

Scene setup and warmup are excluded. `Time` uses GPU timer queries for `Trace` and `InteractiveFrame` when supported; `Preview` uses synchronized wall time. `endToEnd_ms` includes CPU submission and the GPU wait. `items_per_second` counts pixel samples.
